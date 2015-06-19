// --------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// --------------------------------------------------------------------
#include <stdafx.h>
#include <typeinfo>
#include <algorithm>
#include <windows.h>
#include <WbemIdl.h>
#include <TlHelp32.h>
#include "SmbiosTable.h"

// The following code is extracted from DeviceInfo collector and will be merged with DeviceInfo once it is shipped

typedef UINT(WINAPI *PFN_GetSystemFirmwareTable)(
    _In_   DWORD FirmwareTableProviderSignature,
    _In_   DWORD FirmwareTableID,
    _Out_writes_(BufferSize)  PVOID pFirmwareTableBuffer,
    _In_   DWORD BufferSize
    );

struct RawSMBIOSData
{
    BYTE    Used20CallingMethod;
    BYTE    SMBIOSMajorVersion;
    BYTE    SMBIOSMinorVersion;
    BYTE    DmiRevision;
    DWORD    Length;
    BYTE    SMBIOSTableData[1]; // Use 1 to avoid warning c4200 zero-sized array in struct/union
};

HRESULT
GetSmbiosFirmwareTable_Api(
_Outptr_result_buffer_(*pcbSmbios) PBYTE* ppbSmbios,
_Out_ DWORD* pcbSmbios
)
{
    static DWORD const s_dwRSMB = 'RSMB';
    static PFN_GetSystemFirmwareTable s_pfnGetSystemFirmwareTable = nullptr;

    PBYTE pbData = nullptr;
    UINT cbData = 0;
    UINT cbRead = 0;
    PBYTE pbSmbios = nullptr;
    DWORD cbSmbios = 0;
    RawSMBIOSData* pHeader = nullptr;
    HMODULE hModule = nullptr;
    PFN_GetSystemFirmwareTable pfn = nullptr;
    HRESULT hr = S_OK;

    *pcbSmbios = 0;
    *ppbSmbios = nullptr;

    pfn = (PFN_GetSystemFirmwareTable)InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID*>(&s_pfnGetSystemFirmwareTable),
        nullptr,
        nullptr
        );

    if (pfn == nullptr)
    {
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN, L"kernel32.dll", &hModule))
        {
            pfn = (PFN_GetSystemFirmwareTable)GetProcAddress(hModule, "GetSystemFirmwareTable");
            if (pfn != nullptr)
            {
                InterlockedCompareExchangePointer(
                    reinterpret_cast<PVOID*>(&s_pfnGetSystemFirmwareTable),
                    reinterpret_cast<PVOID>(pfn),
                    nullptr
                    );
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (pfn != nullptr)
    {
        cbData = pfn(s_dwRSMB, 0, nullptr, 0);
        pbData = reinterpret_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbData));
        if (pbData != nullptr)
        {
            cbRead = pfn(s_dwRSMB, 0, pbData, cbData);
            if (cbRead == cbData)
            {
                // The data start with MSSmBios_RawSMBiosTables/SMBIOSVERSIONINFO header
                if (cbData >= FIELD_OFFSET(RawSMBIOSData, SMBIOSTableData))
                {
                    pHeader = reinterpret_cast<RawSMBIOSData*>(pbData);
                    pbSmbios = pbData + FIELD_OFFSET(RawSMBIOSData, SMBIOSTableData);
                    cbSmbios = cbData - FIELD_OFFSET(RawSMBIOSData, SMBIOSTableData);

                    if (pHeader->Length == cbSmbios)
                    {
                        *ppbSmbios = reinterpret_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbSmbios));
                        if (*ppbSmbios != nullptr)
                        {
                            memcpy_s(*ppbSmbios, cbSmbios, pbSmbios, cbSmbios);
                            *pcbSmbios = cbSmbios;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (pbData)
    {
        HeapFree(GetProcessHeap(), 0, pbData);
    }
    return hr;
}
