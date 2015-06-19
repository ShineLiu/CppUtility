#include "stdafx.h"
//#pragma warning(disable: 4995)
#include "atlstr.h"
#include "SystemInfoUUIDHandler.h"

HRESULT SystemInfoUUIDHandler::OnLoadTable(_In_reads_(cbStruct) PBYTE pbStructHeader, _In_ DWORD cbStruct) 
{
	UNREFERENCED_PARAMETER(cbStruct);
    if (!m_found)
    {
        PSMBIOS_SYSTEM_INFORMATION_STRUCT pSysInfoStruct = reinterpret_cast<PSMBIOS_SYSTEM_INFORMATION_STRUCT>(pbStructHeader);
        size_t const cbUuid = ARRAYSIZE(pSysInfoStruct->Uuid);
        UCHAR zeros[cbUuid] = {};
        if (memcmp(zeros, pSysInfoStruct->Uuid, cbUuid) != 0)
        {
            C_ASSERT(sizeof(pSysInfoStruct->Uuid) == sizeof(GUID));
            // The buffer of pSysInfoStruct->Uuid is the same form as in GUID struct
            GUID *uuid = reinterpret_cast<GUID*>(&pSysInfoStruct->Uuid);
            int res = StringFromGUID2(*uuid, m_uuid, GUIDSTR_MAX);
            if (res != 0)
            {
                m_found = true;
            }
        }
    }
    return S_OK;
}

CStringW SystemInfoUUIDHandler::StrUUID() const
{
    return m_uuid;
}

bool SystemInfoUUIDHandler::Found() const 
{
    return m_found;
}
