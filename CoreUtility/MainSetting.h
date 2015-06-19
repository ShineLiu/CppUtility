#pragma once

#include <map>
#include <string>
#include <vector>
#include "Registry.h"
#include "string_utility.h"
#include "win32_utility.h"

class MainSetting
{
public:
	MainSetting();
	~MainSetting();

	bool GetRegBoolValue(LPCTSTR eleId);
	std::wstring GetRegStringValue(LPCTSTR eleId);
	void SetRegBoolValue(LPCTSTR eleId, bool value); 
	void SetRegStringValue(LPCTSTR eleId, LPCTSTR value);

	std::wstring GetFolderViewRegStringValue(LPCTSTR subPath, LPCTSTR eleId);
	void SetFolderViewRegStringValue(LPCTSTR subPath, LPCTSTR eleId, LPCTSTR value);

	bool IsAutoRunEnabled();

	std::wstring GetLocalMachineRegStrValue(LPCTSTR eleId);
private:
	HKEY GetRegKey(); 
	HKEY GetFolderViewRegKey(LPCTSTR subPath); 
	LPCTSTR regPath_;
	LPCTSTR wpRegPath_;
	LPCTSTR autoRunRegPath_;
	std::map<std::wstring, std::wstring> eleIdToRegKey_; 
	std::map<std::wstring, std::wstring> defaultKeyValuePair_; 
};

inline MainSetting::MainSetting()
{
	//elementIdToRegKey is used to map element id to its RegKey counterpart
	//key: html element id
	//value: reg key name
	eleIdToRegKey_[_T("open_desktop_manager")] = _T("OpenDesktopManager");defaultKeyValuePair_[_T("open_desktop_manager")] = _T("true");
	eleIdToRegKey_[_T("Msn123LastLoadSuccessTime")] = _T("Msn123LastLoadSuccessTime");defaultKeyValuePair_[_T("Msn123LastLoadSuccessTime")] = _T("");
	eleIdToRegKey_[_T("show_msn123")] = _T("ShowMsn123");defaultKeyValuePair_[_T("show_msn123")] = _T("true");
	eleIdToRegKey_[_T("enable_dbclick")] = _T("EnableDbclick");defaultKeyValuePair_[_T("enable_dbclick")] = _T("true");
	eleIdToRegKey_[_T("show_folder_view")] = _T("ShowFolderView");defaultKeyValuePair_[_T("show_folder_view")] = _T("true");
	eleIdToRegKey_[_T("auto_play")] = _T("AutoPlay");defaultKeyValuePair_[_T("auto_play")] = _T("false");
	eleIdToRegKey_[_T("interval")] = _T("Interval");defaultKeyValuePair_[_T("interval")] = _T("-2");
	eleIdToRegKey_[_T("category")] = _T("Category");defaultKeyValuePair_[_T("category")] = _T("bing");
	eleIdToRegKey_[_T("open_search_box")] = _T("OpenSearchBox");defaultKeyValuePair_[_T("open_search_box")] = _T("true");
	eleIdToRegKey_[_T("enable_query_history")] = _T("EnableQueryHistory");defaultKeyValuePair_[_T("enable_query_history")] = _T("true");
	eleIdToRegKey_[_T("statusbardlg_position")] = _T("StatusBarDlgPosition");defaultKeyValuePair_[_T("statusbardlg_position")] = _T("fixed_topright");//_T("fixed_topright");//_T("float");
	eleIdToRegKey_[_T("WallPaperFolder")] = _T("WallPaperFolder");defaultKeyValuePair_[_T("WallPaperFolder")] = CombinePath(GetAppDataDir().c_str(),_T("\\BingWallPaper\\WallPapers"),_T(""));

	eleIdToRegKey_[_T("icon_size")] = _T("IconSize");defaultKeyValuePair_[_T("icon_size")] = _T("32");
	eleIdToRegKey_[_T("sort_type")] = _T("SortType");defaultKeyValuePair_[_T("sort_type")] = _T("-1");
	eleIdToRegKey_[_T("version")] = _T("Version");defaultKeyValuePair_[_T("version")] = _T("1.0.0.0");

	regPath_ = _T("Software\\Microsoft\\BingWallPaper\\Config");
	wpRegPath_ = _T("Software\\Microsoft\\BingWallPaper");
	autoRunRegPath_ = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
}


