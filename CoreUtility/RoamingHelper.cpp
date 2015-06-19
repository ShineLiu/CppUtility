#include "stdafx.h"
#include <vector>
#include <sstream> 
#include <Windows.h>
#include <wininet.h>
#include <string>
#include <time.h>
#include <io.h> 
#include "http.h"
#include "RoamingHelper.h"
#include "win32_utility.h"
#include "scope_guard.h"
#include "error_handling_utility.h"
#include <regex>
#include <ShTypes.h>
#include <WinCrypt.h>
#pragma comment (lib, "crypt32.lib")

using namespace std;

namespace 
{
    const std::wstring IsRefreshTokenEncrypted = L"IsRefreshTokenEncrypted";
    const std::wstring RefreshToken            = L"RefreshToken";

    //roaming files
    const std::wstring LexiconFileName            = L"LexiconRoaming.dat";
    const std::wstring SettingFileName            = L"SettingRoaming.dat";
    //roaming folder
    const std::wstring ImportLexiconFolder        = L"ImportedLexicons";
    const std::wstring UDPFolder	              = L"UDP";
    const std::wstring URLFolder				  = L"URL";
    const std::wstring DoublePinyinSchemaFolder   = L"DoublePinyinSchema";
}

//convert timet_t in utc to filetime
void timet_to_filetime(FILETIME&ft, const time_t& utctime)
{
    LONGLONG ll;
    ll = Int32x32To64(utctime, 10000000) + 116444736000000000;
    ft.dwLowDateTime = (DWORD)ll;
    ft.dwHighDateTime = ll >> 32;
}
//convert filetime in utc to timet_t
time_t filetime_to_timet(const FILETIME& ft)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    time_t tm = ull.QuadPart / 10000000ULL - 11644473600ULL;
    return tm;
}

//returns the utc timezone offset
int get_utc_offset() {

    time_t zero = 24*60*60L;
    struct tm  tm;
    int gmtime_hours;

    /* get the local time for Jan 2, 1900 00:00 UTC */
    localtime_s(&tm, &zero );
    gmtime_hours = tm.tm_hour;

    /* if the local time is the "day before" the UTC, subtract 24 hours
    from the hours to get the UTC offset */
    if( tm.tm_mday < 2 )
        gmtime_hours -= 24;

    return gmtime_hours;
}

//utc version of mktime
//convert utc struct tm to time_t
time_t mktime_utc( struct tm * timeptr ) {

    /* gets the epoch time relative to the local time zone,
    and then adds the appropriate number of seconds to make it UTC */
    return mktime( timeptr ) + get_utc_offset() * 3600;
}

inline std::wstring GetSkyDriveRoamingDataFolder()
{
    return GetRoamingAppDataFolder() + L"\\BingWallPaper\\";
}

int RoamingHelper::Signin(const std::wstring& authorizationToken)
{	
	// url(L"oauth20_token.srf?client_id={0}&redirect_uri={1}&client_secret={2}&code={3}&grant_type=authorization_code",ClientID, RedirectURL, ClientScret, authorizationToken);
	std::wstringstream path;
	path << L"oauth20_token.srf?client_id=" << ClientID
		<< L"&redirect_uri=" << RedirectURL
		<< L"&client_secret=" << ClientScret
		<< L"&code=" << authorizationToken
		<< L"&grant_type=authorization_code";
    std::wstring response;
    try
    {
        response = HttpsRequest(AuthorizeServer, INTERNET_DEFAULT_HTTPS_PORT, path.str(), L"GET", L"", L"");		
    }catch(...)
	{
		return ERR_NETWORK_FAULT;
	}

    std::map<std::wstring, std::wstring> pairs;
    if(!ParseJSON(response, pairs) || pairs[L"access_token"].empty() || pairs[L"refresh_token"].empty()) 
    {
		return ERR_ACCESS_TOKEN_FAULT;
	}

    userAccount_.accessToken = pairs[L"access_token"];
    userAccount_.refreshToken = pairs[L"refresh_token"];
    userAccount_.accountStatus = ST_LOGIN;

    //get user name
    GetSkyDriveUserInfo();
    //prepare the skydrive folder and files
    InitializeSkyDrive();
    SaveLiveAccountInfo();
    return ERR_SUCCESS;
}

bool RoamingHelper::IsUserLogin()
{
	return userAccount_.accountStatus == ST_NONE || userAccount_.accountStatus == ST_LOGOUT ? false : true;
}

void RoamingHelper::Signout()
{
    userAccount_.accountStatus = ST_LOGOUT; 
    std::wstringstream path;
	path << L"oauth20_logout.srf?client_id=" << ClientID
	<< L"&redirect_uri=" << RedirectURL;
    std::wstring response;
    try
    {
        response = HttpsRequest(AuthorizeServer, INTERNET_DEFAULT_HTTPS_PORT, path.str(), L"GET", L"", L"");		
    }
	catch(...)
	{
	}
    SaveLiveAccountInfo();
}

