#include "stdafx.h"
#include "newsha256.h"
#include <wincrypt.h>
#include <Windows.h>

HRESULT SHA256HashData(__in_bcount(cbHashDataLength) BYTE *pbHashData, __in DWORD cbHashDataLength, __out SHA256_HASHVAL *pHashValue)
{
    HCRYPTPROV hcryptprov;
    BOOL fReturn = TRUE;
    fReturn = CryptAcquireContextW(&hcryptprov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    HRESULT hr = E_FAIL;
    if (hcryptprov != NULL && fReturn == TRUE)
    {
        HCRYPTHASH hash = NULL;
        if (CryptCreateHash(hcryptprov, CALG_SHA_256, 0, 0, &hash))
        {
            if (CryptHashData(hash, pbHashData, cbHashDataLength, 0))
            {
                DWORD dwHashValSize = sizeof(pHashValue->data);
                if (CryptGetHashParam(hash, HP_HASHVAL, pHashValue->data, &dwHashValSize, 0))
                {
                    hr = S_OK;
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
            CryptDestroyHash(hash);
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
    if (hcryptprov != NULL)
    {
        CryptReleaseContext(hcryptprov, 0);
    }
    return hr;
}

HRESULT SHA256HashData(__in PWSTR pszHashData, __out SHA256_HASHVAL *pHashValue)
{
	return SHA256HashData((BYTE *)pszHashData, static_cast<DWORD>(wcslen(pszHashData) * sizeof(WCHAR)), pHashValue);
}
