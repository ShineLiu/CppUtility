#pragma once

#include <Windows.h>
#define STRSAFE_NO_DEPRECATE 
#include <type_traits>
#include "scope_guard.h"
#include <tchar.h>
#include <strsafe.h>

struct RegValue
{
    DWORD             Type;
    std::vector<BYTE> Data;
};

// Reads 64bits registry key even in WOW64.
inline LONG ReadRegKey(HKEY hKey, LPCTSTR subKey, LPCTSTR keyName, _Out_ RegValue* pValue)
{
    HKEY hk;
    LONG flag = RegOpenKeyEx(hKey, subKey, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hk);
    if(flag != ERROR_SUCCESS)
    {
        return flag;
    }
    ON_SCOPE_EXIT([&] { RegCloseKey(hk); });

    DWORD type;
    DWORD size;
    flag = RegQueryValueEx(hk, keyName, NULL, &type, NULL, &size);
    if(flag != ERROR_SUCCESS)
    {
        return flag;
    }

    pValue->Type = type;
    pValue->Data.resize(size);

	return RegQueryValueEx(hk, keyName, NULL, NULL, pValue->Data.data(), &size);
}

// Try to read from HKCU first, if not found then HKLM
inline LONG _ReadUSRegKey(LPCTSTR subKey, LPCTSTR keyName, bool isIgnoreHKCU, _Out_ RegValue* pValue) 
{
    if (!isIgnoreHKCU && ReadRegKey(HKEY_CURRENT_USER, subKey, keyName, pValue) == ERROR_SUCCESS) 
    {
        return ERROR_SUCCESS;
    }

    return ReadRegKey(HKEY_LOCAL_MACHINE, subKey, keyName, pValue);
}

inline LONG ReadRegKey(std::wstring const& subKey, std::wstring const& keyName, DWORD requiredType, _Out_ RegValue* pValue, bool ignoreHKCU = false)
{
    auto ret = _ReadUSRegKey(subKey.c_str(), keyName.c_str(), ignoreHKCU, pValue);
    if(ret != ERROR_SUCCESS)
    {
        return ret;
    }
    if(requiredType != pValue->Type)
    {
        return ERROR_INVALID_DATA;
    }
    return ret;
}

struct BinDataBlob
{
    BinDataBlob()
    { }

    BinDataBlob(DWORD size, BYTE* data)
        : Size(size), Data(data)
    { }

    DWORD Size;
    BYTE* Data;
};

template <typename T>
struct RegKeyTypeTraits;

template <>
struct RegKeyTypeTraits<DWORD>
{
    enum { type = REG_DWORD };
};

template <>
struct RegKeyTypeTraits<time_t>
{
    enum { type = REG_QWORD };
};

template <>
struct RegKeyTypeTraits<int>
{
    enum { type = REG_DWORD };
};

template <>
struct RegKeyTypeTraits<long>
{
    enum { type = REG_DWORD };
};

template <int N>
struct RegKeyTypeTraits<WCHAR[N]>
{
    enum { type = REG_SZ };
};

template <int N>
struct RegKeyTypeTraits<WCHAR const[N]>
{
    enum { type = REG_SZ };
};

template <>
struct RegKeyTypeTraits<std::wstring>
{
    enum { type = REG_SZ };
};

template <>
struct RegKeyTypeTraits<const wchar_t*>
{
    enum { type = REG_SZ };
};

template <>
struct RegKeyTypeTraits<BinDataBlob>
{
    enum { type = REG_BINARY };
};

template <typename T>
inline LONG ReadRegKey(std::wstring const& subKey, std::wstring const& keyName, T& data, bool ignoreHKCU = false)
{
    static_assert(!std::is_pointer<T>::value, "incorrect data type");

    RegValue value;
    auto ret = ReadRegKey(subKey, keyName, RegKeyTypeTraits<T>::type, &value, ignoreHKCU);
    if(ret != ERROR_SUCCESS)
    {
        return ret;
    }

    if(value.Data.size() > sizeof(T))
    {
        return ERROR_MORE_DATA;
    }

    CopyMemory(&data, value.Data.data(), value.Data.size());
    return ERROR_SUCCESS;
}

inline LONG ReadRegKey(std::wstring const& subKey, std::wstring const& keyName, std::wstring& data, bool ignoreHKCU = false)
{
    RegValue value;
    auto ret = ReadRegKey(subKey, keyName, RegKeyTypeTraits<std::wstring>::type, &value, ignoreHKCU);
    if(ret != ERROR_SUCCESS)
    {
        return ret;
    }

    data = (WCHAR*)value.Data.data();
    return ERROR_SUCCESS;
}

