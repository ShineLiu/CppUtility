#include "stdafx.h"
#include "newsha256.h"
#include <Windows.h>
#include "lock.h"
#include "TPMInfo.h"
#include <tchar.h>
#include <strsafe.h>
#include "SmbiosScanner.h"
#include "SystemInfoUUIDHandler.h"
#include "WMI.h"
#include "PhysicalDriveInNTUsingSmart.h"
#include "DeviceID.h"

using namespace DeviceHealth;

CriticalSection g_collectDeviceIdLock;
BOOL g_fIsDeviceIdCached = FALSE;
DeviceHealth::DeviceID g_cachedDeviceId;

HRESULT SHA256HashDeviceID(__in PCWSTR pszUUID, __in PCWSTR pszDiskSN, __out SHA256_HASHVAL* pDeviceID)
{
	DWORD cbBufferSize = static_cast<DWORD>(wcslen(pszUUID) + wcslen(pszDiskSN) + 1);
	PWSTR pszBuffer = new WCHAR[cbBufferSize];
	HRESULT hr = StringCchCopyW(pszBuffer, cbBufferSize, pszUUID);
	if (SUCCEEDED(hr))
	{
		hr = StringCchCatW(pszBuffer, cbBufferSize, pszDiskSN);
		if (SUCCEEDED(hr))
		{
			hr = SHA256HashData(pszBuffer, pDeviceID);
		}
	}

	if (pszBuffer != NULL)
	{
		delete[] pszBuffer;
	}
	return hr;
}

HRESULT GetAlphanumericPart(__inout PWSTR *ppszData)
{
	DWORD dwValidNum = 0;
	DWORD dwLen = static_cast<DWORD>(wcsnlen_s(*ppszData, MAX_UUID_LEN));
	for (DWORD index = 0; index <dwLen; index++)
	{
		dwValidNum += !!iswalnum((*ppszData)[index]);
	}
	PWSTR pszBuffer = new WCHAR[dwValidNum + 1];
	DWORD dwCount = 0;
	for (DWORD index = 0; dwCount < dwValidNum; index++)
	{
		if (iswalnum((*ppszData)[index]) != 0)
		{
			pszBuffer[dwCount++] = (*ppszData)[index];
		}
	}
	pszBuffer[dwCount] = 0;
	delete[] * ppszData;
	*ppszData = pszBuffer;
	return S_OK;
}

HRESULT GetDiskSNFromAPI(__deref_out PWSTR *ppszDiskSN)
{
	HRESULT hr = GetDiskSNFromPhysicalDriveInNTUsingSmart(ppszDiskSN);
	if (SUCCEEDED(hr))
	{
		hr = GetAlphanumericPart(ppszDiskSN);
	}
	return hr;
}

