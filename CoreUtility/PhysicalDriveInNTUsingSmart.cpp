#include "stdafx.h"
#include "PhysicalDriveInNTUsingSmart.h"
#include <winioctl.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>
#include "formatstr.h"


HRESULT ConvertUSHORTToWSTR(__in_bcount(cbData) USHORT *puData, __in DWORD cbData, __in WORD startIndex, __in WORD endIndex, __out PWSTR *ppszData)
{
	UNREFERENCED_PARAMETER(cbData);

    HRESULT hr = S_OK;
    size_t dwSrcSize = (endIndex - startIndex + 1) * 2;
    size_t dwDestSize = dwSrcSize + 1; 
    char *pszIndex = (char*) (puData + startIndex);
    *ppszData = new WCHAR[dwDestSize];
    memset((void*) *ppszData, 0, dwDestSize *sizeof(WCHAR));
    for (size_t index = 0; index < dwDestSize - 1; index++)
    {
        (*ppszData)[index] = (0xff & pszIndex[index]);
    }
    if (SUCCEEDED(hr))
    {
        for (DWORD index = 0; index + 2 < dwDestSize; index +=2)
        {
            WCHAR temp = (*ppszData)[index];
            (*ppszData)[index] = (*ppszData)[index + 1];
            (*ppszData)[index + 1] = temp;
        }
    }
    return hr;
}

HRESULT GetDiskSNFromPhysicalDriveInNTUsingSmart(__out PWSTR *ppszDiskSN)
{
    HRESULT hr = E_FAIL;

    for(WORD drive = 0; drive < MAX_IDE_DRIVES && FAILED(hr); drive++)
    {
        HANDLE hDrive = NULL;
        CFormatStr driveName(L"\\\\.\\PhysicalDrive%d", drive);
        hDrive = CreateFileW(
            (PCWSTR) driveName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
        if (hDrive != INVALID_HANDLE_VALUE)
        {
            GETVERSIONINPARAMS getVersionParams;
            DWORD dwBytesReturnd = 0;
            memset((void*) &getVersionParams, 0, sizeof(getVersionParams));
            if (DeviceIoControl(
                hDrive,
                SMART_GET_VERSION,
                NULL,
                0,
                &getVersionParams,
                sizeof(GETVERSIONINPARAMS),
                &dwBytesReturnd,
                NULL
                ))
            {
                DWORD cbCommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
                PSENDCMDINPARAMS pCommand = (PSENDCMDINPARAMS) malloc(cbCommandSize);
                pCommand->irDriveRegs.bCommandReg = ID_CMD;
                if (DeviceIoControl(
                    hDrive,
                    SMART_RCV_DRIVE_DATA,
                    pCommand,
                    sizeof(SENDCMDINPARAMS),
                    pCommand,
                    cbCommandSize,
                    &dwBytesReturnd,
                    NULL
                    ))
                {
                    assert(sizeof(IDENTIFY_DATA) == 512);
                    USHORT *pIdSector = (USHORT*)(PIDENTIFY_DATA)((PSENDCMDOUTPARAMS) pCommand) -> bBuffer;
                    hr = ConvertUSHORTToWSTR(pIdSector, 512, 10, 19, ppszDiskSN);
                }
                CloseHandle(hDrive);
                free(pCommand);
            }
        }
    }
    return hr;
}