//called by the background BingIMEPlatform.exe process periodically
int RoamingHelper::AutoSync()
{
    try
    {
        LoadLiveAccountInfo();//get the latest settings

        if(userAccount_.accountStatus == ST_NONE || userAccount_.accountStatus == ST_LOGOUT)
            return ERR_DONOT_LOGIN;

        //first refresh the access token
        CK_RET_RETURN(RefreshAccessToken());
        CK_RET_RETURN(InitializeSkyDrive());

        /*if(ConfigDataManager::Instance().GetIsRoamingSettings())
        {
            std::map<std::wstring, std::wstring> allSettings;
            CK_RET_RETURN(DownloadSettings(allSettings, false));
            if(!allSettings.empty() && _wtoi64(allSettings[L"LastModifyTime"].c_str()) > _wtoi64(ConfigDataManager::Instance().GetLastModifyTime()))
            {
                ConfigDataManager::Instance().SaveAllSettings(allSettings);		
            }

            return SyncIMEData();
        }*/

        return ERR_SUCCESS;
    }
    catch(...)
    {
        return ERR_OHTERS;
    }	
}

//this method may call from configuration panel or BingIMEPlatform.exe
int RoamingHelper::UploadSettings(const std::map<std::wstring, std::wstring>& allSettings, bool bNeedUpdateTokens)
{	
    if(bNeedUpdateTokens)
        LoadLiveAccountInfo();

    CK_RET_RETURN(CheckAccountStatus());

    std::wstringstream strSettings;
    for(auto it = allSettings.begin(); it != allSettings.end(); ++it)
    {
        strSettings << L"\"" << it->first << L"\": " 
            << L"\"" << it->second << L"\",\r\n";
    }

    return UpdateFile(SettingFileName, userAccount_.skydriveFolderID, strSettings.str()).empty() ? ERR_NETWORK_FAULT : ERR_SUCCESS;
}

//call from the configuration panel or BingIMEPlatform.exe
int RoamingHelper::DownloadSettings(std::map<std::wstring, std::wstring>& settings, bool bNeedUpdateTokens)
{
    if(bNeedUpdateTokens)
        LoadLiveAccountInfo();

    CK_RET_RETURN(CheckAccountStatus());

    //download uid.txt first
    std::wstring path = Format(L"/v5.0/{0}/content?access_token={1}",  userAccount_.skydriveIDs[SettingFileName], userAccount_.accessToken);
    std::wstring response;
    try
    {
        response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", L"");	
        return ParseJSON(response, settings) ? ERR_SUCCESS : ERR_SKYDRIVE_SETTINGS_NOT_EXIST;
    }
    catch(...)	
    {
        return ERR_NETWORK_FAULT;
    }	
}


//sync user typed lexcions, user imported lexcions, UDPs, URLs, and double pinyin schema
//call from the configuration panel or backgroud updater
int RoamingHelper::SyncIMEData()
{
    try
    {
        CK_RET_RETURN(CheckAccountStatus());
        CK_RET_RETURN(SyncImportedLexiconsFolder());
        CK_RET_RETURN(SyncLexicons());

        //sync ime data, udp, url, and double pinyin schema
        CK_RET_RETURN(SyncIMEData(UDPFolder, L"udpdump ", L"udpbuild"));
        CK_RET_RETURN(SyncIMEData(URLFolder, L"urldump ", L"urlbuild"));
        CK_RET_RETURN(SyncIMEData(DoublePinyinSchemaFolder, L"dpsdump ", L"dpsbuild"));
        return ERR_SUCCESS;
    }
    catch(...)	
    {
        return ERR_OHTERS;
    }		
}

//sync udp, url, dps etc
int RoamingHelper::SyncIMEData(const std::wstring& localDataFolderName, const std::wstring& exportDataType, const std::wstring& importDataType)
{/*
    //first, get the latest local data

    std::wstring skyDriveRoamingDataFolder = GetSkyDriveRoamingDataFolder();

    if (skyDriveRoamingDataFolder == L"\\BingWallPaper\\")  // the expected skyDriveRoamingDataFolder is like: "C:\\Users\\youralias\\AppData\\Roaming\\BingWallPaper\\"
    {
        return ERR_LOCAL_FILE_ERROR;
    }

    std::wstring localDataFolderFullPath =  path::combine(skyDriveRoamingDataFolder, localDataFolderName);
    if(!DirectoryExists(localDataFolderFullPath.c_str()))
    {
        CreateDirectoryRecursive(localDataFolderFullPath); 
    }
    //export local data
    std::wstring cmd = Format(L"-cmd {0} -out \"{1}\"", exportDataType, localDataFolderFullPath);
    int ret = RunKlconsole(cmd);
    if(ret != ERR_SUCCESS && ret != KL_URL_ROAMING_ERROR_EMPTY_URL && ret != KL_UDP_ROAMING_ERROR_EMPTY_UDP)
        return ret;

    //then, sync with the skydrive data folder
    bool isUpdated = false;
    CK_RET_RETURN(SyncFolder(localDataFolderFullPath, userAccount_.skydriveIDs[localDataFolderName], isUpdated));

    //finally, update local data
    if(isUpdated)
    {
        cmd = Format(L"-cmd {0} -in \"{1}\"", importDataType, localDataFolderFullPath);
        ret = RunKlconsole(cmd);
        if(ret != ERR_SUCCESS && ret != KL_URL_ROAMING_ERROR_EMPTY_URL && ret != KL_UDP_ROAMING_ERROR_EMPTY_UDP)
            return ret;
    }
*/
    return ERR_SUCCESS;
}

