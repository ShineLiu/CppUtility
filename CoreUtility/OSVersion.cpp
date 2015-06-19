#include <stdafx.h>
#include "OSVersion.h"
#include <blob.h>
#include <Windows.h>
#include <RegUtil.h>
#include <dhutil.h>

#define REGKEY_CRYPTOGRAPHY L"SOFTWARE\\Microsoft\\Cryptography"
#define REGKEY_NAME_MACHINE_GUID L"MachineGuid"
#define _WIN32_WINNT_WINBLUE 0x0603

typedef BOOL(WINAPI *FuncPtr_IsWow64Process) (HANDLE, PBOOL);

NONUT_STATIC HRESULT GetMachineGuid(__out PWSTR *ppszMachineGuid)
{
    HRESULT hr = E_FAIL;
    CString csMachineGuid;
    if (ReadRegistryString(REGKEY_CRYPTOGRAPHY, REGKEY_NAME_MACHINE_GUID, csMachineGuid, HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY) == rrrSucceeded)
    {
        DWORD cchMachineGuid = csMachineGuid.GetLength() + 1;
        PWSTR pszBuffer = NULL;
        pszBuffer = new WCHAR[cchMachineGuid];
        hr = wcsncpy_s(pszBuffer, cchMachineGuid, csMachineGuid, _TRUNCATE);
        if (SUCCEEDED(hr))
        {
            *ppszMachineGuid = pszBuffer;
        }
        else
        {
            if (pszBuffer != NULL)
            {
                delete[] pszBuffer;
            }
        }
    }
    return hr;
}

bool IsWindows8Point1OrGreater()
{
    return DhUtil::IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0);
}

HRESULT GetOSVersionInfoEx(__out OSVERSIONINFOEXW *pOSVersionInfoEx)
{
    ZeroMemory(pOSVersionInfoEx, sizeof(OSVERSIONINFOEXW));
    pOSVersionInfoEx->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
#pragma warning(suppress: 4996) // GetVersionExW was declared deprecated.
    BOOL fReturn = GetVersionExW((OSVERSIONINFOW*)pOSVersionInfoEx);
    return (fReturn != 0) ? S_OK : E_FAIL;
}

void FreeOSVersion(__in POSVersionInfo pOSVersionInfo)
{
    if (pOSVersionInfo != NULL)
    {
        if (pOSVersionInfo->pszMachineGuid != NULL)
        {
            delete[] pOSVersionInfo->pszMachineGuid;
            pOSVersionInfo->pszMachineGuid = NULL;
        }
    }
}

HRESULT CollectOSVersionInfo(__out POSVersionInfo pOSVersionInfo)
{
	HRESULT hr = E_FAIL;

	if(pOSVersionInfo)
	{
		HMODULE hModule = GetModuleHandle(L"kernel32.dll");

		WCHAR lpFilename[MAX_PATH] = { 0 };
		DWORD nSize = MAX_PATH;

		DWORD Size = 0;
		DWORD Dummy;
		LPVOID pData = NULL;
		LPVOID pBuffer = NULL;
		UINT Len = 0;
		VS_FIXEDFILEINFO * pInfo = NULL;

		Size = GetModuleFileName(hModule, lpFilename, nSize);
		if (0 != Size)
		{
			Size = GetFileVersionInfoSize(lpFilename, &Dummy);
			if (0 != Size)
			{
				pData = new BYTE[Size];
				if (pData)
				{
					if (GetFileVersionInfo(lpFilename, 0, Size, pData))
					{
						if (VerQueryValue(pData, TEXT("\\"), &pBuffer, &Len))
						{
							pInfo = (VS_FIXEDFILEINFO *)pBuffer;
							if (pInfo)
							{
								pOSVersionInfo->osVersionInfo.dwMajorVersion = HIWORD(pInfo->dwProductVersionMS);
								pOSVersionInfo->osVersionInfo.dwMinorVersion = LOWORD(pInfo->dwProductVersionMS);
								pOSVersionInfo->osVersionInfo.dwBuildNumber = HIWORD(pInfo->dwProductVersionLS);
								pOSVersionInfo->osVersionInfo.dwPlatformId = LOWORD(pInfo->dwProductVersionLS);
								hr = S_OK;
							}
						}
					}
				}
			}
		}

		if (pData)
		{
			delete pData;
			pData = NULL;
		}

		if (SUCCEEDED(hr))
		{
			// If fail in GetMachineGuid(), pszMachienGuid will be default value NULL
			GetMachineGuid(&(pOSVersionInfo->pszMachineGuid));
		}
	}

	return hr;
}

HRESULT DumpOSVersionInfo(__in  POSVersionInfo pOSVersionInfo, __deref_out_bcount(*pcbData) PBYTE *ppbData, __out PDWORD pcbData)
{
    HRESULT hr = E_FAIL;
    CBlob<ProcAllocator> blobOSVersionInfo;
    CBlob<ProcAllocator> blobMachineGuidString;
    if (pOSVersionInfo->pszMachineGuid != NULL)
    {
        hr = blobMachineGuidString.AppendString(pOSVersionInfo->pszMachineGuid, &pOSVersionInfo->cbMachineGuid);
    }    
    blobOSVersionInfo.Append(pOSVersionInfo, sizeof(OSVersionInfo));
    if (SUCCEEDED(hr))
    {
        blobOSVersionInfo.Append(blobMachineGuidString);
    }
    blobOSVersionInfo.Detach((PVOID *) ppbData, pcbData);
    return S_OK;
}

BOOL IsWindows64bit()
{
#ifdef _WIN64
    // 64-bit processes can only run on 64-bit Windows.
    return TRUE;
#else
    // 32-bit processes run in WOW64 on 64-bit Windows and without WOW64 otherwise.
    // Note that IsWow64Process is not available on all supported versions of Windows.
    BOOL fWow64 = FALSE;
    HMODULE hModule = GetModuleHandle(L"kernel32");
    if (hModule != NULL)
    {
        FuncPtr_IsWow64Process pfuncIsWow64Process = (FuncPtr_IsWow64Process)GetProcAddress(
            hModule, "IsWow64Process");
        if (pfuncIsWow64Process != NULL)
        {
            pfuncIsWow64Process(GetCurrentProcess(), &fWow64);
        }
    }
    return fWow64;
#endif
}
