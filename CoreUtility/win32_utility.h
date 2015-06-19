#pragma once

#include <windows.h>
#include <string>
#include <locale>
#include <codecvt>
#include <fstream>
#include <wininet.h> // must be included before shlobj.h
#include <shlobj.h>
#include <Shellapi.h>
#include <stdarg.h>
#include "string_utility.h"
#include "http.h"
#include <ShTypes.h>
#include "error_handling_utility.h"
#include <cstdlib>
#include <memory>
using namespace std;

inline void CenterWindow(HWND hwndWindow, HWND hwndRelative = NULL)
{
	RECT rectWindow, rectRelative;

	::GetWindowRect(hwndWindow, &rectWindow);
	::GetWindowRect(hwndRelative == NULL ? ::GetDesktopWindow() : hwndRelative, &rectRelative);

	int nWidth = rectWindow.right - rectWindow.left;
	int nHeight = rectWindow.bottom - rectWindow.top;

	int nX = (rectRelative.left + rectRelative.right - nWidth) / 2;
	int nY = (rectRelative.top + rectRelative.bottom - nHeight) / 2 ;

	::MoveWindow(hwndWindow, nX, nY, nWidth, nHeight, FALSE);
}

inline RECT MakeRect(int left, int top, int right, int bottom)
{
	RECT rc = { left, top, right, bottom };
	return rc;
}

inline POINT MakePoint(int x, int y)
{
	POINT pt = { x, y };
	return pt;
}

inline int CalcDistance(POINT pt1, POINT pt2)
{
	return (int)pow(pow((pt1.x - pt2.x),2) + pow((pt1.y - pt2.y),2), 0.5);
}

inline HWND GetDesktopDefViewWnd()
{
	HWND hWnd = NULL;  
	HWND hDefView = NULL;  

	hWnd = ::FindWindow(L"Progman", L"Program Manager");
	if(hWnd)
	{  
		hDefView = ::FindWindowEx(hWnd, NULL, L"SHELLDLL_DefView", NULL);
	}

	if (hDefView == NULL)
	{ 
		hWnd = ::FindWindow(L"WorkerW", NULL);
		while(hWnd)
		{
			hDefView = ::FindWindowEx(hWnd, NULL, L"SHELLDLL_DefView", NULL);
			if (hDefView)
			{
				break;
			}
			hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
		}
	}

	return hDefView;
}

inline HWND GetDesktopListViewWnd()
{
	HWND hWnd = NULL;  
	HWND hListView = NULL;  

	hWnd = ::FindWindow(L"Progman", L"Program Manager");
	if(hWnd)
	{  
		hListView = ::FindWindowEx(hWnd, NULL, L"SHELLDLL_DefView", NULL);
		hListView = ::FindWindowEx(hListView, NULL, L"SysListView32", NULL);
	}

	if (hListView == NULL)
	{ 
		hWnd = ::FindWindow(L"WorkerW", NULL);
		while(hWnd)
		{
			hListView = ::FindWindowEx(hWnd, NULL, L"SHELLDLL_DefView", NULL);
			hListView = ::FindWindowEx(hListView, NULL, L"SysListView32", NULL);
			if (hListView)
			{
				break;
			}
			hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
		}
	}

	return hListView;
}

inline HWND FindWindowByProp(HWND hParent, LPCTSTR szProp)
{
	HWND hWnd = ::GetWindow(hParent, GW_CHILD);   
	while(::IsWindow(hWnd))
	{
		if(::GetProp(hWnd, szProp))   
		{
			return hWnd;
		}
		hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
	}
	return NULL;
}

typedef struct tagLVITEM64
{
	UINT mask;
	int iItem;
	int iSubItem;
	UINT state;
	UINT stateMask;
	INT64 pszText;
	int cchTextMax;
	int iImage;
	LPARAM lParam;
#if (_WIN32_IE >= 0x0300)
	int iIndent;
#endif
#if (_WIN32_WINNT >= 0x501)
	int iGroupId;
	UINT cColumns; // tile view columns
	PUINT puColumns;
#endif
} LVITEM64, *LPLVITEM64;

