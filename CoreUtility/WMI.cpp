#include "stdafx.h"
#include "WMI.h"
#include <Windows.h>
#include <WbemIdl.h>
#include <strsafe.h>
#include <Shlwapi.h>

#define WQL L"WQL"
#define SELETE_FROM L"SELECT * FROM "
#define MAX_QUERY_NAME 1024

WMI::WMI()
{
    _pLoc = NULL;
    _pSvc = NULL;
    _pEnumerator = NULL;
    _pclsObj = NULL;
    _fComInitResult = FALSE;
}
WMI::~WMI()
{
    if (_pLoc != NULL)
    {
        _pLoc->Release();
        _pLoc = NULL;
    }
    if (_pEnumerator != NULL)
    {
        _pEnumerator->Release();
        _pEnumerator = NULL;
    }
    if (_pSvc != NULL)
    {
        _pSvc->Release();
        _pSvc = NULL;
    }    
    if (_pclsObj != NULL)
    {
        _pclsObj->Release();
        _pclsObj = NULL;
    }
    if (_fComInitResult == TRUE)
    {
        CoUninitialize();
        _fComInitResult = FALSE;
    }
}

HRESULT WMI::ConnectServer(__in PWSTR pszNamespacePath)
{
    HRESULT hr = S_OK;
    // Step 1: Initialize COM
    if (_fComInitResult == FALSE)
    {
        hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    }
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE)
    {
        _fComInitResult = TRUE;
        // Step 2: Set general COM security levels
        hr = CoInitializeSecurity(
            NULL,
            -1,
            NULL,
            NULL,
            RPC_C_AUTHN_LEVEL_NONE,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE,
            NULL
            );
        if (SUCCEEDED(hr) || hr == RPC_E_TOO_LATE)
        {
            // Step 3: Obtain the initial locator to WMI
            if (_pLoc != NULL)
            {
                _pLoc->Release();
                _pLoc = NULL;
            }
            hr = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID *) &_pLoc
                );
            if (SUCCEEDED(hr))
            {
                // Step 4: Connect to WMI through the IWbemLocator::ConnectServer method
                if (_pSvc != NULL)
                {
                    _pSvc->Release();
                    _pSvc = NULL;
                }
                hr = _pLoc->ConnectServer(
                    pszNamespacePath,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    &_pSvc
                    );
                if (SUCCEEDED(hr))
                {
                    // Step 5: Set security levels on the proxy
                    hr = CoSetProxyBlanket(
                        _pSvc,
                        RPC_C_AUTHN_WINNT,
                        RPC_C_AUTHZ_NONE,
                        NULL,
                        RPC_C_AUTHN_LEVEL_CALL,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        EOAC_NONE
                        );
                }
            }
        }
    }
    return hr;
}

HRESULT WMI::GetQueryName(__out PWSTR *ppszQuery, __in PCWSTR pszProductName)
{
	DWORD dwQuerySize = static_cast<DWORD>(wcslen(SELETE_FROM) + wcsnlen_s(pszProductName, MAX_QUERY_NAME) + 1);
    *ppszQuery = new WCHAR[dwQuerySize];
    HRESULT hr = StringCchCopyW(*ppszQuery, dwQuerySize, SELETE_FROM);
    if (SUCCEEDED(hr))
    {
        hr = StringCchCatW(*ppszQuery, dwQuerySize, pszProductName);
    }
    return hr;
}