int RoamingHelper::CheckAccountStatus()
{
    if(userAccount_.accountStatus == ST_NONE || userAccount_.accountStatus == ST_LOGOUT)
        return ERR_DONOT_LOGIN;

    //first refresh the access token
    CK_RET_RETURN(RefreshAccessToken());

    return InitializeSkyDrive();
}

//start a new process to merge the two Lexicons
int RoamingHelper::MergeLexicons(const std::wstring& remoteLexiconPath, const std::wstring& newLexiconPath)
{/*
    using namespace BingWallPaper::diagnostics;
    try
    {
        process_start_info psi;
        psi.arguments = Format(L"-cmd roaming -in \"{0}\" -out \"{1}\"", remoteLexiconPath, newLexiconPath);
        psi.file_name = path::combine(Settings::Instance().GetInstallDir(), L"klconsole.exe");
        psi.window_style = process_window_style::pws_hidden;

        auto p = process::start(psi);
        CK_EXP_RETURN((!p), ERR_MERGE_FAILED);

        p.get_value().wait_for_exit();
        return p.get_value().get_exit_code(false);        
    }
	catch(...)
	{
		return ERR_MERGE_FAILED;
	}
	*/
	return ERR_SUCCESS;
}

int RoamingHelper::SyncLexicons()
{/*
    //download the remote user Lexicon first

    std::wstring skyDriveRoamingDataFolder = GetSkyDriveRoamingDataFolder();

    if (skyDriveRoamingDataFolder == L"\\BingWallPaper\\") // the expected skyDriveRoamingDataFolder is like: "C:\\Users\\youralias\\AppData\\Roaming\\BingWallPaper\\"
    {
        return ERR_LOCAL_FILE_ERROR;
    }

    std::wstring localLexiconPath = path::combine(skyDriveRoamingDataFolder, LexiconFileName);
    std::wstring newLexiconlocalPath = path::combine(skyDriveRoamingDataFolder, L"LexiconRoaming_new.dat");
    std::wstring remotePath = LexiconFileName;
    std::wstring skydriveFileID;
    int ret = ERR_SUCCESS;
    try{
        ret = DownloadFile(localLexiconPath, userAccount_.skydriveIDs[LexiconFileName]);
        if( ret == ERR_SUCCESS ) 
        {
            //merge the remote user Lexicon to the local user Lexicon
            ret = MergeLexicons(localLexiconPath, newLexiconlocalPath);
            if(ret == KL_MERGE_SUCCESS)
            {
                //upload the new user Lexicon if needed
                ret = UploadFile(newLexiconlocalPath, userAccount_.skydriveFolderID, remotePath, skydriveFileID);				
            }
            else if( ret == KL_ROAMING_ERROR_NEED_SHOW_MESSAGE)
            {
                ret = UploadFile(newLexiconlocalPath, userAccount_.skydriveFolderID, remotePath, skydriveFileID);
                if(ret == ERR_SUCCESS)// if upload successes, notify the client to show the message, else return the error code of upload operation
                {
                    ret = KL_ROAMING_ERROR_NEED_SHOW_MESSAGE;
                }
            }
            else if(ret == KL_MERGE_ERROR_NEED_NOT_UPLOAD || ret == KL_ROAMING_ERROR_NEED_NOT_UPLOAD)//the remote Lexicon is the latest one, do not need upload it
            {
                ret = ERR_SUCCESS;
            }
        }
    }
	catch(...)
	{
		ret = ERR_MERGE_FAILED;
	}

    return ret;*/
	return ERR_SUCCESS;
}

/*
return 0 if succeed
else, error code
*/
int RoamingHelper::SyncImportedLexiconsFolder()
{/*
    int ret = ERR_SUCCESS;
    std::wstring localPath = ConfigDataManager::GetImportedUserLexiconFolder();
    std::wstring tempPath = GetUserTempFolder();
    std::vector<LocalFileInfo> localFiles = GetLocalFiles(localPath);

    std::wstring path = Format(L"/v5.0/{0}/files?access_token={1}",  userAccount_.skydriveIDs[ImportLexiconFolder], userAccount_.accessToken);
    std::wstring response;
    try
    {
        response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", L"");		
    }catch(...)	{return ERR_NETWORK_FAULT;}
    std::vector<SkydriveFileInfo> skydriveFiles = ParseFileInJSON(response);

    for (std::vector<LocalFileInfo>::iterator iLocalFile = localFiles.begin() ; iLocalFile != localFiles.end() ; iLocalFile++ )
    {
        bool isUpdated = false;
        for (std::vector<SkydriveFileInfo>::iterator iRemoteFile = skydriveFiles.begin() ; iRemoteFile != skydriveFiles.end() ; iRemoteFile++ )
        {
            if((*iLocalFile).name == (*iRemoteFile).name)
            {
                isUpdated = true;
                skydriveFiles.erase(iRemoteFile);
                break;
            }
        }

        if(!isUpdated)//this file is just at local machine, upload it
        {
            std::wstring skydriveFileID;
            std::wstring localLexiconFile = path::combine(localPath, iLocalFile->name); 
            std::wstring tempFile = path::combine(tempPath, iLocalFile->name); 
            std::wstring cmd = Format(L"-cmd ddxdump  -in \"{0}\" -out \"{1}\"", localLexiconFile, tempFile);
            CK_RET_RETURN(RunKlconsole(cmd));
            CK_RET_RETURN(UploadFile(tempFile, userAccount_.skydriveIDs[ImportLexiconFolder], iLocalFile->name, skydriveFileID));
        }
    }

    //download the file only exsites at skydrive
    for (std::vector<SkydriveFileInfo>::iterator iRemoteFile = skydriveFiles.begin() ; iRemoteFile != skydriveFiles.end() ; iRemoteFile++ )
    {
        if(!DDXBadList::Instance().IsInBadList(iRemoteFile->name))
        {
            std::wstring tempFile = path::combine(tempPath, iRemoteFile->name); 
            CK_RET_RETURN(DownloadFile(tempFile, iRemoteFile->id));

            std::wstring targetPath = path::combine(ConfigDataManager::GetImportedUserLexiconFolder(),  path::get_file_name(tempFile));
            std::wstring cmd = Format(L"-cmd ddxbuild -in \"{0}\" -out \"{1}\" -q",	tempFile,targetPath);
            CK_RET_RETURN(RunKlconsole(cmd));

            ConfigDataManager::Instance().EnableUserImportedLexicon(iRemoteFile->name);
        }
    }
    return ret;
	*/
	return ERR_SUCCESS;
}

