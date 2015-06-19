#include "stdafx.h"
#include "NotifyIconManager.h"
#include <windows.h>
#include <tchar.h>
#include "shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")

NOTIFYICONDATA NotifyIconManager::m_NotifyIconData;  
NotifyIconManager::NotifyIconManager(void)
{

}
NotifyIconManager::~NotifyIconManager(void)
{
}

int NotifyIconManager::GetNotifyDataSize()
{
	HMODULE instance = ::LoadLibraryW(L"shell32.dll");
	int result = sizeof(NOTIFYICONDATA);
	if (instance != NULL)
	{
		DLLGETVERSIONPROC dllGetVersion = DLLGETVERSIONPROC(::GetProcAddress(instance, "DllGetVersion"));
		if (dllGetVersion != NULL)
		{
			DLLVERSIONINFO version;
			memset(&version, 0, sizeof version);
			version.cbSize = sizeof version;
			dllGetVersion(&version);
			if (version.dwMajorVersion == 6 && version.dwMinorVersion == 0 && version.dwBuildNumber <= 2900)
			{
				//windows xp
				result = NOTIFYICONDATA_V3_SIZE;
			}
		}
		::FreeLibrary(instance);
	}
	return result;
}

bool NotifyIconManager::AddNotifyIcon(HWND hwnd, UINT uID, UINT uCallbackMessage, HICON hIcon, LPCTSTR pszTip)
{
	//NOTIFYICONDATA nid;  
	ZeroMemory(&m_NotifyIconData, sizeof(NOTIFYICONDATA));  
	m_NotifyIconData.cbSize           = GetNotifyDataSize();   
	m_NotifyIconData.hWnd             = hwnd;  
	m_NotifyIconData.uID              = uID;  
	m_NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;  
	m_NotifyIconData.uCallbackMessage = uCallbackMessage;  
	m_NotifyIconData.hIcon            = hIcon;   
	_tcscpy_s(m_NotifyIconData.szTip, sizeof(m_NotifyIconData.szTip)/sizeof(m_NotifyIconData.szTip[0]), pszTip);

	return ::Shell_NotifyIcon(NIM_ADD, &m_NotifyIconData) != 0;  
}

bool NotifyIconManager::ModifyNotifyIcon(HWND hwnd, UINT uID, UINT uCallbackMessage, HICON hIcon, LPCTSTR pszTip)
{
	//NOTIFYICONDATA nid;
	ZeroMemory(&m_NotifyIconData, sizeof(NOTIFYICONDATA));
	m_NotifyIconData.cbSize           = GetNotifyDataSize();
	m_NotifyIconData.hWnd             = hwnd;
	m_NotifyIconData.uID              = uID;
	m_NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_NotifyIconData.uCallbackMessage = uCallbackMessage;
	if (hIcon!=NULL)
	{
		m_NotifyIconData.hIcon            = hIcon;
	}
	if (pszTip!=NULL)
	{
		_tcscpy_s(m_NotifyIconData.szTip, sizeof(m_NotifyIconData.szTip)/sizeof(m_NotifyIconData.szTip[0]), pszTip);
	}

	return ::Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData) != 0;
}


void NotifyIconManager::DelNotifyIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &m_NotifyIconData);
}


void NotifyIconManager::ShowBalloonTip(LPCTSTR pszTitle, LPCTSTR pszContent, DWORD dwInfoIcon)
{
	_tcscpy_s(m_NotifyIconData.szInfoTitle, sizeof(m_NotifyIconData.szInfoTitle)/sizeof(m_NotifyIconData.szInfoTitle[0]), pszTitle);
	_tcscpy_s(m_NotifyIconData.szInfo, sizeof(m_NotifyIconData.szInfo)/sizeof(m_NotifyIconData.szInfo[0]), pszContent);
	m_NotifyIconData.uFlags |= NIF_INFO;

	m_NotifyIconData.uTimeout = 500;
	m_NotifyIconData.dwInfoFlags = dwInfoIcon;

	Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData);
}