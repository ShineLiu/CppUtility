#include <devicehealth.h>
using namespace DeviceHealth;
#define OSVERSION_NAME L"OSVersion"
#define OSVERSION_VERSION 1
extern GUID IID_OSVERSION;

DWORD GetServicepackVersion(const OSVERSIONINFOEXW osvi);
HRESULT CollectSystemVersionInfo(__deref_out_bcount(*pcbBuffer) PVOID *ppBuffer, __out PDWORD pcbBuffer);

HRESULT CollectOSVersionInfo(__out POSVersionInfo pOSVersionInfo);

HRESULT DumpOSVersionInfo(__in POSVersionInfo pOSVersionInfo, __deref_out_bcount(*pcbData) PBYTE *ppbData, __out PDWORD pcbData);

void FreeOSVersion(__in POSVersionInfo pOSVersionInfo);

HRESULT GetOSVersionInfoEx(__out OSVERSIONINFOEXW *pOSVersionInfoEx);

BOOL IsWindows64bit();