namespace
{
    const std::wstring BingWallPaperSubKey = L"Software\\Microsoft\\BingWallPaper";
}

template <typename T>
inline LONG ReadBingWallPaperRegKey(std::wstring const& keyName, T& data, bool ignoreHKCU = false)
{
    return ReadRegKey(BingWallPaperSubKey, keyName, data, ignoreHKCU);
}

// Writes key to HKCU
inline LONG WriteRegKey(HKEY hKey, LPCTSTR subKey, LPCTSTR keyName, DWORD dwType, LPCVOID lpData, DWORD cbData) 
{
    HKEY hk;

    LONG flag = RegCreateKeyEx(hKey, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &hk, NULL);
    if (flag != ERROR_SUCCESS) return flag;
    
	flag = RegSetValueEx(hk, keyName, 0, dwType, (CONST BYTE*)lpData, cbData);
    RegCloseKey(hk);

    return flag;
}

inline LONG _WriteUSRegKey(LPCTSTR subKey, LPCTSTR keyName, DWORD dwType, LPCVOID lpData, DWORD cbData) 
{
	return WriteRegKey(HKEY_CURRENT_USER, subKey, keyName, dwType, lpData, cbData);
}

inline BOOL SetRegValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPCTSTR data) {
	HKEY key;
	if (ERROR_SUCCESS != RegCreateKeyEx(hkey, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &key, NULL))
	{
		return FALSE;
	}
	if(ERROR_SUCCESS != RegSetValueEx(key, pszValue, 0, REG_SZ, (BYTE*) data, static_cast<DWORD>((_tcslen(data) + 1) * sizeof(pszValue[0]))))
	{
		RegCloseKey(key);
		return FALSE;
	}
	RegCloseKey(key);
	return TRUE;
}

// remove key from HKCU
inline LONG RemoveRegKey(HKEY hKey, LPCTSTR subKey, LPCTSTR keyName) 
{
    HKEY hk;

    LONG flag = RegOpenKeyEx(hKey, subKey, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &hk);
    if(flag != ERROR_SUCCESS) return flag;

    flag = RegDeleteValue(hk, keyName);
	if(hk != NULL) RegCloseKey(hk);
	return flag;
}

inline LONG _RemoveUSRegKey(LPCTSTR subKey, LPCTSTR keyName) 
{
	return RemoveRegKey(HKEY_CURRENT_USER, subKey, keyName);
}

template <typename T>
inline std::pair<LPCVOID, DWORD> GetDataPtr(T const& data)
{
    static_assert(!std::is_pointer<T>::value, "incorrect data type");
    return std::pair<LPCVOID, DWORD>(&data, static_cast<DWORD>(sizeof(T)));
}

inline std::pair<LPCVOID, DWORD> GetDataPtr(std::wstring const& str)
{
    return std::pair<LPCVOID, DWORD>(str.c_str(), static_cast<DWORD>((str.length() + 1) * sizeof(wchar_t)));
}

inline std::pair<LPCVOID, DWORD> GetDataPtr(wchar_t const* str)
{
    return std::pair<LPCVOID, DWORD>(str, static_cast<DWORD>((wcslen(str) + 1) * sizeof(wchar_t)));
}

inline std::pair<LPCVOID, DWORD> GetDataPtr(BinDataBlob data)
{
    return std::pair<LPCVOID, DWORD>(data.Data, data.Size);
}

template <typename T>
inline LONG WriteRegKey(HKEY hKey, std::wstring const& subKey, std::wstring const& keyName, T const& data)
{
    auto pData = GetDataPtr(data);
    return WriteRegKey(hKey, subKey.c_str(), keyName.c_str(), RegKeyTypeTraits<T>::type, pData.first, pData.second);
}

template <typename T>
inline LONG WriteRegKey(std::wstring const& subKey, std::wstring const& keyName, T const& data)
{
    auto pData = GetDataPtr(data);
    return _WriteUSRegKey(subKey.c_str(), keyName.c_str(), RegKeyTypeTraits<T>::type, pData.first, pData.second);
}

template <typename T>
inline LONG WriteBingWallPaperRegKey(std::wstring const& keyName, T const& data, bool ignoreHKCU = false)
{
	if(ignoreHKCU)
	{
		return WriteRegKey(HKEY_LOCAL_MACHINE, BingWallPaperSubKey, keyName, data);
	}
	return WriteRegKey(BingWallPaperSubKey, keyName, data);
}

inline LONG RemoveRegKey(std::wstring const& subKey, std::wstring const& keyName)
{
    return _RemoveUSRegKey(subKey.c_str(), keyName.c_str());
}
