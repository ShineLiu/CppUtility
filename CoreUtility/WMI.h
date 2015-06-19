#include <Windows.h>
#include <OaIdl.h>
#include <Wbemidl.h>
#include <atlstr.h>

class WMI
{
public:
    HRESULT ConnectServer(__in PWSTR pszNamespacePath);
    HRESULT ExecQuery(__in PWSTR pszProductName);
    HRESULT ExecQuery(__in PWSTR pszNamespacePath, __in PWSTR pszProductName);
    HRESULT NextObject(__out BOOL *pfEnd);
    HRESULT ResetObject();
    HRESULT GetCurrentObjectPoperty(__in PWSTR pszPopertyName, __out VARIANT *pVal);
    HRESULT GetCurrentObjectPoperty(__in PWSTR pszNamespacePath, __in PWSTR pszProductName, __in PWSTR pszPopertyName, __out VARIANT *pVal);
    HRESULT ConvertVariantToWSTR(__in VARIANT *pValInfo, __out PWSTR *ppszInfo);
    HRESULT ConvertVariantToBOOL(__in VARIANT *pValInfo, __out BOOL *pfInfo);
    HRESULT ConvertVariantToDWORD(__in VARIANT *pValInfo, __out DWORD *pdwInfo);
    HRESULT ConvertVariantToCStringW(__in VARIANT *pValInfo, __out CStringW *pcsInfo);
    BOOL IsBadDisplayName(__in PWSTR pszDisplayName);
    HRESULT TrytoCorrectDisplayName(__inout PWSTR *ppszDisplayName, __in_ecount(cbPathNum) PWSTR *ppszPath, __in DWORD cbPathNum);
    WMI();
    ~WMI();

PRIVATE_UT_PUBLIC:
    HRESULT GetQueryName(__out PWSTR *ppszQuery, __in PCWSTR pszProductName);
    HRESULT AlloString(__in PCWSTR pszDataSource, __out PWSTR *ppszDest);
    IWbemLocator *_pLoc;
    IWbemServices *_pSvc;
    IEnumWbemClassObject *_pEnumerator;
    IWbemClassObject *_pclsObj;
    BOOL _fComInitResult;
};