inline MainSetting::~MainSetting()
{
}


//--------------------------------------------------------------
inline HKEY MainSetting::GetRegKey()
{
	HKEY subKey = Registry::OpenSubKey(HKEY_CURRENT_USER, regPath_);
	if (subKey == NULL)
	{
		subKey = Registry::CreateKey(HKEY_CURRENT_USER, regPath_);
	}
	return subKey;
}


inline bool MainSetting::GetRegBoolValue(LPCTSTR  eleId) 
{
	HKEY subKey = GetRegKey();

	bool bRet = false;
	std::wstring result = std::wstring(GetRegStringValue(eleId));
	if (wcscmp(result.c_str(), L"true") == 0)
	{
		bRet = true;
	}
	else
	{
		bRet = false;
	}

	Registry::Release(subKey);

	return bRet;
}


inline std::wstring MainSetting::GetRegStringValue(LPCTSTR eleId)
{
	HKEY subKey = GetRegKey();

	std::wstring result = Registry::GetValueAsString(subKey, eleIdToRegKey_[std::wstring(eleId)].c_str());
	if(result.empty() && defaultKeyValuePair_.find(eleId) != defaultKeyValuePair_.end())
	{
		result = defaultKeyValuePair_[eleId];
	}

	Registry::Release(subKey);

	return result;
}


inline void MainSetting::SetRegBoolValue(LPCTSTR eleId, bool value)
{
	SetRegStringValue(eleId, value?_T("true"):_T("false"));
}


inline void MainSetting::SetRegStringValue(LPCTSTR eleId, LPCTSTR value)
{
	HKEY subKey = GetRegKey();

	Registry::SetValueAsString(subKey, eleIdToRegKey_[std::wstring(eleId)].c_str(), value);

	Registry::Release(subKey);
}

//---------------------------------------------------------------
inline HKEY MainSetting::GetFolderViewRegKey(LPCTSTR subPath)
{
	std::wstring path; 
	path += wpRegPath_; 
	path += subPath;
	HKEY subKey = Registry::OpenSubKey(HKEY_CURRENT_USER, path.c_str());
	if (subKey == NULL)
	{
		subKey = Registry::CreateKey(HKEY_CURRENT_USER, path.c_str());
	}
	return subKey;
}

inline std::wstring MainSetting::GetFolderViewRegStringValue(LPCTSTR subPath, LPCTSTR eleId)
{
	HKEY subKey = GetFolderViewRegKey(subPath);

	std::wstring result = Registry::GetValueAsString(subKey, eleIdToRegKey_[std::wstring(eleId)].c_str());
	if(result.empty() && defaultKeyValuePair_.find(eleId) != defaultKeyValuePair_.end())
	{
		result = defaultKeyValuePair_[eleId];
	}

	Registry::Release(subKey);

	return result;
}

inline void MainSetting::SetFolderViewRegStringValue(LPCTSTR subPath, LPCTSTR eleId, LPCTSTR value)
{
	HKEY subKey = GetFolderViewRegKey(subPath);
	Registry::SetValueAsString(subKey, eleIdToRegKey_[std::wstring(eleId)].c_str(), value);

	Registry::Release(subKey);
}

inline std::wstring MainSetting::GetLocalMachineRegStrValue(LPCTSTR eleId)
{
	HKEY subKey = Registry::OpenSubKey(HKEY_LOCAL_MACHINE, wpRegPath_);
	if (subKey == NULL)
	{
		return L"";
	}

	std::wstring result = Registry::GetValueAsString(subKey, eleIdToRegKey_[std::wstring(eleId)].c_str());
	if(result.empty() && defaultKeyValuePair_.find(eleId) != defaultKeyValuePair_.end())
	{
		result = defaultKeyValuePair_[eleId];
	}

	Registry::Release(subKey);

	return result;
}

//---------------------------------------------------------------
inline bool MainSetting::IsAutoRunEnabled()
{
	HKEY subKey = Registry::OpenLMSubKey(autoRunRegPath_, false);
	if (subKey == NULL)
	{
		return false;
	}

	std::wstring result = Registry::GetValueAsString(subKey, _T("BingWallPaper"));
	Registry::Release(subKey);

	return !result.empty();
}

