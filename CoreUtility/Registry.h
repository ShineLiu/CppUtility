#pragma once

#include <windows.h>
#include <string>

class Registry
{
public:
	static std::wstring GetValueAsString(HKEY Key, LPCWSTR name);
	static bool SetValueAsString(HKEY key,LPCWSTR name, std::wstring value);

	static int GetValueAsInt(HKEY key, LPCWSTR name, int defaultValue = 0);
	static bool SetValueAsInt(HKEY key, LPCWSTR name, int value);

	static LPBYTE GetValueAsBinary(HKEY key, LPCWSTR name);
	static bool SetValueAsBinary(HKEY key, LPCWSTR name, LPBYTE value, DWORD dwCount);

	static bool DeleteValue(HKEY key, LPCWSTR name);

	static HKEY OpenSubKey(HKEY baseKey, LPCWSTR name, DWORD ulOptions = KEY_READ|KEY_WRITE);
	static HKEY OpenLMSubKey(LPCWSTR name, bool needWrite);

	static HKEY CreateKey(HKEY baseKey, LPCWSTR name);

	static void Release(HKEY& key);

	static DWORD GetSubItemCount(HKEY baseKey, LPCWSTR name);
};


inline HKEY Registry::OpenSubKey(HKEY baseKey, LPCWSTR name, DWORD ulOptions)
{
	if(baseKey == NULL)
	{
		return NULL;
	}

	HKEY openedKey = NULL;
	RegOpenKeyEx(baseKey, name, 0, ulOptions, &openedKey);
	return openedKey;
}

inline HKEY Registry::OpenLMSubKey(LPCWSTR name, bool needWrite)
{
	HKEY openedKey = NULL;
	int flag = KEY_READ | (needWrite ? KEY_WRITE : 0);
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, name, 0, flag, &openedKey);
	return openedKey;
}

inline HKEY Registry::CreateKey(HKEY baseKey, LPCWSTR name)
{
	if(baseKey == NULL)
	{
		return NULL;
	}

	HKEY subKey = NULL;
	DWORD dwDisposition;
	RegCreateKeyEx(baseKey,
		name,
		0,
		REG_NONE, REG_OPTION_NON_VOLATILE, 
		KEY_WRITE|KEY_READ, 
		NULL,
		&subKey,
		&dwDisposition);
	return subKey;							
}

inline bool Registry::DeleteValue(HKEY key, LPCWSTR name)
{
	if(key == NULL)
	{
		return false;
	}

	LONG lResult = RegDeleteValue(key, name);
	return lResult == ERROR_SUCCESS;
}

inline bool Registry::SetValueAsString(HKEY key, LPCWSTR name, std::wstring value)
{
	if(key == NULL)
	{
		return false;
	}

	DWORD dwCount = (DWORD)sizeof(WCHAR)*((DWORD)wcslen(value.c_str()) + 1);
	LONG lResult = RegSetValueEx(key, name, 0, REG_SZ, (LPBYTE)value.c_str(), dwCount);
	return lResult == ERROR_SUCCESS;
}

inline std::wstring Registry::GetValueAsString(HKEY key, LPCWSTR name)
{
	if(key == NULL)
	{
		return L"";
	}

	DWORD dwType = REG_SZ;
	DWORD dwCount = 0;

	// get data length
	LONG lResult = RegQueryValueEx(key, name, 0, &dwType, NULL, &dwCount);

	if(lResult == ERROR_SUCCESS)
	{
		std::wstring result;

		result.resize(dwCount);
		lResult = RegQueryValueEx(key, name, 0, &dwType, (LPBYTE)result.data(), &dwCount);

		if(lResult == ERROR_SUCCESS)
		{
			return result;
		}
	}

	return L"";
}

inline int Registry::GetValueAsInt(HKEY key, LPCWSTR name, int defaultValue/* = 0*/)
{
	int result = 0;
	DWORD dwType = REG_DWORD;
	DWORD dwCount = sizeof(DWORD);
	LONG lResult = RegQueryValueEx(key, name, 0, &dwType, (LPBYTE)&result, &dwCount);
	if(lResult == ERROR_SUCCESS)
	{
		return result;
	}
	return defaultValue;
}

inline bool Registry::SetValueAsInt(HKEY key, LPCWSTR name, int value)
{
	if(key == NULL)
	{
		return false;
	}

	LONG lResult = RegSetValueEx(key, name, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
	return lResult == ERROR_SUCCESS;
}

inline LPBYTE Registry::GetValueAsBinary(HKEY key, LPCWSTR name)
{
	if(key == NULL)
	{
		return NULL;
	}

	DWORD dwType = REG_BINARY;
	DWORD dwCount = 0;

	// get data length
	LONG lResult = RegQueryValueEx(key, name, 0, &dwType, NULL, &dwCount);

	if(lResult == ERROR_SUCCESS)
	{
		LPBYTE result;
		result = new BYTE[dwCount];
		lResult = RegQueryValueEx(key, name, 0, &dwType, (LPBYTE)result, &dwCount);

		if(lResult == ERROR_SUCCESS)
		{
			return result;
		}
		else
		{
			delete[] result;
		}	
	}

	return NULL;
}

inline bool Registry::SetValueAsBinary(HKEY key, LPCWSTR name, LPBYTE value, DWORD dwCount)
{
	if(key == NULL)
	{
		return false;
	}
	LONG lResult = RegSetValueEx(key, name, 0, REG_BINARY, value, dwCount);
	return lResult == ERROR_SUCCESS;
}

inline void Registry::Release(HKEY& key)
{
	if(key != NULL)
	{
		RegCloseKey(key);
		key = NULL;
	}
}

inline DWORD Registry::GetSubItemCount(HKEY baseKey, LPCWSTR name)
{
	HKEY     hKey;

	TCHAR    achClass[1024] = _T("");  // buffer for class name 
	DWORD    cchClassName = 1024;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD   retCode;

	hKey = Registry::OpenSubKey(baseKey, name);
	if (hKey == NULL)
	{
		return 0;
	}

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hKey,                    // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 

	Registry::Release(hKey);

	if (retCode != ERROR_SUCCESS)
	{
		return 0;
	}

	return cSubKeys + cValues;
}