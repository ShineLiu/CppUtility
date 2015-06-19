#pragma once

class NotifyIconManager
{
public:  
	NotifyIconManager(void);  
	~NotifyIconManager(void);  

public:
	bool AddNotifyIcon(HWND hwnd, UINT uID, UINT uCallbackMessage, HICON hIcon, LPCTSTR pszTip);
	bool ModifyNotifyIcon(HWND hwnd, UINT uID, UINT uCallbackMessage, HICON hIcon, LPCTSTR pszTip);
	void DelNotifyIcon();
	void ShowBalloonTip(LPCTSTR pszTitle, LPCTSTR pszContent, DWORD dwInfoIcon);
	int GetNotifyDataSize();

public:
	static NOTIFYICONDATA m_NotifyIconData;  	 
};
