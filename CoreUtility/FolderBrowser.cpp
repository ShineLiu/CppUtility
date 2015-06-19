#include "stdafx.h"
#include "FolderBrowser.h"


FolderBrowser::FolderBrowser()
{
}

FolderBrowser::~FolderBrowser()
{
}

int CALLBACK FolderBrowser::BrowseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		::SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, lpData);
		break;
	case BFFM_SELCHANGED:
		{
			TCHAR pszPath[MAX_PATH];
			::SHGetPathFromIDList((LPCITEMIDLIST)lParam, pszPath);
			::SendMessage(hwnd, BFFM_SETSTATUSTEXT, TRUE, (LPARAM)pszPath);
		}
		break;
	}
	return 0;
}

std::wstring FolderBrowser::DoBrowse(HWND hParent, LPCTSTR originPath)
{
    BROWSEINFO bInfo;
    bInfo.hwndOwner			= hParent;
	bInfo.pidlRoot			= NULL;  
	bInfo.pszDisplayName	= NULL; 
    bInfo.lpszTitle			= L"请选择文件夹";
    bInfo.ulFlags			= BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    bInfo.lpfn				= BrowseCtrlCallback;
    bInfo.lParam			= (LPARAM)originPath;
	bInfo.iImage			= 0; 

	LPITEMIDLIST pidl = ::SHBrowseForFolder(&bInfo);
    if (NULL == pidl)
    {
        return L"";
    }

	wchar_t prgfDir[1024] = {0};
	BOOL bRet = ::SHGetPathFromIDList(pidl, prgfDir);
    if (!bRet)
    {
		return L"";
    }

    return prgfDir;
}