inline bool Is64BitOS()
{
	typedef void (WINAPI *LPFN_PGNSI)(LPSYSTEM_INFO);

	bool bRetVal = false;
	SYSTEM_INFO si = { 0 };
	LPFN_PGNSI pGNSI = (LPFN_PGNSI)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
	if (pGNSI == NULL)
	{
		return false;
	}
	pGNSI(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || 
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 )
	{
		bRetVal = true;
	}

	return bRetVal;
}

inline RECT GetDesktopIconRect32(std::wstring iconText)
{
	//get the handle of the desktop listview
	HWND hListView = GetDesktopListViewWnd();

	//get total count of the icons on the desktop
	int count = (int)::SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);

	//get the handle of the process(explorer.exe actually) which the listview belongs to.
	DWORD dwPID = 0;
	::GetWindowThreadProcessId(hListView, &dwPID);
	HANDLE hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

	//allocate memory in the virtual address space of the specific process
	LVITEM* plvItem = (LVITEM*)::VirtualAllocEx(hProc, NULL, sizeof(LVITEM), MEM_COMMIT, PAGE_READWRITE); 
	wchar_t* pItemText = (wchar_t*)::VirtualAllocEx(hProc, NULL, 512*sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE); 
	RECT* pItemRect = (RECT*)::VirtualAllocEx(hProc, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE); 

	//get item text and rect
	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.cchTextMax = 512;
	lvItem.pszText = pItemText;
	wchar_t itemText[512] = {0};
	RECT rc = {-1};

	for(int i=0; i<count; i++) 
	{
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		::WriteProcessMemory(hProc, plvItem, &lvItem, sizeof(lvItem), NULL);
		::SendMessage(hListView, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)plvItem);
		::ReadProcessMemory(hProc, pItemText, itemText, sizeof(itemText), NULL);

		if (std::wstring(itemText) == iconText)
		{
			rc.left = LVIR_BOUNDS;
			::WriteProcessMemory(hProc, pItemRect, &rc, sizeof(rc), NULL);
			::SendMessage(hListView, LVM_GETITEMRECT, (WPARAM)i, (LPARAM)pItemRect);
			::ReadProcessMemory(hProc, pItemRect, &rc, sizeof(rc), NULL);
			break;
		}
	}

	//free the memory
	::VirtualFreeEx(hProc, plvItem, 0, MEM_RELEASE);
	::VirtualFreeEx(hProc, pItemText, 0, MEM_RELEASE);
	::VirtualFreeEx(hProc, pItemRect, 0, MEM_RELEASE);
	CloseHandle(hProc);
	return rc;
}

inline RECT GetDesktopIconRect64(std::wstring iconText)
{
	//get the handle of the desktop listview
	HWND hListView = GetDesktopListViewWnd();

	//get total count of the icons on the desktop
	int count = (int)::SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);

	//get the handle of the process(explorer.exe actually) which the listview belongs to.
	DWORD dwPID = 0;
	::GetWindowThreadProcessId(hListView, &dwPID);
	HANDLE hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

	//allocate memory in the virtual address space of the specific process
	LVITEM64* plvItem = (LVITEM64*)::VirtualAllocEx(hProc, NULL, sizeof(LVITEM64), MEM_COMMIT, PAGE_READWRITE); 
	wchar_t* pItemText = (wchar_t*)::VirtualAllocEx(hProc, NULL, 512*sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE); 
	RECT* pItemRect = (RECT*)::VirtualAllocEx(hProc, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE); 

	//get item text and rect
	LVITEM64 lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.cchTextMax = 512;
	lvItem.pszText = (INT64)pItemText;
	wchar_t itemText[512] = {0};
	RECT rc = {-1};

	for(int i=0; i<count; i++) 
	{
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		::WriteProcessMemory(hProc, plvItem, &lvItem, sizeof(lvItem), NULL);
		::SendMessage(hListView, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)plvItem);
		::ReadProcessMemory(hProc, pItemText, itemText, sizeof(itemText), NULL);

		if (std::wstring(itemText) == iconText)
		{
			rc.left = LVIR_BOUNDS;
			::WriteProcessMemory(hProc, pItemRect, &rc, sizeof(rc), NULL);
			::SendMessage(hListView, LVM_GETITEMRECT, (WPARAM)i, (LPARAM)pItemRect);
			::ReadProcessMemory(hProc, pItemRect, &rc, sizeof(rc), NULL);
			break;
		}
	}

	//free the memory
	::VirtualFreeEx(hProc, plvItem, 0, MEM_RELEASE);
	::VirtualFreeEx(hProc, pItemText, 0, MEM_RELEASE);
	::VirtualFreeEx(hProc, pItemRect, 0, MEM_RELEASE);
	CloseHandle(hProc);
	return rc;
}

