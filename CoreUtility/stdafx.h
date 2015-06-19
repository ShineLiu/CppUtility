// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifdef UNIT_TESTING
#include <SystemCallMockup.h>
#define PRIVATE_UT_PUBLIC public
#define PROTECTED_UT_PUBLIC public
#define NONUT_STATIC
#else
#define PRIVATE_UT_PUBLIC private
#define PROTECTED_UT_PUBLIC protected
#define NONUT_STATIC static
#endif
