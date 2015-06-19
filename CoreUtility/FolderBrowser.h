
#pragma once
#include <shlobj.h>
#include <string>

class FolderBrowser  
{
public:
	FolderBrowser();
	virtual ~FolderBrowser();

    std::wstring DoBrowse(HWND hParent, LPCTSTR originPath);
private:
    // The callback function
    static int CALLBACK BrowseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

