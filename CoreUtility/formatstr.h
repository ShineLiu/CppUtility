#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Lightweight in-place format shim
// If the result string is short enough, it will be allocated on stack.
// Otherwise, it will be allocated on heap and deallocated when CFormatStr leaves its scope.

class CFormatStr
{
public:
    CFormatStr(__in __format_string __nullterminated const wchar_t *pszFormat, ...)
    {
        _szFixedBuffer[0] = L'\0';
        va_list args;
        va_start(args, pszFormat);
        int nRequiredSize = _vscwprintf(pszFormat, args);
        if (nRequiredSize < _countof(_szFixedBuffer))
        {
            _pBuffer = _szFixedBuffer;
        }
        else
        {
            _pBuffer = new wchar_t[nRequiredSize + 1];
        }
        _vsnwprintf_s(_pBuffer, nRequiredSize + 1, _TRUNCATE, pszFormat, args);
    }
    ~CFormatStr()
    {
        if (_pBuffer != _szFixedBuffer)
        {
            delete [] _pBuffer;
            _pBuffer = NULL;
        }
    }

    operator const wchar_t*() const
    {
        return _pBuffer;
    }
    const wchar_t* operator&() const
    {
        return _pBuffer;
    }

private:
    wchar_t *_pBuffer;
    wchar_t _szFixedBuffer[256];
};