/*
return 0 sucessed
else, error code
*/
int RoamingHelper::SyncFolder(const std::wstring& localPath, const std::wstring& skydriveFolderID, bool &isUpdatedLocalData)
{/*
    int ret = ERR_SUCCESS;
    if(!DirectoryExists(localPath.c_str()))
    {
        CreateDirectoryRecursive(localPath); 
    }
    std::vector<LocalFileInfo> localFiles = GetLocalFiles(localPath);

    std::wstring path = Format(L"/v5.0/{0}/files?access_token={1}",  skydriveFolderID, userAccount_.accessToken);
    std::wstring response;
    try
    {
        response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", L"");		
    }catch(...)	{return ERR_NETWORK_FAULT;}
    std::vector<SkydriveFileInfo> skydriveFiles = ParseFileInJSON(response);

    std::vector<LocalFileInfo> needUpdateFiles;
    std::vector<SkydriveFileInfo> needDownloadFiles;
    for (std::vector<LocalFileInfo>::iterator iLocalFile = localFiles.begin() ; iLocalFile != localFiles.end() ; iLocalFile++ )
    {
        bool isNewFile = true;
        for (std::vector<SkydriveFileInfo>::iterator iRemoteFile = skydriveFiles.begin() ; iRemoteFile != skydriveFiles.end() ; iRemoteFile++ )
        {
            if((*iLocalFile).name == (*iRemoteFile).name)
            {
                if((*iLocalFile).updated_time > (*iRemoteFile).updated_time) 
                {
                    //local file is latest, upload it					
                    needUpdateFiles.push_back(*iLocalFile);
                }
                else if((*iLocalFile).updated_time < (*iRemoteFile).updated_time)
                {
                    //remote file is latest, download it
                    needDownloadFiles.push_back(*iRemoteFile);
                }

                isNewFile = false;
                skydriveFiles.erase(iRemoteFile);
                break;
            }
        }

        if(isNewFile)//this file is just at local machine, upload it
        {
            needUpdateFiles.push_back(*iLocalFile);
        }
    }

    //download the file only exists at skydrive
    needDownloadFiles.insert(needDownloadFiles.end(), skydriveFiles.begin(), skydriveFiles.end());

    //upload files
    for (std::vector<LocalFileInfo>::iterator iLocalFile = needUpdateFiles.begin() ; iLocalFile != needUpdateFiles.end() ; iLocalFile++ )
    {
        std::wstring localFilePath = path::combine(localPath, iLocalFile->name); 
        std::wstring skydriveFileID;
        CK_RET_RETURN(UploadFile(localFilePath, skydriveFolderID, iLocalFile->name, skydriveFileID));
        //SetSkydriveFileLastModifyTime(skydriveFileID, &(iLocalFile->updated_time));
    }

    //download files
    for (std::vector<SkydriveFileInfo>::iterator iRemoteFile = needDownloadFiles.begin() ; iRemoteFile != needDownloadFiles.end() ; iRemoteFile++ )
    {
        std::wstring localFilePath = path::combine(localPath, iRemoteFile->name);
        CK_RET_RETURN(DownloadFile(localFilePath, iRemoteFile->id));

        //update the last write time of this file
        scope_safe_handle<> handle = ::CreateFile(localFilePath.c_str(),				
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (handle.is_valid())
        {
            FILETIME ft;
            timet_to_filetime(ft, iRemoteFile->updated_time);	
            SetFileTime(handle.get_handle(), NULL, NULL, &ft);
        }		
    }

    //need update the local date after download it from skydrive
    isUpdatedLocalData = !needDownloadFiles.empty();

    return ret;*/
	return ERR_SUCCESS;
}



std::vector<LocalFileInfo> RoamingHelper::GetLocalFiles(const std::wstring& folderPath)
{
    std::vector<LocalFileInfo> localFiles; 
    std::wstring  path = folderPath + L"\\*.*";     

    WIN32_FIND_DATA fileFindData;
    HANDLE hFind = ::FindFirstFile(path.c_str(), &fileFindData);     
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return localFiles;
    }

    do 
    {
        if(!(fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))//type is file
        {
            LocalFileInfo fileInfo;
            fileInfo.name = fileFindData.cFileName;
            fileInfo.updated_time = filetime_to_timet(fileFindData.ftLastWriteTime);
            localFiles.push_back(fileInfo);
        }
    }while (::FindNextFile(hFind, &fileFindData)); 

    FindClose(hFind); 
    return localFiles;
}

