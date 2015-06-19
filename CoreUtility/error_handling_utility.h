#pragma once

#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib") 
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <WinInet.h>
#include <ShlObj.h>
#include <type_traits>
#include <stdexcept>
#pragma warning(push)
#pragma warning(disable: 4995)
#include <string>
#pragma warning(pop)
#include <vector>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <map>
#include "scope_guard.h"


#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)

#undef min
#undef max

//
// Platform Message Window
//
class ExceptionWithMiniDump : public virtual std::runtime_error
{
public:
    ExceptionWithMiniDump(std::string const& msg, std::wstring miniDumpFileFullPath)
        : std::runtime_error(msg)
        , dumpFileFullPath_(std::move(miniDumpFileFullPath))
    { }

    std::wstring GetMiniDumpFilePath()
    {
        return dumpFileFullPath_;
    }

private:
    std::wstring dumpFileFullPath_;
};

#define MAX_MSG (MAX_PATH + 256)

struct LogExceptionEntry
{
	wchar_t expMsg[MAX_MSG];
	size_t msgLength;
};

inline void DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo)
{
    if (excpInfo == NULL) 
    {
        __try
        {
            RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
        } 
        __except(DumpMiniDump(hFile, GetExceptionInformation()),
            EXCEPTION_EXECUTE_HANDLER) 
        {
        }
    } 
    else
    {
        MINIDUMP_EXCEPTION_INFORMATION eInfo;
        eInfo.ThreadId = GetCurrentThreadId();
        eInfo.ExceptionPointers = excpInfo;
        eInfo.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpNormal,
            excpInfo ? &eInfo : NULL,
            NULL,
            NULL);
    }
}

std::wstring DumpMiniDumpFile(PEXCEPTION_POINTERS excpInfo = NULL);

namespace BingWallPaper { namespace Detail 
{
    template <typename T>
    std::wstring ToWString(T const& t)
    {
        std::wstringstream wss;
        wss << t;
        return wss.str();
    }

    class EnsureBase
    {
    public:
        typedef std::map<std::wstring, std::wstring> TContextVarMap;

        EnsureBase(std::wstring expr)
            : expr_(expr)
        { }

    protected:
        std::wstring   expr_;
        std::wstring   file_;
        long           line_;
        TContextVarMap contextVars_;
    };

    inline std::string ToAscii(std::wstring const& wstr)
    {
        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::vector<char> str(requiredSize, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], (int)str.size(), nullptr, nullptr);
        return &str[0];
    }

    inline std::wstring GetFileName(std::wstring const& path)
    {
        return path.substr(path.find_last_of(L"/\\") + 1);
    }

    inline std::wstring FormatMessage(
        std::wstring const& expr, 
        EnsureBase::TContextVarMap const& contextVars,
        std::wstring const& file,
        long line)
    {
        std::wstringstream ss;
        ss << expr << L" ["
            << GetFileName(file) << L", " << line << L"]\n";

        if(!contextVars.empty())
        {
            std::for_each(contextVars.begin(), contextVars.end(), 
                [&](EnsureBase::TContextVarMap::const_reference p)
            {
                ss << L"    " << p.first << L" = " << p.second << L"\n";
            });
        }

        return ss.str();
    }

    struct ThrowBehavior
    {
        void operator()(
            std::wstring const& expr, 
            EnsureBase::TContextVarMap const& contextVars, 
            std::wstring const& file, 
            long line)
        {
            throw ExceptionWithMiniDump(ToAscii(FormatMessage(expr, contextVars, file, line)), DumpMiniDumpFile());
        }
    };

    struct ThrowWithoutDumpBehavior
    {
        void operator()(
            std::wstring const& expr, 
            EnsureBase::TContextVarMap const& contextVars, 
            std::wstring const& file, 
            long line)
        {
            throw std::runtime_error(ToAscii(FormatMessage(expr, contextVars, file, line)));
        }
    };

    struct AssertBehavior
    {
        void operator()(
            std::wstring const& expr, 
            EnsureBase::TContextVarMap const& contextVars, 
            std::wstring const& file, 
            long line)
        {
#ifndef NDEBUG
            _wassert(FormatMessage(expr, contextVars, file, line).c_str(), file.c_str(), line);
#endif
            ThrowBehavior()(expr, contextVars, file, line);
        }
    };