inline RECT GetDesktopIconRect(std::wstring iconText)
{
	RECT rc = {0};
	if (!Is64BitOS())
	{
		rc = GetDesktopIconRect32(iconText);
	}
	else
	{
		rc = GetDesktopIconRect64(iconText);
	}

	return rc;
}

inline bool IsPtInDesktopIconRect(POINT pt)
{
	bool bRet = false;
	//get the handle of the desktop listview
	HWND hListView = GetDesktopListViewWnd();

	//get total count of the icons on the desktop
	int count = (int)::SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);

	//get the handle of the process(explorer.exe actually) which the listview belongs to.
	DWORD dwPID = 0;
	::GetWindowThreadProcessId(hListView, &dwPID);
	HANDLE hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

	//allocate memory in the virtual address space of the specific process
	RECT* pItemRect = (RECT*)::VirtualAllocEx(hProc, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE); 

	//get item rect
	RECT rc = {-1};

	for(int i=0; i<count; i++) 
	{
		rc.left = LVIR_BOUNDS;
		::WriteProcessMemory(hProc, pItemRect, &rc, sizeof(rc), NULL);
		::SendMessage(hListView, LVM_GETITEMRECT, (WPARAM)i, (LPARAM)pItemRect);
		::ReadProcessMemory(hProc, pItemRect, &rc, sizeof(rc), NULL);
		if (::PtInRect(&rc, pt))
		{
			bRet = true;
			break;
		}
	}

	//free the memory
	::VirtualFreeEx(hProc, pItemRect, 0, MEM_RELEASE);
	CloseHandle(hProc);
	return bRet;
}


//-----------------------------------------------
inline std::wstring GetSpecialDir(int id)
{
	wchar_t dir[1024];
	memset(dir, 0, sizeof(dir)); 
	BOOL bSuccess = ::SHGetSpecialFolderPath(NULL, dir, id, FALSE);
	return std::wstring(dir);
}

inline std::wstring GetAppDataDir()
{
	return GetSpecialDir(CSIDL_APPDATA);
}

inline std::wstring GetDesktopDir()
{
	return GetSpecialDir(CSIDL_DESKTOP);
}

//------------------------------------------------
inline std::wstring GetParentDir(std::wstring const& filePath)
{
	return filePath.substr(0, filePath.find_last_of(L"\\"));
}

inline std::wstring GetExePath()
{
	wchar_t exePath[1024];
	::GetModuleFileName(NULL, exePath, 1024);
	return std::wstring(exePath);
}

inline std::wstring GetExeDir()
{
	return GetParentDir(GetExePath());
}

inline std::wstring GetDllDir(HMODULE hInst)
{
	wchar_t szModule[MAX_PATH];
	if (0 != GetModuleFileName(hInst, szModule, ARRAYSIZE(szModule)))
	{
		return GetParentDir(szModule);
	}
	return L"";
}

inline std::wstring CombinePath(LPCTSTR arg, ...)
{
	va_list argPtr;//va_list 
	va_start(argPtr, arg);//va_start(arg_ptr,count);
	std::wstring result;
	result = arg;
	while (1)
	{
		std::wstring tmp;
		tmp = va_arg(argPtr, LPCTSTR);
		if(tmp == L"")
		{
			break;
		}

		result = TrimRight(result, L"\\") + L"\\" + TrimLeft(tmp, L"\\");
	}

	va_end(argPtr);
	return result;
}