int RoamingHelper::RefreshAccessToken()
{
    //only user login or expired user can refresh it's accass token
    if(userAccount_.accountStatus == ST_EXPIRED)
        return ERR_EXPIRED;

    if(userAccount_.accountStatus != ST_LOGIN)
        return ERR_DONOT_LOGIN;
    //
    time_t currentLocalTime = std::time(NULL);
    if(currentLocalTime - userAccount_.lastRefreshTime < 3000)
    {
        //do not need refresh 
        return ERR_SUCCESS;
    }

    //refresh access token cann't execute in multi-processes at the same time
    HANDLE hMutex = CreateMutex(NULL, false, L"EngkooPinyinRoamingMutex");
    if(hMutex == NULL)
    {
        //create mutex failed!
        return ERR_MUTEX;
    }
    ON_SCOPE_EXIT([&] { ReleaseMutex(hMutex); });

    if(WAIT_OBJECT_0 != WaitForSingleObject(hMutex,0))//get it immediatly
    {
        //other process is roaming
        return ERR_MUTEX;
    }

    //enter the ciritical section

    std::wstring path = Format(L"oauth20_token.srf?client_id={0}&redirect_uri={1}&client_secret={2}&refresh_token={3}&grant_type=refresh_token",
        ClientID, RedirectURL, ClientScret,userAccount_.refreshToken);
    std::wstring response;
    try
    {
        response = HttpsRequest(AuthorizeServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"", L"");		
    }catch(...)	{return ERR_NETWORK_FAULT;}

    int ret = ERR_SUCCESS;
    std::map<std::wstring, std::wstring> pairs;
    if(!ParseJSON(response, pairs) || pairs[L"access_token"].empty() || pairs[L"refresh_token"].empty()) 
    {
        userAccount_.accountStatus = ST_EXPIRED;
        ret = ERR_EXPIRED;
    }
    else
    {
        userAccount_.accessToken = pairs[L"access_token"];
        userAccount_.refreshToken = pairs[L"refresh_token"];
        userAccount_.lastRefreshTime = currentLocalTime;
        ret = ERR_SUCCESS;
    }
    SaveLiveAccountInfo();
    return ret;
}

int RoamingHelper::GetSkyDriveUserInfo()
{
    //clear the info firstly
    userAccount_.userName = L"";
    userAccount_.userID = L"";

    std::wstring hostName = APIServer;
	// url(L"v5.0/me?access_token={0}", userAccount_.accessToken)
	std::wstringstream path;
	path << L"v5.0/me?access_token=" << userAccount_.accessToken;
    std::wstring response;
    try
    {
        response = HttpsRequest(hostName, INTERNET_DEFAULT_HTTPS_PORT, path.str(), L"GET", L"", L"");		
    } catch(...)
	{
		return ERR_NETWORK_FAULT;
	}

    std::map<std::wstring, std::wstring> pairs;
    if(response.empty() || !ParseJSON(response, pairs)) 
    {
		return ERR_GET_USE_INFO_FAULT;
	}

    userAccount_.userName = pairs[L"name"];
    userAccount_.userID = pairs[L"id"];
    return ERR_SUCCESS;
}

/*
create a folder in skydrive
return the folder id
*/
std::wstring RoamingHelper::CreateFolderInSkydrive(const std::wstring& parentFolder, const std::wstring& folderName)
{
    //prepare the data	
    std::wstringstream path;
	path << L"/v5.0/" << parentFolder << L"/";
    std::wstringstream headers;
	headers << L"Authorization: Bearer "<< userAccount_.accessToken << L"\r\nContent-Type: application/json\r\n";
    std::wstringstream content;
    content << L"{\r\n\t\"name\": \"" << folderName << L"\",\r\n"
		<< L"\t\"description\": \"A folder for BingIME.\"\r\n}";

    try
    {
        std::wstring response;
        response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path.str(), L"POST", headers.str(), content.str());	
        return ParseIDinJSON(response, folderName);
    }
	catch(...)
	{ 
		return L"";
	}
}

/*
move a folder in skydrive
return the response
*/
std::wstring RoamingHelper::MoveFolderInSkydrive(const std::wstring& srcFolder, const std::wstring& destFolder)
{
	//prepare the data	
	std::wstring path = Format(L"/v5.0/{0}", srcFolder);
	std::wstring headers = Format(L"Authorization: Bearer {0}\r\nContent-Type: application/json\r\n", userAccount_.accessToken);
	std::wstringstream content;
	content << L"{\r\n\t\"destination\": \"" << destFolder << L"\"\r\n}";

	try
	{
		std::wstring response;
		response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"MOVE", headers, content.str());	
		return response;
	}catch(...)	{ return L"";}	
}

std::wstring RoamingHelper::DeleteFolderInSkydrive(const std::wstring& folderId)
{
	//prepare the data	
	std::wstring path = Format(L"/v5.0/{0}?access_token={1}", folderId, userAccount_.accessToken);

	try
	{
		std::wstring response;
		response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"DELETE", L"", L"");	
		return response;
	}catch(...)	{ return L"";}	
}

