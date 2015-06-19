// --------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// --------------------------------------------------------------------
#pragma once

HRESULT
GetSmbiosFirmwareTable_Api(
        _Outptr_result_buffer_(*pcbSmbios) PBYTE* ppbSmbios,
        _Out_ DWORD* pcbSmbios
        );
