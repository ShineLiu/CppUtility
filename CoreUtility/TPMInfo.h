#define TPMINFO_NAME L"TPMInfo"
#define TPMINFO_VERSION 1

HRESULT TPMGetID(_Outptr_result_bytebuffer_(*pcbTPMID) PBYTE *ppbTPMID, _Out_ DWORD *pcbTPMID);