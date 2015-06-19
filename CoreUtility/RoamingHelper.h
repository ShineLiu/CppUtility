#pragma once

#include <string>
#include <map>
#include <time.h>
#include "singleton.h"
#include "registry_utility.h"

//roaming operation return status
#define ERR_SUCCESS 0
#define ERR_NETWORK_FAULT 5001
#define ERR_AUTHORIZATION_FAULT 5002
#define ERR_EXPIRED 5003
#define ERR_PARAMETER_FAULT 5004
#define ERR_DONOT_LOGIN 5005
#define ERR_GET_USE_INFO_FAULT 5006
#define ERR_SKYDRIVE_FILE_FAULT 5007
#define ERR_LOCAL_FILE_ERROR 5008
#define ERR_ACCESS_TOKEN_FAULT 5009
#define ERR_INTERNET_UNAVAILABLE 5010
#define ERR_SKYDRIVE_SETTINGS_NOT_EXIST 5011
#define ERR_MUTEX 5079
#define ERR_MERGE_FAILED 5081
#define ERR_RUN_KLCONSOLE_FAILED 5090
#define ERR_OHTERS 5100

//Account status
#define ST_NONE 0
#define ST_LOGIN 1
#define ST_LOGOUT 2
#define ST_EXPIRED 3

//if the function returns !ERR_SUCCESS, the call function of this macros returns
#define CK_RET_RETURN(exp) \
{ \
	int __ret = exp; \
	if(__ret != ERR_SUCCESS) return __ret; \
}

//check the parameter, if true return the value
#define CK_EXP_RETURN(exp, ret) \
{ \
	if(exp) return ret; \
}

wchar_t const* const AuthorizeServer = L"login.live.com";
wchar_t const* const AccessServer = L"oauth.live.com";
wchar_t const* const APIServer = L"apis.live.net";
wchar_t const* const RedirectURL = L"https%3A%2F%2Flogin.live.com%2Foauth20_desktop.srf";
wchar_t const* const ClientID = L"0000000040137AC2";
wchar_t const* const ClientScret = L"YXGZSEwwLoK8tQEbusOHqHUekIghjN8Q";

struct UserAccountInfo
{
	int accountStatus;
    std::wstring userName;
    std::wstring userID;	
	std::wstring accessToken;
	std::wstring refreshToken;	
	std::wstring skydriveFolderID;	
	std::map<std::wstring, std::wstring> skydriveIDs;
	time_t lastRefreshTime;
};

typedef struct _SkydriveFileInfo
{
	std::wstring id;
	std::wstring name;
	time_t updated_time;
}SkydriveFileInfo;

typedef struct _LocalFileInfo
{
	std::wstring name;
	time_t updated_time;
}LocalFileInfo;

inline std::wstring GetLiveAccountRegPath()
{
    return std::wstring(L"Software\\Microsoft\\BingWallPaper\\LiveAccount\\");
}

inline std::wstring ReadLiveAccountEntry(std::wstring const& key)
{
    std::wstring value;
    auto ret = ReadRegKey(GetLiveAccountRegPath(), key, value);
	if(ret != ERROR_SUCCESS)
		return L"";
    return value;
}

inline int SetLiveAccountEntry(std::wstring const& key, std::wstring const& value)
{
    return WriteRegKey(GetLiveAccountRegPath(), key, value);
}

inline int ReadLiveAccountINTEntry(std::wstring const& key)
{
    int value;
    auto ret = ReadRegKey(GetLiveAccountRegPath(), key, value);
	if(ret != ERROR_SUCCESS)
		return 0;
    return value;
}

inline void SetLiveAccountEntry(std::wstring const& key, int const& value)
{
   auto ret = WriteRegKey(GetLiveAccountRegPath(), key, value);
   ENSURE(ret == ERROR_SUCCESS)(ret);
}


class RoamingHelper : public Singleton<RoamingHelper>
{
	friend class Singleton<RoamingHelper>;

public: 
	static std::wstring GetLoginURL()
	{
		std::wstringstream url;
		url	<< L"https://" << AuthorizeServer
			<< L"/oauth20_authorize.srf?mkt=zh-cn&"
			<< L"client_id=" << ClientID << L"&"
			<< L"scope=wl.basic%20wl.offline_access%20wl.skydrive_update" << L"&"
			<< L"response_type=code" << L"&"
			<< L"redirect_uri=" << RedirectURL;
		return url.str();
	}

	static std::wstring GetLogoutURL()
	{
		std::wstringstream url;
		url	<< L"https://" << AuthorizeServer
			<< L"/oauth20_logout.srf?"
			<< L"client_id=" << ClientID << L"&"
			<< L"redirect_uri=" << RedirectURL;
		return url.str();
	}

	static std::wstring GetErrorInfo(int errorCode)
	{
		switch (errorCode)
		{
		case ERR_NETWORK_FAULT: 
			return L"网络连接错误";
		case ERR_INTERNET_UNAVAILABLE: 
			return L"无法连接到互联网";
		case ERR_AUTHORIZATION_FAULT: 
			return L"账户认证失败";
		case ERR_EXPIRED: 
			return L"账户认证过期";
		case ERR_GET_USE_INFO_FAULT: 
			return L"读用户信息失败";
		case ERR_SKYDRIVE_FILE_FAULT: 
			return L"访问SkyDrive失败";
		case ERR_LOCAL_FILE_ERROR: 
			return L"打开文件失败";
		case ERR_MERGE_FAILED: 
			return L"合并词库失败";	
		case ERR_SKYDRIVE_SETTINGS_NOT_EXIST:
			return L"在SkyDrive上没有找到你的配置信息。你可先上传配置信息。";
		default:
			break;
		}
		//Todo: if (is_klconsole_error(errorCode)) return get_klconsole_error_message(errorCode);
		return L"未知错误";
	}

public:
	/*
		this function just get the latest user info from skydrive, 
		do not sync setting or Lexicons
	*/
	int Signin(const std::wstring& authorizationToken);

