// --------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// --------------------------------------------------------------------
#pragma once

#include "SmbiosScanner.h"
#pragma warning(disable: 4995)
#include "atlstr.h"

#define GUIDSTR_MAX 39   // {8-4-4-4-12} (includes brackets and null-terminated)

class SystemInfoUUIDHandler sealed : public SmbiosHandler
{
public:
    SystemInfoUUIDHandler::SystemInfoUUIDHandler() : SmbiosHandler(SMBIOS_SYSTEM_INFORMATION), m_found(false) {}
    HRESULT OnLoadTable(_In_reads_(cbStruct) PBYTE pbStructHeader, _In_ DWORD cbStruct) override;
    CStringW StrUUID() const;
    bool Found() const;

private:
    WCHAR m_uuid[GUIDSTR_MAX];
    bool m_found;
};