HRESULT GetDiskSNFromWMI(__deref_out PWSTR *ppszDiskSN)
{
	WMI wmiDiskSN;

	HRESULT hr = wmiDiskSN.ExecQuery(CIMV2_NAMESPACE, DiskDrive);
	if (SUCCEEDED(hr))
	{
		for (;;)
		{
			BOOL fEnd = TRUE;
			hr = wmiDiskSN.NextObject(&fEnd);
			if (SUCCEEDED(hr) && fEnd == FALSE)
			{
				VARIANT valMediaType;
				hr = wmiDiskSN.GetCurrentObjectPoperty(MEDIATYPE, &valMediaType);
				if (SUCCEEDED(hr))
				{
					PWSTR pszMediaType;
					hr = wmiDiskSN.ConvertVariantToWSTR(&valMediaType, &pszMediaType);
					VariantClear(&valMediaType);
					if (SUCCEEDED(hr))
					{
						bool fIsFixedDrive = (wcsncmp(pszMediaType, L"Fixed", 5) == 0);
						delete[] pszMediaType;
						if (fIsFixedDrive) // Ignore non-fixed disk drive
						{
							VARIANT valDiskSN;
							hr = wmiDiskSN.GetCurrentObjectPoperty(SERIALNUMBER, &valDiskSN);
							if (SUCCEEDED(hr))
							{
								hr = wmiDiskSN.ConvertVariantToWSTR(&valDiskSN, ppszDiskSN);
								VariantClear(&valDiskSN);
								if (SUCCEEDED(hr))
								{
									hr = GetAlphanumericPart(ppszDiskSN);
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				hr = E_FAIL;
				break;
			}
		}
	}
	return hr;
}

HRESULT GetDiskSN(__deref_out PWSTR *ppszDiskSN)
{
	HRESULT hr = GetDiskSNFromWMI(ppszDiskSN);
	if (FAILED(hr)) // Fall back to API
	{
		hr = GetDiskSNFromAPI(ppszDiskSN);
	}
	return hr;
}

HRESULT GetBasicDeviceID(__out SHA256_HASHVAL* pBasicDeviceID)
{
	PWSTR pszUUID = NULL;
	PWSTR pszDiskSN = NULL;
	HRESULT hr = S_OK;
	BOOL fGetData = FALSE;

	hr = GetUUID(&pszUUID);
	if (FAILED(hr))
	{
		pszUUID = new WCHAR[1];
		pszUUID[0] = 0;
	}
	else
	{
		fGetData = TRUE;
	}
	hr = GetDiskSN(&pszDiskSN);
	if (FAILED(hr))
	{
		pszDiskSN = new WCHAR[1];
		pszDiskSN[0] = 0;
	}
	else
	{
		fGetData = TRUE;
	}

	if (fGetData)
	{
		hr = SHA256HashDeviceID(pszUUID, pszDiskSN, pBasicDeviceID);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}

	if (pszUUID != NULL)
	{
		delete[] pszUUID;
	}
	if (pszDiskSN != NULL)
	{
		delete[] pszDiskSN;
	}

	return hr;
}

HRESULT GetTPMDeviceID(__out SHA256_HASHVAL* pTPMDeviceID)
{
	HRESULT hr = S_OK;
	PBYTE pbTPMID = nullptr;
	DWORD cbTPMID = 0;
	hr = TPMGetID(&pbTPMID, &cbTPMID);

	if (SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(memcpy_s(pTPMDeviceID, sizeof(SHA256_HASHVAL), pbTPMID, cbTPMID));
	}

	// Cleanup
#pragma warning(suppress: 6102)
	// in TPMGetSaltedID
	//
	//if (*ppbTPMID != nullptr && FAILED(hr))
	//{
	//    HeapFree(GetProcessHeap(), 0, *ppbTPMID);
	//    ppbTPMID = nullptr;
	//}
	//if TPMGetID failed and ppbTPMID will free and set to nullptr.


	if (pbTPMID != nullptr)
	{
		HeapFree(GetProcessHeap(), 0, pbTPMID);
		pbTPMID = nullptr;
	}

	return hr;
}

HRESULT CollectDeviceID(__out DeviceHealth::PDeviceID pDeviceID)
{
	SHA256_HASHVAL bBasicDeviceID;
	SHA256_HASHVAL bTPMDeviceID;
	HRESULT hr = S_OK;

	// Only allow single thread to enter
	LockGuard<CriticalSection> lock(g_collectDeviceIdLock);

	if (!g_fIsDeviceIdCached)
	{
		// Compute the device ID if it's not cached
		memset((void*)&g_cachedDeviceId, 0, sizeof(g_cachedDeviceId));

		// Set version
		g_cachedDeviceId.dwheader |= DEVICEID_VERSION << 24;

		// Collect different types of IDs
		HRESULT hrBasicID = GetBasicDeviceID(&bBasicDeviceID);
		if (SUCCEEDED(hrBasicID))
		{
			memcpy(g_cachedDeviceId.bBasicDeviceID, &bBasicDeviceID, sizeof(SHA256_HASHVAL));
			g_cachedDeviceId.dwheader |= 1 << DeviceIDType::BASIC_DEVICE_ID;
		}

		HRESULT hrTPMID = GetTPMDeviceID(&bTPMDeviceID);
		if (SUCCEEDED(hrTPMID))
		{
			memcpy(g_cachedDeviceId.bTPMDeviceID, &bTPMDeviceID, sizeof(SHA256_HASHVAL));
			g_cachedDeviceId.dwheader |= 1 << DeviceIDType::TPM_DEVICE_ID;
		}

		// Indicate the cached value contains the valid device ID
		if (SUCCEEDED(hrBasicID) || SUCCEEDED(hrTPMID))
		{
			g_fIsDeviceIdCached = TRUE;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		}
	}
	// Copy the cached device ID to the target
	memcpy(pDeviceID, &g_cachedDeviceId, sizeof(DeviceID));

	return hr;
}

HRESULT GetUUIDFromSmbios(__deref_out PWSTR *ppszUUID)
{
	SmbiosScanner SmbiosScanner;
	SystemInfoUUIDHandler UUIDHandler;

	SmbiosScanner.AddHandler(&UUIDHandler);
	HRESULT hr = SmbiosScanner.Scan();
	if (SUCCEEDED(hr))
	{
		if (UUIDHandler.Found())
		{
			// We have to allocate a buffer now because GetAlphanumericPart() deletes the input buffer internally
			PWSTR pszBuffer = new WCHAR[GUIDSTR_MAX];
			if (pszBuffer == nullptr)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				wcscpy_s(pszBuffer, GUIDSTR_MAX, UUIDHandler.StrUUID());
				*ppszUUID = pszBuffer;
				hr = GetAlphanumericPart(ppszUUID);
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		}
	}
	return hr;
}

HRESULT GetUUIDFromWMI(__deref_out PWSTR *ppszUUID)
{
	VARIANT valUUID;
	WMI wmiUUID;

	VariantInit(&valUUID);
	HRESULT hr = wmiUUID.GetCurrentObjectPoperty(CIMV2_NAMESPACE, COMPUTERSYSTEMPRODUCT, UUID, &valUUID);
	if (SUCCEEDED(hr))
	{
		hr = wmiUUID.ConvertVariantToWSTR(&valUUID, ppszUUID);
		if (SUCCEEDED(hr))
		{
			hr = GetAlphanumericPart(ppszUUID);
		}
	}
#pragma warning(suppress: 6102) 
	// WMI::GetCurrentObjectPoperty will never put valUUID into a unintialized state. It actually call IWbemClassObject::Get to modify valUUID. and valUUID is the third parameter passed in.
	// When IWbemClassObject::Get successful, this parameter is assigned the correct type and value for the qualifier, and the VariantInit function is called on pVal. It is the responsibility of the caller to call VariantClear on pVal when the value is not needed. If there is an error, the value that pVal points to is not modified. If an uninitialized pVal value is passed to the method, then the caller must check the return value of the method, and call VariantClear only when the method succeeds.
	VariantClear(&valUUID);
	return hr;
}

HRESULT GetUUID(__deref_out PWSTR *ppszUUID)
{
	HRESULT hr = GetUUIDFromWMI(ppszUUID);
	if (FAILED(hr)) // Fall back to API
	{
		hr = GetUUIDFromSmbios(ppszUUID);
	}
	return hr;
}
