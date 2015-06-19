// --------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// --------------------------------------------------------------------
#pragma once

#include <typeinfo>
#include <list>
#include "smbios.h"

#define MAX_SMBIOS_TYPE 256

class SmbiosHandler
{
public:
    SmbiosHandler(DWORD type) : _dwTableType(type) {}
    virtual HRESULT OnLoadTable(_In_reads_(cbStruct) PBYTE pbStructHeader, _In_ DWORD cbStruct) = 0;
    DWORD TableType() const
    {
        return _dwTableType;
    }
    virtual ~SmbiosHandler() {}

protected:
    DWORD _dwTableType;
    DWORD _cbSmbios;

private:
    SmbiosHandler() {}
};

class SmbiosScanner sealed
{
public:
    HRESULT Scan();
    void AddHandler(_In_ SmbiosHandler *pHandler)
    {
        m_pHandler = pHandler;
    }
private:
    SmbiosHandler * m_pHandler;
};
