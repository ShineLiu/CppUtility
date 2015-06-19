#pragma once
#include <windows.h>
enum WindowMessage
{
	WM_InterProcessStart = WM_USER + 100,
	WM_UpdateClose,
	WM_Info,
	WM_SaveAs,
	WM_Pre,
	WM_Next,
	WM_ClassifyOn,
	WM_ClassifyOff,
	WM_ShowMainDlg,
	WM_NotifyHideIcons,
	WM_InterProcessEnd = WM_USER + 1000,
	//above:inter-process window messages, MUST NOT change order
	//----------------------------------------
	//below:in-process window messages, can change order
	WM_InProcessStart = WM_USER + 1001,
	WM_AssociateChanged,
	WM_WallpaperChanged,
	WM_FolderViewCreate,
	WM_FolderViewShowHide,
	WM_FolderViewRefresh,
	WM_FolderViewSortIcons,
	WM_FolderViewIconSize,
	WM_PasteToDesktop,
	WM_NotifyIcon,
	WM_SetWallPaperStart,
	WM_SetWallPaperSuccess,
	WM_SetWallPaperError,
	WM_InProcessEnd
};

