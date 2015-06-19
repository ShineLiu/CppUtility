#include "stdafx.h"
#include "TPMInfo.h"
#include "newsha256.h"

// these macros haven't been defined before v120_xp
#ifndef NCRYPT_PCP_EKPUB_PROPERTY
#define NCRYPT_PCP_EKPUB_PROPERTY                          L"PCP_EKPUB" // ncrypt.h
#endif
#ifndef MS_PLATFORM_CRYPTO_PROVIDER
#define MS_PLATFORM_CRYPTO_PROVIDER             L"Microsoft Platform Crypto Provider" // bcrypt.h
#endif

#define TPM_HASH_SALT "#$TGFFT^Y&HC#R$%$%&*IKNNYY*&KWER#"

typedef SECURITY_STATUS
	(WINAPI *FuncPtr_NCryptFreeObject)(
	__in    NCRYPT_HANDLE hObject);

typedef SECURITY_STATUS
	(WINAPI *FuncPtr_NCryptOpenStorageProvider)(
	__out   NCRYPT_PROV_HANDLE *phProvider,
	__in_opt LPCWSTR pszProviderName,
	__in    DWORD   dwFlags);

typedef SECURITY_STATUS
	(WINAPI *FuncPtr_NCryptGetProperty)(
	__in    NCRYPT_HANDLE hObject,
	__in    LPCWSTR pszProperty,
	__out_bcount_part_opt(cbOutput, *pcbResult) PBYTE pbOutput,
	__in    DWORD   cbOutput,
	__out   DWORD * pcbResult,
	__in    DWORD   dwFlags);

HRESULT TPMGetEKPub(_Outptr_result_bytebuffer_(*pcbEKPub) PBYTE *ppbEKPub, _Out_ DWORD *pcbEKPub)
{
	*ppbEKPub = nullptr;
	HRESULT hr = S_OK;

	HMODULE hDllHandle = LoadLibrary(L"Ncrypt.dll");
	if (hDllHandle == nullptr)
	{
		hr = HRESULT_FROM_WIN32(ERROR_NOINTERFACE);
	}

	FuncPtr_NCryptOpenStorageProvider pNCryptOpenStorageProvider = nullptr;
	FuncPtr_NCryptGetProperty pNCryptGetProperty = nullptr;
	FuncPtr_NCryptFreeObject pNCryptFreeObject = nullptr;

	if (SUCCEEDED(hr))
	{
		pNCryptOpenStorageProvider = (FuncPtr_NCryptOpenStorageProvider)GetProcAddress(hDllHandle, "NCryptOpenStorageProvider");
		pNCryptGetProperty = (FuncPtr_NCryptGetProperty)GetProcAddress(hDllHandle, "NCryptGetProperty");
		pNCryptFreeObject = (FuncPtr_NCryptFreeObject)GetProcAddress(hDllHandle, "NCryptFreeObject");
		if (pNCryptOpenStorageProvider == nullptr
			|| pNCryptGetProperty == nullptr
			|| pNCryptFreeObject == nullptr)
		{
			hr = HRESULT_FROM_WIN32(ERROR_NOINTERFACE);
		}
	}

	NCRYPT_PROV_HANDLE hProv = NULL;
	BYTE pbEKPub[1024] = { 0 };
	DWORD cbEKPub = 0;

	if (SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(pNCryptOpenStorageProvider(
			&hProv,
			MS_PLATFORM_CRYPTO_PROVIDER,
			0));
	}

	if (SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(pNCryptGetProperty(hProv,
			NCRYPT_PCP_EKPUB_PROPERTY,
			pbEKPub,
			sizeof(pbEKPub),
			&cbEKPub,
			0));
	}

	if (SUCCEEDED(hr))
	{
		*ppbEKPub = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbEKPub));
		if (*ppbEKPub == nullptr)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(memcpy_s(*ppbEKPub, cbEKPub, pbEKPub, cbEKPub));
	}

	if (SUCCEEDED(hr))
	{
		*pcbEKPub = cbEKPub;
	}

	// Cleanup
	if (hProv != NULL)
	{
		pNCryptFreeObject(hProv);
		hProv = NULL;
	}

	if (*ppbEKPub != nullptr && FAILED(hr))
	{
		HeapFree(GetProcessHeap(), 0, *ppbEKPub);
		*ppbEKPub = nullptr;
	}
	return hr;
}

HRESULT TPMSha256HashWithSalt(_In_ PBYTE pbTPMKey, _In_ DWORD cbTPMKey, _In_ PSTR pszSalt, _Out_ SHA256_HASHVAL* pTPMID)
{
	DWORD cbSalt = static_cast<DWORD>(strlen(pszSalt));
	DWORD cbBufferSize = cbTPMKey + cbSalt;
	PBYTE pbBuffer = new BYTE[cbBufferSize];

	HRESULT hr = HRESULT_FROM_WIN32(memcpy_s(pbBuffer, cbBufferSize, pbTPMKey, cbTPMKey));

	if (SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(memcpy_s(pbBuffer + cbTPMKey, cbBufferSize - cbTPMKey, pszSalt, cbSalt));
		if (SUCCEEDED(hr))
		{
			hr = SHA256HashData(pbBuffer, cbBufferSize, pTPMID);
		}
	}

	if (pbBuffer != NULL)
	{
		delete[] pbBuffer;
	}
	return hr;
}

HRESULT TPMGetSaltedID(_Outptr_result_bytebuffer_(*pcbTPMID) PBYTE *ppbTPMID, _Out_ DWORD *pcbTPMID, _In_ PSTR pszSalt)
{
	*ppbTPMID = nullptr;
	PBYTE pbEKPub = nullptr;
	DWORD cbEKPub = 0;
	SHA256_HASHVAL hashedTPMID;

	HRESULT hr = TPMGetEKPub(&pbEKPub, &cbEKPub);

	if (SUCCEEDED(hr))
	{
		hr = TPMSha256HashWithSalt(pbEKPub, cbEKPub, pszSalt, &hashedTPMID);
	}
	else
	{
		pbEKPub = nullptr;
	}

	if (SUCCEEDED(hr))
	{
		*ppbTPMID = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(hashedTPMID.data)));
		if (*ppbTPMID == nullptr)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(memcpy_s(*ppbTPMID, sizeof(hashedTPMID.data), hashedTPMID.data, sizeof(hashedTPMID.data)));
	}

	if (SUCCEEDED(hr))
	{
		*pcbTPMID = sizeof(hashedTPMID.data);
	}

	// Cleanup
	// Make sure to make pbEKPub nullptr when TPMGetEKPub to initialize pbEKPub
	if (pbEKPub != nullptr)
	{
		HeapFree(GetProcessHeap(), 0, pbEKPub);
		pbEKPub = nullptr;
	}

	if (*ppbTPMID != nullptr && FAILED(hr))
	{
		HeapFree(GetProcessHeap(), 0, *ppbTPMID);
		*ppbTPMID = nullptr;
	}

	return hr;
}

HRESULT TPMGetID(_Outptr_result_bytebuffer_(*pcbTPMID) PBYTE *ppbTPMID, _Out_ DWORD *pcbTPMID)
{
	return TPMGetSaltedID(ppbTPMID, pcbTPMID, TPM_HASH_SALT);
}