	inline LRESULT SendBingWallPaperExceptionMessage(HWND hwnd, LogExceptionEntry& log, HWND hClientMsgWnd = NULL)
	{
		COPYDATASTRUCT copyData;
		copyData.dwData = /*MsgGroup_LOG_EXCEPTION*/9;
		copyData.cbData = sizeof(log);
		copyData.lpData = &log;

		DWORD_PTR result = 0;
		SendMessageTimeout(hwnd, WM_COPYDATA, (WPARAM)hClientMsgWnd, (LPARAM)&copyData, SMTO_ABORTIFHUNG, /*SendMessageTimeoutInMS = */ 1000, &result);
		return result;
	}

	struct LogWithoutDumpBehavior
	{
		void operator()(
			std::wstring const& expr, 
			EnsureBase::TContextVarMap const& contextVars, 
			std::wstring const& file, 
			long line)
		{
			LogExceptionEntry log;
			std::wstring msg = FormatMessage(expr, contextVars, file, line);
			log.msgLength = std::min(size_t(MAX_MSG), msg.length());
			memcpy_s(log.expMsg, MAX_MSG * sizeof(wchar_t), msg.data(), log.msgLength * sizeof(wchar_t));
			// TODO: we can send log here. Optional.
		}
	};

#pragma warning(push)
#pragma warning(disable: 4355)

    template <typename TBehavior>
    class Ensure : private EnsureBase
    {
    public:
        Ensure(std::wstring expr)
            : EnsureBase(std::move(expr)), 
            ENSURE_A(*this), 
            ENSURE_B(*this),
            abandoned_(false)
        { }

        Ensure(Ensure const& other)
            : EnsureBase(other), 
            ENSURE_A(*this), 
            ENSURE_B(*this),
            abandoned_(other.abandoned_)
        { 
            other.abandoned_ = true;
        }

        template <typename T>
        Ensure& PrintContextVar(std::wstring const& name, T&& val)
        {
            contextVars_[name] = ToWString(val);
            return *this;
        }

        Ensure& PrintLocation(std::wstring file, long line)
        {
            file_ = std::move(file);
            line_ = line;
            return *this;
        }

        ~Ensure()
        {
            if(abandoned_)
            {
                return;
            }

            TBehavior()(expr_, contextVars_, file_, line_);
        }

        Ensure& ENSURE_A;
        Ensure& ENSURE_B;
    private:
        Ensure& operator=(const Ensure&);
    private:
        mutable bool abandoned_;
    };

#pragma warning(pop)

    inline Ensure<ThrowBehavior> MakeEnsureThrow(std::wstring expr)
    {
        return Ensure<ThrowBehavior>(std::move(expr));
    }

    inline Ensure<ThrowWithoutDumpBehavior> MakeEnsureThrowWithoutDump(std::wstring expr)
    {
        return Ensure<ThrowWithoutDumpBehavior>(std::move(expr));
    }

    inline Ensure<AssertBehavior> MakeEnsureAssert(std::wstring expr)
    {
        return Ensure<AssertBehavior>(std::move(expr));
    }

	inline Ensure<LogWithoutDumpBehavior> MakeEnsureLogWithoutDump(std::wstring expr)
	{
		return Ensure<LogWithoutDumpBehavior>(std::move(expr));
	}
} } // BingWallPaper::Detail

#define __WFILE__ WIDEN(__FILE__)