	void Signout();

	/*
		refresh the access token
		return 0 success, otherwise failed.
	*/
	int RefreshAccessToken();

	/*
		upload the latest user settings to skydrive, 
		if bNeedUpdateTokens is true load the the latest refresh token from the regist table before upload the settings
		return 0 success, otherwise failed.
	*/

	int UploadSettings(const std::map<std::wstring, std::wstring>& allSettings, bool bNeedUpdateTokens = true);
	
	/*
		download the latest user settings from skydrive,
		if bNeedUpdateTokens is true load the the latest refresh token from the regist table before download the settings
		return 0 success, otherwise failed.
	*/
	int DownloadSettings(std::map<std::wstring, std::wstring>& settings , bool bNeedUpdateTokens = true);

	/*
		sync the settings and Lexicons automatically, called by the BingIMEPlatform.exe
		return 0: successed; otherwise: failed
	*/
	int AutoSync();

	/*
		sync ime data, called by the configuration panel or background updater
		return 0: successed; otherwise: failed
	*/
	int SyncIMEData();
	
	/*
		user info including user name, account status and so on
	*/
	UserAccountInfo GetUserAccountInfo() { return userAccount_;}	

	/*
		link of user profile photo
	*/
	std::wstring GetUserPhotoLink() 
	{ 
		if(!userAccount_.userID.empty())
		{
			std::wstringstream url;
			url	<< L"https://apis.live.net/v5.0/" << userAccount_.userID << L"/picture";
			return url.str();
		}
		else
		{
			return L"";
		}
	}

	/*
		call by BingIMEPlatform.exe
	*/
	bool IsUserLogin();

private:
	/*
		upload the latest user Lexicons to skydrive, or
		download them from skydrive, basing on the last modify time

		return 0: skydrive file and local file are the same
		1: local file is newer, update the skydrive file
		2: local file is older, download file from skydrive
		<0: error happens
	*/
	int SyncLexicons();
	/*
		if there are a uid in skydrive, download it. Otherwise upload it. 

		return 0: upload uid
		1: download uid
		<0: error happens
	*/
	int SyncUid();

	//int UploadFile(const std::vector<BYTE>& Data, std::wstring remoteFilePath);
	int UploadFile(const std::wstring& localFilePath, const std::wstring& folderID, const std::wstring& remoteFilePath, std::wstring& skydriveFileID);
	//int Download(std::wstring localFilePath, std::wstring remoteFileID);
	int DownloadFile(const std::wstring& localFilePath, const std::wstring& remoteFileID);
	int MergeLexicons(const std::wstring& remoteLexiconPath, const std::wstring& newLexiconPath);
	/*
		create a folder is in user's SkyDrive
		return folder id if successes, otherwise return ""
	*/
	std::wstring CreateFolderInSkydrive(const std::wstring& parentFolder, const std::wstring& folderName);
	std::wstring MoveFolderInSkydrive(const std::wstring& srcFolder, const std::wstring& destFolder);
	std::wstring DeleteFolderInSkydrive(const std::wstring& folderId);
	std::wstring UpdatePropertyInSkydrive(const std::wstring& folderId, const std::wstring& propertyName, const std::wstring& propertyValue);

	int SyncImportedLexiconsFolder();
	int SyncFolder(const std::wstring& localPath, const std::wstring& skydriveFolderID, bool &isUpdatedLocalData);
	int SyncIMEData(const std::wstring& localDataFolderName, const std::wstring& exportDataType, const std::wstring& importDataType);

	/*
	return 0: the two files are the same
	1: the local file is newer
	-1: the skydrive file is newer
	*/
	int CompairLastModifyTime(const std::wstring& localFile, const std::wstring remoteFile);

	/*
		create/update an file in skydrive folder, with the content
		return the file id if successes, otherwise return ""
	*/
	std::wstring UpdateFile(const std::wstring& fileName, const std::wstring& folderID,  const std::wstring& content);
	std::wstring UpdateFile(const std::wstring& fileName, const std::wstring& folderID,  const std::vector<BYTE>& data);

	int GetSkyDriveUserInfo();
	int InitializeSkyDrive();//perpare the folder and files on skydrive
	int CheckAccountStatus();

	void LoadLiveAccountInfo();
	void SaveLiveAccountInfo();
	bool ParseJSON(const std::wstring& json, std::map<std::wstring, std::wstring>& pairs);
	std::wstring ParseIDinJSON(const std::wstring& json, const std::wstring itemName);
	RoamingHelper()	{ userAccount_.lastRefreshTime = 0; LoadLiveAccountInfo(); }
	std::vector<SkydriveFileInfo> ParseFileInJSON(const std::wstring& json);
	std::vector<LocalFileInfo> GetLocalFiles(const std::wstring& folderPath);
	int RunKlconsole(const std::wstring& cmd);
	bool SetSkydriveFileLastModifyTime(const std::wstring& fileID, time_t* modifyTime);
private:
	UserAccountInfo userAccount_;
};