HRESULT WMI::ExecQuery(__in PWSTR pszProductName)
{
    HRESULT hr = S_OK;
    PWSTR pszQuery = NULL;

    if (_pEnumerator != NULL)
    {
        _pEnumerator->Release();
        _pEnumerator = NULL;
    }
    hr = GetQueryName(&pszQuery, pszProductName);
    if (SUCCEEDED(hr))
    {
        // Step 6: Use the IWbemServices pointer to make requests of WMI
        if (_pSvc != NULL)
        {
            hr = _pSvc->ExecQuery(
                WQL,
                pszQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &_pEnumerator);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    if (pszQuery != NULL)
    {
        delete[] pszQuery;
    }
    return hr;
}

HRESULT WMI::ExecQuery(__in PWSTR pszNamespacePath, __in PWSTR pszProductName)
{
    HRESULT hr = S_OK;
    hr = ConnectServer(pszNamespacePath);
    if (SUCCEEDED(hr))
    {
        hr = ExecQuery(pszProductName);
    }
    return hr;
}

HRESULT WMI::NextObject(__out BOOL *pfEnd)
{
    ULONG uReturn = 0;
    HRESULT hr = S_OK;
    
    if (_pEnumerator)
    {
        if (_pclsObj != NULL)
        {
            _pclsObj->Release();
            _pclsObj = NULL;
        }
        hr = _pEnumerator->Next(WBEM_INFINITE, 1, &_pclsObj, &uReturn);
        if (0 == uReturn)
        {
            *pfEnd = TRUE;
        }
        else
        {
            *pfEnd = FALSE;
        }
    }
    else
    {
        hr = S_OK;
        *pfEnd = TRUE;
    }
    return hr;
}

HRESULT WMI::ResetObject()
{
    HRESULT hr = S_OK;

    if (_pEnumerator != NULL)
    {
        hr = _pEnumerator->Reset();
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT WMI::GetCurrentObjectPoperty(__in PWSTR pszPopertyName, __out VARIANT *pVal)
{
    return _pclsObj->Get(pszPopertyName, 0, pVal, 0, 0);
}

HRESULT WMI::GetCurrentObjectPoperty(__in PWSTR pszNamespacePath, __in PWSTR pszProductName, __in PWSTR pszPopertyName, __out VARIANT *pVal)
{
    HRESULT hr = S_OK;
    hr = ExecQuery(pszNamespacePath, pszProductName);
    if (SUCCEEDED(hr))
    {
        BOOL fEnd = TRUE;
        hr = NextObject(&fEnd);
        if (SUCCEEDED(hr) && fEnd == FALSE)
        {
            hr = GetCurrentObjectPoperty(pszPopertyName, pVal);
        }
        else
        {
            // NextObject() returns S_FALSE when next object not found and _pclsObj remains NULL, in this case
            // we report E_FAIL here so that the outside would not treat us as succeeded when using SUCCEEDED(hr)
            hr = E_FAIL;
        }
    }
    return hr;
}

BOOL WMI::IsBadDisplayName(__in PWSTR pszDisplayName)
{
    return pszDisplayName[0] == L'?';
}

HRESULT WMI::AlloString(__in PCWSTR pszDataSource, __out PWSTR *ppszDest)
{
	DWORD dwDestLength = static_cast<DWORD>(wcslen(pszDataSource) + 1);
    *ppszDest = new WCHAR[dwDestLength];
    return StringCchCopyW(*ppszDest, dwDestLength, pszDataSource);
}

HRESULT WMI::TrytoCorrectDisplayName(__inout PWSTR *ppszDisplayName, __in_ecount(cbPathNum) PWSTR *ppszPath, __in DWORD cbPathNum)
{
    VARIANT val; 
    HRESULT hr = E_FAIL;
    PWSTR pszOriginal = NULL;
    
    pszOriginal = *ppszDisplayName;
    *ppszDisplayName = NULL;
    VariantInit(&val);
    for (DWORD index = 0; index < cbPathNum && FAILED(hr); index++)
    {
        hr = GetCurrentObjectPoperty(ppszPath[index], &val);
        if (SUCCEEDED(hr))
        {
            PWSTR pszPath = NULL;
            hr = ConvertVariantToWSTR(&val, &pszPath);
            VariantClear(&val);
            if (SUCCEEDED(hr))
            {
                PWSTR pszName = NULL;
                pszName = PathFindFileNameW(pszPath);
                hr = AlloString(pszName, ppszDisplayName);
                if (pszPath != NULL)
                {
                    delete[] pszPath;
                }
            }
        }
    }    
    if (SUCCEEDED(hr))
    {
        delete[] pszOriginal;
    }
    else
    {
        *ppszDisplayName = pszOriginal;
        hr = S_OK;
    }
    return hr;
}

HRESULT WMI::ConvertVariantToWSTR(__in VARIANT *pValInfo, __out PWSTR *ppszInfo)
{
    HRESULT hr = S_OK;
    DWORD dwDestSize = 0;
    switch(V_VT(pValInfo))
    {
    case VT_NULL:
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        break;
    case VT_BSTR:
		dwDestSize = static_cast<DWORD>(wcslen(pValInfo->bstrVal) + 1);
        *ppszInfo = new WCHAR[dwDestSize];
        memset((void*) *ppszInfo, 0, dwDestSize *sizeof(WCHAR));
        if (wcscpy_s(*ppszInfo, dwDestSize, pValInfo->bstrVal) != 0)
        {
            hr = E_FAIL;
        }
        break;
    default:
        hr = E_FAIL;
        break;
    }
    return hr;
}

HRESULT WMI::ConvertVariantToBOOL(__in VARIANT *pValInfo, __out BOOL *pfInfo)
{
    HRESULT hr = S_OK;

    switch(V_VT(pValInfo))
    {
    case VT_NULL:
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        break;
    case VT_BOOL:
        *pfInfo = (pValInfo->boolVal == VARIANT_FALSE) ? FALSE : TRUE;
        break;
    default:
        hr = E_FAIL;
        break;
    }
    return hr;
}

HRESULT WMI::ConvertVariantToDWORD(__in VARIANT *pValInfo, __out DWORD *pdwInfo)
{
    HRESULT hr = S_OK;

    switch(V_VT(pValInfo))
    {
    case VT_NULL:
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        break;
    case VT_I4:
        *pdwInfo = pValInfo->lVal;
        break;
    default:
        hr = E_FAIL;
        break;
    }
    return hr;
}

HRESULT WMI::ConvertVariantToCStringW(__in VARIANT *pValInfo, __out CStringW *pcsInfo)
{
    HRESULT hr = S_OK;

    switch (V_VT(pValInfo))
    {
    case VT_NULL:
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        break;
    case VT_BSTR:
        *pcsInfo = pValInfo->bstrVal;
        break;
    default:
        hr = E_FAIL;
        break;
    }
    return hr;
}