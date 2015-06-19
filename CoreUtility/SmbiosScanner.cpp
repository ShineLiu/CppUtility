// --------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// --------------------------------------------------------------------

#include "stdafx.h"
#include <vector>
#include <windows.h>
#include "smbios.h"
#include "SmbiosScanner.h"
#include "SmbiosTable.h"
#include <assert.h>

// The following code is extracted from DeviceInfo collector and will be merged with DeviceInfo once it is shipped

HRESULT SmbiosScanner::Scan()
{
    HRESULT hr = S_OK;
    PBYTE pbData = nullptr;
    DWORD cbData = 0;
    PBYTE pbTail = nullptr;
    size_t cbTail = 0;

    hr = GetSmbiosFirmwareTable_Api(&pbData, &cbData);

    if (SUCCEEDED(hr))
    {
        pbTail = pbData;
        cbTail = cbData;

        if (cbTail >= sizeof(SMBIOS_STRUCT_HEADER))
        {
            while (cbTail > 0 && SUCCEEDED(hr))
            {
                PSMBIOS_STRUCT_HEADER pStructHeader = reinterpret_cast<PSMBIOS_STRUCT_HEADER>(pbTail);
                if (pStructHeader->Length < sizeof(SMBIOS_STRUCT_HEADER) || pStructHeader->Length > cbTail)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    break;
                }

                PBYTE pbEntry = pbTail + pStructHeader->Length;
                size_t cbEntry = cbTail - pStructHeader->Length;

                while (cbEntry > 0)
                {
                    PBYTE pbNextEntry = reinterpret_cast<const PBYTE>(memchr(pbEntry, '\0', cbEntry - 1));
                    if (pbNextEntry == nullptr)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                        break;
                    }

                    pbNextEntry++;
                    cbEntry -= (pbNextEntry - pbEntry);
                    pbEntry = pbNextEntry;

                    if (cbEntry > 0)
                    {
                        if (*pbEntry == '\0')
                        {
                            pbEntry++;
                            cbEntry--;
                            break;
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                        break;
                    }
                }
                if (SUCCEEDED(hr))
                {
                    assert(cbEntry >= 0 && pbTail == reinterpret_cast<PBYTE>(pStructHeader));
                    if (m_pHandler != nullptr && pStructHeader->Type == m_pHandler->TableType())
                    {
                        m_pHandler->OnLoadTable(pbTail, pStructHeader->Length);
                    }

                    pbTail = pbEntry;
                    cbTail = cbEntry;
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (cbTail != 0)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
    }
    // Make sure that pbData is nullptr when the allocation from GetSmbiosFirmwareTable_Api failed.
    if (pbData)
    {
        HeapFree(GetProcessHeap(), 0, pbData);
    }
    return hr;
}
