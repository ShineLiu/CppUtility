#include "stdafx.h"
#include "GenerateUserID.h"
#include <time.h>
#include <string>
#include <sstream>
#include <Objbase.h>
#include <Rpcdce.h> 

#include "string_utility.h"
#include "registry_utility.h"

std::string GenLocalRand()
{
    SYSTEMTIME  SystemTime;
    GetSystemTime(&SystemTime);

    srand((unsigned)time(NULL));
    int randNum = rand();

    std::stringstream out;

    out << SystemTime.wDay;
    out << SystemTime.wDayOfWeek;
    out << SystemTime.wHour;
    out << SystemTime.wMilliseconds;
    out << SystemTime.wMinute;
    out << SystemTime.wMonth;
    out << SystemTime.wSecond;
    out << SystemTime.wYear;
    out << randNum;

    int count = 32-out.str().length();
    while(count--)
    {
        out << 0;
    }
    return out.str();
}

bool GenGUID(GUID& guid)
{
    if(S_OK != CoCreateGuid(&guid))
    {
		return false;
	}
    return true;
}

std::string stripTo32Guid(std::string& strUid)
{
    std::string strUid32;
    std::string str;
    size_t pos = 0;

    for(;;)
    {
        pos = strUid.find('-');
        if((pos < 0)||(pos > 36))
        {
            strUid32.append(strUid);
            break;
        }

        str = strUid.substr(0,pos);
        strUid32.append(str);
        strUid.erase(0,pos+1);
    }
    return strUid32;
}

std::string GetGuidStr()
{
    GUID guid;
    if(!GenGUID(guid))
	{
        return GenLocalRand();
	}

    LPTSTR guid_example = L"34DF2C50-2E42-4172-B257-CE868FFC8C42";
    size_t guid_length = (wcslen(guid_example)+1);
    RPC_WSTR UuidToStringPtr;
    if(RPC_S_OK != UuidToString((UUID*)&guid, &UuidToStringPtr))
	{
		return GenLocalRand();
	}

    char* str = new char[guid_length];
    for(int nCounter=0; nCounter < static_cast<int>(guid_length);nCounter++)
    {
        str[nCounter] = static_cast<char>(UuidToStringPtr[nCounter]);
    }

    std::string guidStr(str);
    delete []str;
    return stripTo32Guid(guidStr);
}

bool WriteUidToConfigRegistryIfEmpty()
{
	WriteRegKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\BingWallPaper\\Config", L"NewUser", L"0");
	std::wstring formerUid;
	if(ERROR_SUCCESS == ReadRegKey(L"Software\\Microsoft\\BingWallPaper\\Config", L"Uid", formerUid, false))
	{
		if(L"" != formerUid)
		{
			return true;
		}
	}
    std::string strUid = GetGuidStr();
    
	if(ERROR_SUCCESS != WriteRegKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\BingWallPaper\\Config", L"Uid", UTF8ToUTF16(strUid).c_str()))
	{
		return false;
	}
	WriteRegKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\BingWallPaper\\Config", L"NewUser", L"1");
    return true;
}