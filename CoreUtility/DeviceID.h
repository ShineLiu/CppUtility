#include <Windows.h>

#define DEVICEID_NAME L"DeviceID"
#define DEVICEID_VERSION 2
#define MAX_UUID_LEN 64

// WMI needed
#define CIMV2_NAMESPACE L"root\\CIMV2"
#define COMPUTERSYSTEMPRODUCT L"Win32_ComputerSystemProduct"
#define UUID L"UUID"
#define DiskDrive L"Win32_DiskDrive"
#define SERIALNUMBER L"SerialNumber"
#define MEDIATYPE L"MediaType"

#ifdef __cplusplus
extern "C" {
namespace DeviceHealth
{
#endif

	typedef enum
    {
        ZONE_NUM = 5,
        ZONE_POLICY_ID_END = 7,
        SHA256INBYTE = 32
    } GlobalConstants;

	// DeviceID is computed based on Sha256 on a few sources in the device
    typedef struct _DeviceID
    {
        // If it is requested by a V1 client (DH_DATA_VERSION = 1), the header is always 0x00000001;
        // Otherwise,
        //     The highest 8 bits of the header is the DeviceID version
        //     The lowest 24 bits of the header is a bitmap where the Nth bit indicates whether DeviceIDType N 
        //     exists in the payload that follows
        DWORD dwheader; 
        BYTE bBasicDeviceID[SHA256INBYTE];
        BYTE bTPMDeviceID[SHA256INBYTE];
    } DeviceID, *PDeviceID;

	// Device ID Type
    // The ID could be derived based on a set of system parameters or TPM
    typedef enum
    {
        BASIC_DEVICE_ID = 0,
        TPM_DEVICE_ID = 1
    } DeviceIDType;

#ifdef __cplusplus
}
}
#endif

HRESULT CollectDeviceID(__out DeviceHealth::PDeviceID pDeviceID);

HRESULT DumpDeviceID(__in DeviceHealth::PDeviceID pDeviceID, __deref_out_bcount(*pcbData) PBYTE *ppbData, __out PDWORD pcbData);

HRESULT GetUUID(__deref_out PWSTR *ppszUUID);
HRESULT GetDiskSN(__deref_out PWSTR *ppszDiskSN);

HRESULT GetUUIDFromSmbios(__deref_out PWSTR *ppszUUID);
HRESULT GetDiskSNFromAPI(__deref_out PWSTR *ppszDiskSN);

HRESULT GetUUIDFromWMI(__deref_out PWSTR *ppszUUID);
HRESULT GetDiskSNFromWMI(__deref_out PWSTR *ppszDiskSN);