std::wstring RoamingHelper::UpdatePropertyInSkydrive(const std::wstring& folderId, const std::wstring& propertyName, const std::wstring& propertyValue)
{
	//prepare the data	
	std::wstring path = Format(L"/v5.0/{0}", folderId);
	std::wstring headers = Format(L"Authorization: Bearer {0}\r\nContent-Type: application/json\r\n", userAccount_.accessToken);
	std::wstringstream content;
	content << L"{\r\n\t\""<< propertyName <<"\": \"" << propertyValue << L"\"\r\n}";

	try
	{
		std::wstring response;
		response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"PUT", headers, content.str());	
		return response;
	}catch(...)	{ return L"";}	
}


/*
SetSkydriveFileLastModifyTime
*/
bool RoamingHelper::SetSkydriveFileLastModifyTime(const std::wstring& fileID, time_t* modifyTime)
{
    //covert time_t to ISO 8601
    struct tm tm;
    gmtime_s(&tm, modifyTime);
    std::wstring strTime = Format(L"{0}-{1}-{2}T{3}:{4}:{5}+0000", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    //prepare the data	
    std::wstring path = Format(L"/v5.0/{0}", fileID);
    std::wstring headers = Format(L"Authorization: Bearer {0}\r\nContent-Type: application/json\r\n", userAccount_.accessToken);
    std::wstringstream content;
    content << L"{\r\n\t\"updated_time\": \"" << strTime << L"\"\r\n}";

    try
    {
        std::wstring response;
        response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"PUT", headers, content.str());	
        return true;
    }catch(...)	{ return false;}	
}

std::wstring RoamingHelper::UpdateFile(const std::wstring& fileName, const std::wstring& folderID, const std::wstring& content)
{
    try
    {
        std::wstring path = Format(L"/v5.0/{0}/files/{1}?access_token={2}", folderID, fileName, userAccount_.accessToken);
        std::wstring response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"PUT", L"", content);	
        return ParseIDinJSON(response, fileName);
    }catch(...)	{ return L""; }	 
}

std::wstring RoamingHelper::UpdateFile(const std::wstring& fileName, const std::wstring& folderID,  const std::vector<BYTE>& data)
{
    try
    {
        std::wstring path = Format(L"/v5.0/{0}/files/{1}?access_token={2}", folderID, fileName, userAccount_.accessToken);
        std::vector<BYTE> response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"PUT", L"", data);	

        std::stringstream sstream;
        std::copy(response.begin(), response.end(), std::ostream_iterator<char>(sstream, ""));
        return ParseIDinJSON(UTF8ToUTF16(sstream.str()), fileName);
    }catch(...)	{ return L""; }
}

/*
create the EnkooPinyin folder if not exists and record the folder 
create LexiconRoaming.dat and SettingRoaming.dat in EnkooPinyin folder is not exists and record the fileIDs
*/
int RoamingHelper::InitializeSkyDrive()
{
    //1.ApplicationData folder
	bool bNeedMigrateData = false;
	// url(L"/v5.0/me/skydrive/files?access_token={0}", userAccount_.accessToken)
	std::wstringstream request;
	request << L"/v5.0/me/skydrive/files?access_token=" << userAccount_.accessToken;
	std::wstring response;
    try
    {  
        response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, request.str(), L"GET", L"", L"");		
    } 
	catch(...)
	{ 
		return ERR_NETWORK_FAULT; 
	}

	std::wstring folderId = ParseIDinJSON(response, L"ApplicationData");
	if (folderId.empty())
	{
		folderId = CreateFolderInSkydrive(L"me/skydrive", L"ApplicationData");
		if (folderId.empty())
		{
			return ERR_SKYDRIVE_FILE_FAULT;
		}
	}

	//2.BingIME folder
	std::wstringstream request2;
	request2 << L"/v5.0/" << folderId << L"/files?access_token=" << userAccount_.accessToken;
	std::wstring response2;
	try
	{  
		response2 = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, request2.str(), L"GET", L"", L"");		
	}
	catch(...)
	{ 
		return ERR_NETWORK_FAULT; 
	}

	std::wstring folderId2 = ParseIDinJSON(response2, L"BingWallPaper");
	if (folderId2.empty())
	{
		folderId2 = CreateFolderInSkydrive(folderId, L"BingWallPaper");
		if (folderId2.empty())
		{
			return ERR_SKYDRIVE_FILE_FAULT;
		}
	}

	userAccount_.skydriveFolderID = folderId2;
	
    //3.make sure all roaming files and folders are created on the skydrive	
	std::wstringstream request3;
	request3 << L"/v5.0/" << userAccount_.skydriveFolderID << L"/files?access_token=" << userAccount_.accessToken;
	std::wstring response3;
    try
    {
        response3 = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, request3.str(), L"GET", L"", L"");		
    }
	catch(...)	
	{ 
		return ERR_NETWORK_FAULT; 
	}

	//3-1.create folder ILike to hold pictures
	// Todo:
	/*
    std::vector<std::wstring> skydriveFiles;
    std::vector<std::wstring> skydriveFolders;
    skydriveFiles.push_back(LexiconFileName);
    skydriveFiles.push_back(SettingFileName);

    skydriveFolders.push_back(ImportLexiconFolder);
    skydriveFolders.push_back(UDPFolder);
    skydriveFolders.push_back(URLFolder);
    skydriveFolders.push_back(DoublePinyinSchemaFolder);

    for(auto iterator = skydriveFiles.begin(); iterator < skydriveFiles.end(); iterator++)
    {
        std::wstring id = ParseIDinJSON(response3, *iterator);
        if(id.empty())
        {
            //if not exist, create an empty file
            std::string utf8Content = UTF16ToUTF8(L" ");
            std::vector<BYTE> databuf(utf8Content.begin(), utf8Content.end());	
           // id = UpdateFile(*iterator, userAccount_.skydriveFolderID, Deflate(databuf));	
            if(id.empty())
			{
				return ERR_SKYDRIVE_FILE_FAULT;// create failed
			}
        }

		if (userAccount_.skydriveIDs.find(*iterator) != userAccount_.skydriveIDs.end())
		{
			userAccount_.skydriveIDs[*iterator] = id;
		} 
		else
		{
			userAccount_.skydriveIDs.insert(std::pair<std::wstring, std::wstring>(*iterator, id));
		}
    }

    for(auto iterator = skydriveFolders.begin(); iterator < skydriveFolders.end(); iterator++)
    {
        std::wstring id = ParseIDinJSON(response3, *iterator);
        if(id.empty())
        {
            //if not exist, create it
            id = CreateFolderInSkydrive(userAccount_.skydriveFolderID, *iterator);
            if(id.empty())
			{
				return ERR_SKYDRIVE_FILE_FAULT;// create failed
			}
		}

		if (userAccount_.skydriveIDs.find(*iterator) != userAccount_.skydriveIDs.end())
		{
			userAccount_.skydriveIDs[*iterator] = id;
		} 
		else
		{
			userAccount_.skydriveIDs.insert(std::pair<std::wstring, std::wstring>(*iterator, id));
		}
    }*/
	
    return ERR_SUCCESS;
}

