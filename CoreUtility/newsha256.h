#include <Windows.h>

#pragma pack(push, 1)

typedef struct SHA256_HASHVAL
{
    BYTE data[32];
} SHA256_HASHVAL;

#pragma pack(pop)

HRESULT SHA256HashData(__in_bcount(cbHashDataLength) BYTE *pbHashData, __in DWORD cbHashDataLength, __out SHA256_HASHVAL *pHashValue);

HRESULT SHA256HashData(__in PWSTR pszHashData, __out SHA256_HASHVAL *pHashValue);