#define ENSURE_A(x) ENSURE_OP(x, B)
#define ENSURE_B(x) ENSURE_OP(x, A)
#define ENSURE_OP(x, next) \
    ENSURE_A.PrintContextVar(WIDEN(#x), (x)).ENSURE_##next

#ifdef _DEBUG
#define ENSURE_CORE(expr) \
    if((expr)) static_assert(std::is_same<decltype(expr), bool>::value, "ENSURE(expr) can only be used on bool expression"); \
    else BingWallPaper::Detail::MakeEnsureAssert(WIDEN(#expr)).PrintLocation(__WFILE__, __LINE__).ENSURE_A
#define FAIL_CORE(expr) \
    BingWallPaper::Detail::MakeEnsureAssert(WIDEN(#expr)).PrintLocation(__WFILE__, __LINE__).ENSURE_A
#else
#define ENSURE_CORE(expr) \
    if((expr)) static_assert(std::is_same<decltype(expr), bool>::value, "ENSURE(expr) can only be used on bool expression"); \
    else BingWallPaper::Detail::MakeEnsureThrowWithoutDump(WIDEN(#expr)).PrintLocation(__WFILE__, __LINE__).ENSURE_A
#define FAIL_CORE(expr) \
    BingWallPaper::Detail::MakeEnsureThrowWithoutDump(WIDEN(#expr)).PrintLocation(__WFILE__, __LINE__).ENSURE_A
#endif

inline std::wstring FormatMessage(DWORD dwError, DWORD dwFlags, LPCVOID source)
{
    LPVOID pText = 0;
    ::FormatMessage(dwFlags|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS, 
        source, dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&pText, 0, NULL);
    std::wstring msg = BingWallPaper::Detail::ToWString(dwError);
    if(pText)
    {
        msg.append(L": ");
        msg.append((LPWSTR)pText);
    }

    LocalFree(pText);
    return msg;
}

inline std::wstring Win32ErrorMessage(DWORD dwError)
{
    return FormatMessage(dwError, FORMAT_MESSAGE_FROM_SYSTEM, NULL);
}

inline std::wstring WinHttpErrorMessage(DWORD dwError)
{
    return FormatMessage(dwError, FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(L"winhttp.dll"));
}

inline std::wstring GetLastErrorStr()
{
    return Win32ErrorMessage(GetLastError());
}

#define ENSURE(exp) ENSURE_CORE(exp)(GetLastErrorStr())
#define FAIL FAIL_CORE(GetLastErrorStr())
#define ENSURE_SUCCEEDED(hr) \
    if(SUCCEEDED(hr)) static_assert(std::is_same<decltype(hr), HRESULT>::value, "ENSURE_SUCCEEDED(hr) can only be used on HRESULT"); \
    else ENSURE(SUCCEEDED(hr))(Win32ErrorMessage(hr))

#define THROW_IF(expr) \
    if(!(expr)) static_assert(std::is_same<decltype(expr), bool>::value, "THROW_IF(expr) can only be used on bool expression"); \
    else BingWallPaper::Detail::MakeEnsureThrowWithoutDump(WIDEN(#expr)).PrintLocation(__WFILE__, __LINE__).ENSURE_A

#define THROW_IF_WIN32(expr) THROW_IF(expr)(GetLastErrorStr())
#define THROW_IF_WINHTTP(expr) THROW_IF(expr)(WinHttpErrorMessage(GetLastError()))

#define LOG_IF(expr) LOG_IF_CORE(expr)(GetLastErrorStr())

#define LOG_IF_CORE(expr) \
	if(!(expr)) static_assert(std::is_same<decltype(expr), bool>::value, "LOG_IF(expr) can only be used on bool expression"); \
	else BingWallPaper::Detail::MakeEnsureLogWithoutDump(WIDEN(#expr)).PrintLocation(__WFILE__, __LINE__).ENSURE_A

inline void MuteAllExceptions(
    std::function<void()> action, 
    std::function<void(std::exception&)> onFail = [](std::exception&){}
)
{
    try
    {
        action();
    }
    catch(std::exception& ex)
    {
        onFail(ex);
    }
    catch(...)
    {
    }
}

inline std::wstring PathCombine(std::wstring const& dir, std::wstring const& file)
{
    WCHAR finalPath[MAX_PATH + 1];
    ENSURE(PathCombine(finalPath, dir.c_str(), file.c_str()) != NULL);
    return finalPath;
}

inline std::wstring CreateDirectoryRecursive(std::wstring const& folder)
{
    SHCreateDirectoryEx(NULL, folder.c_str(), NULL);
    return folder;
}

inline std::wstring GetSpecialFolder(REFKNOWNFOLDERID rfid)
{
    typedef HRESULT (STDAPICALLTYPE * fnptr_SHGetKnownFolderPath)(__in REFKNOWNFOLDERID rfid, __in DWORD dwFlags, __in_opt HANDLE hToken, __deref_out PWSTR *ppszPath);
    fnptr_SHGetKnownFolderPath fnSHGetKnownFolderPath = (fnptr_SHGetKnownFolderPath) GetProcAddress(GetModuleHandle(TEXT("shell32")),"SHGetKnownFolderPath");

    if (fnSHGetKnownFolderPath != NULL)
    {
        PWSTR p;

        if (fnSHGetKnownFolderPath(rfid, 0, NULL, &p) != S_OK) 
        {
            return L"";
        }

        std::wstring p1(p);
        CoTaskMemFree(p);
        return p1;
    } 
    else
    {
        // SHGetKnownFolderPath is not found, in XP
        WCHAR p[MAX_PATH];
        ENSURE(rfid == FOLDERID_LocalAppData || rfid == FOLDERID_LocalAppDataLow || rfid == FOLDERID_RoamingAppData);
        ENSURE(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, p) == S_OK);
        return p;
    }
}

inline std::wstring GetLocalAppDataLowFolder()
{
    return GetSpecialFolder(FOLDERID_LocalAppDataLow);
}

inline std::wstring DumpMiniDumpFile(PEXCEPTION_POINTERS excpInfo)
{
    std::wstring localAppDataLowFolderPath = GetLocalAppDataLowFolder(); // the expected localAppDataLowFolderPath is like: "C:\\Users\\youralias\\AppData\\LocalLow\\"

    if (localAppDataLowFolderPath.empty())	
    {
        return L"";
    }

    std::wstring crashRptFolder(PathCombine(localAppDataLowFolderPath, L"BingWallPaper"));
    CreateDirectoryRecursive(crashRptFolder);

    std::wstring miniDumpFileFullPath = PathCombine(crashRptFolder, L"crash.dmp");

    HANDLE hMiniDumpFile = CreateFile(
        miniDumpFileFullPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL);

    if (hMiniDumpFile == INVALID_HANDLE_VALUE)
    {
        return L"";
    }

    ON_SCOPE_EXIT([&] { CloseHandle(hMiniDumpFile); });

    DumpMiniDump(hMiniDumpFile, excpInfo);

    return miniDumpFileFullPath;
}

template <typename Ret, typename Func>
inline auto CxxExceptionBoundary(
    Func const& action, Ret returnOnFailure,
    std::function<void(std::exception_ptr)> const& onCxxException
    ) -> decltype(action())
{
    try
    {
        return action();
    }
    catch(...)
    {
        onCxxException(std::current_exception());
        return returnOnFailure;
    }
}

template <typename Ret, typename Func>
inline auto Win32ExceptionBoundary(
    Func const& action, Ret returnOnFailure, 
    std::function<void(unsigned int, EXCEPTION_POINTERS*)> const& onWin32Exception
    ) -> decltype(action())
{
    __try
    {
        return action();
    }
    __except(
        onWin32Exception(GetExceptionCode(), GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {
        return returnOnFailure;
    }
}

template <typename Ret, typename Func>
inline auto AllExceptionsBoundary(
    Func const& action, Ret returnOnFailure,
    std::function<void(std::exception_ptr)> const& onCxxException,
    std::function<void(unsigned int, EXCEPTION_POINTERS*)> const& onWin32Exception
    ) -> decltype(action())
{
    return Win32ExceptionBoundary(
        [&] { return CxxExceptionBoundary(action, returnOnFailure, onCxxException); },
        returnOnFailure, onWin32Exception);
}

inline void AllExceptionsBoundary(
    std::function<void()> action, 
    std::function<void(std::exception_ptr)> const& onCxxException,
    std::function<void(unsigned int, EXCEPTION_POINTERS*)> const& onWin32Exception
    )
{
    AllExceptionsBoundary(
        [&]()->int { action(); return 0; },
        0,
        onCxxException,
        onWin32Exception
        );
}