/*
read data from file, deflate it and upload it
*/
int RoamingHelper::UploadFile(const std::wstring& localFilePath, const std::wstring& folderID, const std::wstring& remoteFilePath, std::wstring& skydriveFileID)
{/*
    FILE * fp = NULL;
    errno_t err = _wfopen_s(&fp, localFilePath.c_str(), L"rb" );
    ENSURE(err == 0);
    CK_EXP_RETURN(fp == NULL, ERR_OHTERS);
    ON_SCOPE_EXIT([&] { fclose(fp); });

    //get file length
    if(_fseeki64(fp, 0, SEEK_END)) 
        return ERR_LOCAL_FILE_ERROR;
    DWORD flength = (DWORD)_ftelli64(fp);
    //read the file to memory
    if(_fseeki64(fp, 0, SEEK_SET) != 0)
        return ERR_LOCAL_FILE_ERROR;
    std::vector<BYTE> data(flength);
    DWORD dwRead = fread(&data[0], sizeof(BYTE), flength, fp);	
    if( dwRead <= 0) 
        return  ERR_LOCAL_FILE_ERROR;	
    //deflate the data
    data.resize(dwRead);		
    //return UploadFile( Deflate(data), remoteFilePath);
    skydriveFileID = UpdateFile(remoteFilePath, folderID, Deflate(data));
    return skydriveFileID.empty()? ERR_NETWORK_FAULT: ERR_SUCCESS;	
	*/
	return ERR_SUCCESS;
}

int RoamingHelper::DownloadFile(const std::wstring& localFilePath, const std::wstring& remoteFileID)
{/*
    try
    {
        std::vector<BYTE> content;
        std::wstring path = Format(L"/v5.0/{0}/content?access_token={1}",remoteFileID, userAccount_.accessToken);
        std::vector<BYTE> response = HttpsRequest(APIServer, INTERNET_DEFAULT_HTTPS_PORT, path, L"GET", L"",content);	

        //inflate the data
        if(response.size() > 0)
        {
            std::vector<BYTE> inflateData = Inflate(response);
            FILE * fp = NULL;
            errno_t err = _wfopen_s(&fp, localFilePath.c_str(), L"wb" );	
            ENSURE(err == 0)(localFilePath);
            if(fp == NULL) return ERR_LOCAL_FILE_ERROR;
            if(inflateData.size() > 0)
                fwrite((LPVOID)&inflateData[0], sizeof(BYTE), inflateData.size(), fp);
            fclose(fp);
        }
    }catch(...)	{ return ERR_NETWORK_FAULT;}
	*/
    return ERR_SUCCESS;
}


void SaveRefreshToken(std::wstring const& token)
{
    DATA_BLOB DataIn;
    DataIn.pbData = (BYTE*)token.c_str();    
    DataIn.cbData = (DWORD)(token.length() + 1) * sizeof(wchar_t);

    DATA_BLOB DataOut;

    if(CryptProtectData(&DataIn, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &DataOut))
    {
        ON_SCOPE_EXIT([&] { LocalFree(DataOut.pbData); });

        ENSURE(ERROR_SUCCESS == WriteRegKey(GetLiveAccountRegPath(), IsRefreshTokenEncrypted, 1));
        ENSURE(ERROR_SUCCESS == WriteRegKey(GetLiveAccountRegPath(), RefreshToken, 
        BinDataBlob(DataOut.cbData, DataOut.pbData)));
    }
    else
    {
        ENSURE(ERROR_SUCCESS == WriteRegKey(GetLiveAccountRegPath(), RefreshToken, 0));
        SetLiveAccountEntry(RefreshToken, token);
    }
}