inline bool FileExist(std::wstring filePath)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	hFind = FindFirstFile(filePath.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	else
	{
		FindClose(hFind);
		return true;
	}
}

inline bool DirExist(std::wstring dir)
{	
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(dir.c_str(), &data);
	if(hFind==INVALID_HANDLE_VALUE)
	{
		return false;
	}
	else
	{
		FindClose(hFind);
		return true;
	}
}

inline bool CopyDir(std::wstring dirFrom, std::wstring dirTo)
{
	SHFILEOPSTRUCT FileOp; 
	ZeroMemory((void*)&FileOp,sizeof(SHFILEOPSTRUCT));

	FileOp.fFlags = FOF_NOCONFIRMATION ; 
	FileOp.hNameMappings = NULL; 
	FileOp.hwnd = NULL; 
	FileOp.lpszProgressTitle = NULL; 
	int len1 = dirFrom.length();
	dirFrom.reserve(len1+1);
	dirFrom.insert(len1,1,'\0');
	FileOp.pFrom = dirFrom.c_str();
	int len2 = dirTo.length();
	dirTo.reserve(len2+1);
	dirTo.insert(len2,1,'\0');
	FileOp.pTo = dirTo.c_str(); 
	FileOp.wFunc = FO_COPY; 

	return SHFileOperation(&FileOp) == 0;
}

inline bool DeleteDir(std::wstring dir)
{
	SHFILEOPSTRUCT FileOp; 
	ZeroMemory((void*)&FileOp,sizeof(SHFILEOPSTRUCT));

	FileOp.fFlags = FOF_NOCONFIRMATION ; 
	FileOp.hNameMappings = NULL; 
	FileOp.hwnd = NULL; 
	FileOp.lpszProgressTitle = NULL; 
	int len1 = dir.length();
	dir.reserve(len1+1);
	dir.insert(len1,1,'\0');
	FileOp.pFrom = dir.c_str();
	FileOp.pTo = NULL; 
	FileOp.wFunc = FO_DELETE; 

	return SHFileOperation(&FileOp) == 0;
}

inline std::wstring ReadWholeFile(std::wstring filePath)
{
	::SHCreateDirectory(NULL, GetParentDir(filePath).c_str());

	std::wifstream ifs;
	ifs.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t,0x10ffff,std::generate_header>));
	ifs.open(filePath, std::ios::in);
	std::wstring content;
	std::wstring line;
	if (ifs.is_open())
	{
		while (ifs.good())
		{
			getline(ifs, line);
			content+=line;
		}
	}
	ifs.close();
	if (content[0]==0xfeff)
	{
		content = content.substr(1);
	}
	return content;
}

inline void WriteWholeFile(std::wstring filePath, std::wstring content)
{
	::SHCreateDirectory(NULL, GetParentDir(filePath).c_str());

	std::wofstream ofs;
	ofs.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t,0x10ffff,std::generate_header>)); 
	ofs.open(filePath, std::ios::out);
	ofs<<content;
	ofs.close();
}

inline void DownloadFile(std::wstring url, std::wstring filePath)
{
	::SHCreateDirectory(NULL, GetParentDir(filePath).c_str());
	//get response
	std::vector<BYTE> reqData;
	std::vector<BYTE> response = HttpRequest(url, L"GET", L"", reqData);

	//save response to file
	FILE* fp = NULL; 
	errno_t err = _wfopen_s(&fp, filePath.c_str(), L"wb+");
	if (!fp||err)
	{
		return;
	}
	size_t len = response.size();
	if(len > 0)
	{
		fwrite(&response[0], 1, len, fp);
	}
	fflush(fp);
	fclose(fp);
}

inline int GetFileSize(std::wstring filePath)
{
	std::wifstream ifs;
	int size = 0;
	try
	{
		ifs.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t,0x10ffff,std::generate_header>));
		ifs.open(filePath, std::ios::in);
		ifs.seekg(0, std::ios::end);
		size = (int)ifs.tellg();
		ifs.close();
	}
	catch (...)
	{
		ifs.close();
	}
	return size;
}

inline int Rename(std::wstring oldName, std::wstring newName)
{
	return _wrename(oldName.c_str(), newName.c_str());
}

inline std::wstring GetIEVersion()
{
	//get the trident path
	TCHAR szSystemDirectoryPath[MAX_PATH] = {0}; 
	UINT nSize = GetSystemDirectory(szSystemDirectoryPath, MAX_PATH);
	if (nSize == 0)
	{
		//failed to get system dir
		return L"";
	}
	wstring strTridentPath = wstring(szSystemDirectoryPath)+L"\\mshtml.dll";

	//get file version info size, in bytes
	DWORD dwHandle = 0;
	DWORD dwSize = GetFileVersionInfoSize(strTridentPath.c_str(), &dwHandle);
	if (0 == dwSize) 
	{
		//failed to get the size
		return L"";
	}

	//assign memory for file version info
	char* pVerInfo = new char[dwSize];
	if (NULL == pVerInfo) 
	{
		//failed to new
		return L"";
	}
	ON_SCOPE_EXIT([&]
	{
		delete[] pVerInfo;
		pVerInfo = NULL;
	});

	//get file version info
	if(!GetFileVersionInfo(strTridentPath.c_str(), 0, dwSize, (PVOID)pVerInfo)) 
	{
		//failed to get info
		return L"";
	}

	//get codepage and lang
	typedef struct LANGANDCODEPAGE 
	{
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;
	LANGANDCODEPAGE *pLangCode = NULL;
	UINT  uLen = 0;
	if (!VerQueryValue(pVerInfo, _T("\\VarFileInfo\\Translation"), (PVOID*)&pLangCode, &uLen))
	{
		return L"";
	}


	//Now we have the file version info and the lang, codepage. We can now use these to get ProductVersion
	PVOID pBuffer = NULL; //NOTE: pTmp must be PVOID or LPVOID
	TCHAR subblock[MAX_PATH];
	memset((void*)subblock, 0, sizeof(subblock));
	for(UINT i=0; i<(uLen/sizeof(LANGANDCODEPAGE)); i++)
	{
		//Compose the SubBlock string to retrieve the specific information.
		//------------------------------------------------------
		//information we can retrieve:
		//OriginalFilename
		//Comments			InternalName		ProductName
		//CompanyName		LegalCopyright		ProductVersion
		//FileDescription	LegalTrademarks		PrivateBuild
		//FileVersion		OriginalFilename	SpecialBuild  
		//------------------------------------------------------
		swprintf_s(subblock, MAX_PATH,
			_T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
			pLangCode[i].wLanguage,
			pLangCode[i].wCodePage);

		//Retrieve ProductVersion
		if (VerQueryValue(pVerInfo, subblock, (PVOID*)&pBuffer, &uLen) && pBuffer != NULL)
		{
			break;
		}
	}
	return pBuffer != NULL ? (wchar_t*)pBuffer : L"";
}

inline unsigned __int64 GetDiskFreeSpace(std::wstring filePath)
{
	ULARGE_INTEGER   uiFreeBytesAvailableToCaller; 
	ULARGE_INTEGER   uiTotalNumberOfBytes; 
	ULARGE_INTEGER   uiTotalNumberOfFreeBytes; 

	if(GetDiskFreeSpaceEx(filePath.substr(0, filePath.find(L"\\")!=std::wstring::npos?filePath.find(L"\\"):filePath.length()).c_str(),   
		&uiFreeBytesAvailableToCaller, 
		&uiTotalNumberOfBytes, 
		&uiTotalNumberOfFreeBytes)) 
	{
		return (uiFreeBytesAvailableToCaller.QuadPart);
	}

	return 0;
}

//----------------------------------------------------------
inline void OpenURLWithDefaultBrowser(std::wstring url)
{
	::ShellExecute(NULL, L"open", L"iexplore.exe", url.c_str(), NULL, SW_SHOWNORMAL);
}

inline BOOL RunExec(LPCTSTR cmd, LPCTSTR para, int nShow = SW_SHOWNORMAL, DWORD dwMilliseconds = 0)
{
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL; //_T("open");
	ShExecInfo.lpFile = cmd;
	ShExecInfo.lpParameters = para;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = nShow; /*SW_SHOW;*//*SW_HIDE;*//*SW_SHOWNORMAL;*/
	ShExecInfo.hInstApp = NULL;
	BOOL ret = FALSE;
	ret = ::ShellExecuteEx(&ShExecInfo);
	if (ret && ShExecInfo.hProcess != NULL)
	{
		::WaitForSingleObject(ShExecInfo.hProcess, dwMilliseconds);
		::CloseHandle(ShExecInfo.hProcess);
	}
	return ret;
}

inline std::wstring GetRoamingAppDataFolder()
{
    return GetSpecialFolder(FOLDERID_RoamingAppData);
}

inline std::wstring GetFileName(std::wstring const& path)
{
	return path.substr(path.find_last_of(L"/\\") + 1);
}

inline std::wstring GetFileNameNoExt(std::wstring const& path)
{
	auto fileName = GetFileName(path);
	return fileName.substr(0, fileName.find_last_of(L'.'));
}

//-------------------------File Operation---------------------------------
inline LRESULT CopyOrMoveFile(std::wstring wstrSourceName, std::wstring wstTargetDir, int dropEffect)//wstrSourceName: the source file or folder, wstTargetDir: the target folder, dropEffect: copy(DROPEFFECT_COPY) or move(DROPEFFECT_MOVE)
{
	if(wstrSourceName.empty() || wstTargetDir.empty())
	{
		return S_FALSE;
	}

	SHFILEOPSTRUCT sctFileOp = {0};
	sctFileOp.wFunc = (dropEffect == DROPEFFECT_MOVE ? FO_MOVE : FO_COPY);

	sctFileOp.fFlags &= ~(FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI);//有警告框，有进度条和错误提示

	//int iOldPathLen = (int)_tcslen(wstrSourceName.c_str());
	int iOldPathLen = wstrSourceName.length();
	if (iOldPathLen >= MAX_PATH)
	{
		return S_FALSE;
	}
	TCHAR tczOldName[MAX_PATH + 1];
	ZeroMemory(tczOldName, (MAX_PATH+1)*sizeof(TCHAR));
	wcscpy_s(tczOldName, wstrSourceName.c_str());
	tczOldName[iOldPathLen] = '\0';
	tczOldName[iOldPathLen + 1] = '\0';//必须以两个'\0'结尾
	sctFileOp.pFrom = tczOldName;

	//int iNewPathLen = (int)_tcslen(wstTargetDir.c_str());
	int iNewPathLen = wstTargetDir.length();
	if (iNewPathLen >= MAX_PATH)
	{
		return S_FALSE;
	}
	TCHAR tczNewName[MAX_PATH + 1];
	ZeroMemory(tczNewName, (MAX_PATH + 1)*sizeof(TCHAR));
	wcscpy_s(tczNewName, wstTargetDir.c_str());
	tczNewName[iNewPathLen] = '\0';
	tczNewName[iNewPathLen + 1] = '\0';//必须以两个'\0'结尾
	sctFileOp.pTo = tczNewName;

	if(0 == SHFileOperation(&sctFileOp)) 
	{
		return S_OK;
	}

	if(sctFileOp.hNameMappings)
	{
		SHFreeNameMappings(sctFileOp.hNameMappings);
	}

	return S_FALSE;
}

inline LRESULT PasteFromClipboard(HWND hWnd)
{
	if(hWnd == NULL)
	{
		return S_FALSE;
	}

	UINT uDropEffect = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);//必须先注册CFSTR_PREFERREDDROPEFFE format identifier set to DROPEFFECT_MOVE
	if(!IsClipboardFormatAvailable(uDropEffect)) 
	{
        return S_FALSE; 
	}

	if(!OpenClipboard(hWnd)) 
	{
		return S_FALSE; 
	}

	HDROP hGlobal = HDROP(GetClipboardData(CF_HDROP));
	if(hGlobal) 
	{
        HDROP hDrop = (HDROP)GlobalLock(hGlobal);
		if(hDrop)
		{
			DWORD* dWord = (DWORD*)(GetClipboardData(uDropEffect));
			DWORD dwEffect = *dWord;

			UINT fileCount = DragQueryFile(hDrop, (UINT)-1, NULL, 0);//查看有多少File要Copy或Move
			TCHAR szFile[MAX_PATH];

			for(UINT count = 0; count < fileCount; count++) 
			{
				DragQueryFile(hDrop, count, szFile, sizeof(szFile));
        
				if((dwEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE) 
				{
					CopyOrMoveFile(szFile, GetDesktopDir().append(L"\\"), DROPEFFECT_MOVE);
				} 
				else if((dwEffect & DROPEFFECT_COPY) == DROPEFFECT_COPY) 
				{
					CopyOrMoveFile(szFile, GetDesktopDir().append(L"\\"), DROPEFFECT_COPY);
				}
			}

			CloseClipboard();
		}
		GlobalUnlock(hGlobal);
	}  

	return S_OK;
}

inline std::wstring QuotePathIfNeeded(std::wstring const& path)
{
    size_t newSize = path.length() + 1 /* null */ + 2 /* quotes */;
    std::unique_ptr<TCHAR[]> newPath (new TCHAR[newSize]);
    //wcscpy(newPath.get(), path.c_str());
    ENSURE(wcscpy_s(newPath.get(), newSize, path.c_str()) == 0);
	::PathQuoteSpaces(newPath.get());

    return newPath.get();
}

//exe
inline void LaunchApp(std::wstring const& workingDir, std::wstring const& exeFullPath, std::wstring const& params = std::wstring())
{
	UNREFERENCED_PARAMETER(workingDir);
    std::wstring cmdLine = QuotePathIfNeeded(exeFullPath);
    if(!params.empty())
    {
        cmdLine += L" ";
        cmdLine += params;
    }

    std::unique_ptr<wchar_t[]> cmdLine2 (new wchar_t[cmdLine.length() + 1]);
    //wcscpy(cmdLine2.get(), cmdLine.c_str());
    ENSURE(wcscpy_s(cmdLine2.get(), cmdLine.length() + 1, cmdLine.c_str()) == 0);

    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFO);
    PROCESS_INFORMATION pi = {0};
    ::CreateProcess(NULL, cmdLine2.get(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

inline std::vector<std::wstring> ParseCmdLineArgs(LPCTSTR cmdLine)
{
    LPWSTR* argList;
    int nArgs;

    argList = CommandLineToArgvW(cmdLine, &nArgs);
    ON_SCOPE_EXIT([&] { LocalFree(argList); });

    std::vector<std::wstring> args;
    for(int i = 0; i < nArgs; ++i)
    {
        args.push_back(argList[i]);
    }

    return args;
}

//point and rect of window
inline POINT LeftTop(RECT rc)
{
    POINT pt = {rc.left, rc.top};
    return pt;
}

inline POINT RightTop(RECT rc)
{
    POINT pt = {rc.right, rc.top};
    return pt;
}

inline RECT GetWindowRect(HWND hWnd)
{
    RECT rc;
    ENSURE(GetWindowRect(hWnd, &rc) != 0)((DWORD)hWnd);
    return rc;
}

inline bool Contains(RECT rc1, RECT rc2)
{
	return rc1.left   <= rc2.left 
		&& rc1.top	  <= rc2.top
		&& rc1.right  >= rc2.right 
		&& rc1.bottom >= rc2.bottom;
}

inline MONITORINFO GetMonitorInfo(HMONITOR hMon)
{
    MONITORINFO monInfo = { 0 };
    monInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMon, &monInfo);

    return monInfo;
}