std::wstring LoadRefreshToken()
{
    DWORD isEncrypted = 0;
    ReadRegKey(GetLiveAccountRegPath(), IsRefreshTokenEncrypted, isEncrypted);
    if(isEncrypted)
    {
        RegValue blob;
        if(ERROR_SUCCESS == ReadRegKey(GetLiveAccountRegPath(), RefreshToken, REG_BINARY, &blob))
        {
            DATA_BLOB dataIn;
            dataIn.cbData = (DWORD)blob.Data.size();
            dataIn.pbData = blob.Data.data();

            DATA_BLOB dataOut;

            if(CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut))
            {
                ON_SCOPE_EXIT([&] { LocalFree(dataOut.pbData); });
                return (wchar_t*)dataOut.pbData;
            }
        }
    }
    else
    {
        return ReadLiveAccountEntry(RefreshToken);
    }
    return L"";
}

void RoamingHelper::SaveLiveAccountInfo()
{
    SetLiveAccountEntry(L"LiveAccountStatus",userAccount_.accountStatus);
    SetLiveAccountEntry(L"UserName", userAccount_.userName);
    SetLiveAccountEntry(L"UserID", userAccount_.userID);
    SaveRefreshToken(userAccount_.refreshToken);
}

void RoamingHelper::LoadLiveAccountInfo()
{
    userAccount_.accountStatus = ReadLiveAccountINTEntry(L"LiveAccountStatus");
    userAccount_.userName = ReadLiveAccountEntry(L"UserName");
    userAccount_.userID = ReadLiveAccountEntry(L"UserID");
    userAccount_.refreshToken = LoadRefreshToken();
}

bool RoamingHelper::ParseJSON(const std::wstring& json, std::map<std::wstring, std::wstring>& pairs)
{
    pairs.clear();
    std::wstring input = json;
    std::wregex formCodeRX(L"\"([a-zA-Z_]*?)\": *\"(.*?)\"([,|}])");
    std::wsmatch match;
    while(std::regex_search(input, match, formCodeRX))
    {
        pairs[match[1].str()] =  match[2].str();
        input = match.suffix().str();
    }

    return !pairs.empty();
}

std::wstring RoamingHelper::ParseIDinJSON(const std::wstring& json, const std::wstring itemName)
{
    std::wstring id;
    std::wstring input = json;
    std::wregex formCodeRX(L"\"([a-zA-Z_]*?)\": *\"(.*?)\"([,|}])");
    std::wsmatch match;
    while(std::regex_search(input, match, formCodeRX))
    {
        if(match[1].str() == L"id" && (match[2].str().compare(0, 4, L"file") == 0 || match[2].str().compare(0, 6, L"folder") == 0))
            id = match[2].str();
        if(match[1].str() == L"name" && match[2].str() == itemName)
            return id;

        input = match.suffix().str();
    }

    return L"";
}

std::vector<SkydriveFileInfo> RoamingHelper::ParseFileInJSON(const std::wstring& json)
{
    vector<SkydriveFileInfo> files;
    SkydriveFileInfo fileInfo;
    std::wstring id;
    std::wstring input = json;
    std::wregex formCodeRX(L"\"([a-zA-Z_]*?)\": *\"(.*?)\"([,|}|\\r])");
    std::wsmatch match;

    while(std::regex_search(input, match, formCodeRX))
    {
        if(match[1].str() == L"id" && match[2].str().compare(0, 4, L"file") == 0)
            fileInfo.id = match[2].str();
        if(match[1].str() == L"name")
            fileInfo.name =  match[2].str();
        if(match[1].str() == L"updated_time")
        {
            //parse the skydrive file last modify time
            //example "updated_time": "2011-10-12T23:18:23+0000"
            struct tm timeSkydrive;
            timeSkydrive.tm_isdst = 0;
            swscanf_s(match[2].str().c_str(),L"%4d-%2d-%2dT%2d:%2d:%2d", &timeSkydrive.tm_year,&timeSkydrive.tm_mon,&timeSkydrive.tm_mday,
                &timeSkydrive.tm_hour,&timeSkydrive.tm_min,&timeSkydrive.tm_sec);
            timeSkydrive.tm_mon--;
            timeSkydrive.tm_year-=1900;
            //utc time
            fileInfo.updated_time = mktime_utc(&timeSkydrive);
            if(!fileInfo.id.empty() && !fileInfo.name.empty())
            {
                files.push_back(fileInfo);
                fileInfo.id = L"";
                fileInfo.name = L"";
            }
        }

        input = match.suffix().str();
    }

    return files;
}

int RoamingHelper::RunKlconsole(const std::wstring& cmd)
{/*
    using namespace BingWallPaper::diagnostics;
    try
    {
        process_start_info psi;
        psi.arguments = cmd;
        psi.file_name = path::combine(Settings::Instance().GetInstallDir(), L"klconsole.exe");
        psi.window_style = process_window_style::pws_hidden;

        auto p = process::start(psi);
        if (!p)
        {
            return ERR_RUN_KLCONSOLE_FAILED;
        }
        else
        {
            p.get_value().wait_for_exit();
            return p.get_value().get_exit_code(false);        
        }
    }catch(...){return ERR_RUN_KLCONSOLE_FAILED;}
	*/
	return ERR_SUCCESS;
}