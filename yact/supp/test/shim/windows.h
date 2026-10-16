// windows.h shim: builds the real yact/supp/logging.cpp on the host
// (macOS/Linux) by providing the small Windows API surface it uses.
//
// This file is only used by yact/supp/test_logging_host.py and is never part
// of the product build. The logger implementation itself is compiled
// unmodified, so the tests exercise the production code.
//

#pragma once

#ifndef _WIN32

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int BOOL;
typedef int LONG;
typedef unsigned long ULONG;
typedef uintptr_t ULONG_PTR;
typedef int errno_t;
typedef void *PVOID;
typedef void *LPVOID;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef wchar_t WCHAR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;
typedef int (*FARPROC)(void);
typedef void *HMODULE;
typedef void *HANDLE;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PATH 260

#define WINAPI
#define CALLBACK

// ---------------------------------------------------------------------------
// Critical section (pthread recursive mutex; Windows critical sections are
// recursive too).

typedef struct _CRITICAL_SECTION
{
    pthread_mutex_t Mutex;
    int Initialized;
} CRITICAL_SECTION;

BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *Cs, DWORD SpinCount);
void EnterCriticalSection(CRITICAL_SECTION *Cs);
void LeaveCriticalSection(CRITICAL_SECTION *Cs);
BOOL TryEnterCriticalSection(CRITICAL_SECTION *Cs);
void DeleteCriticalSection(CRITICAL_SECTION *Cs);

// ---------------------------------------------------------------------------
// One-time initialization.

typedef int INIT_ONCE;
typedef INIT_ONCE *PINIT_ONCE;
typedef BOOL(CALLBACK *PINIT_ONCE_FN)(PINIT_ONCE Once, PVOID Parameter, PVOID *Context);

#define INIT_ONCE_STATIC_INIT 0

BOOL InitOnceExecuteOnce(INIT_ONCE *Once, PINIT_ONCE_FN Fn, PVOID Parameter, PVOID *Context);

// ---------------------------------------------------------------------------
// Time / ids / debug output.

typedef struct _SYSTEMTIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *St);
DWORD GetCurrentProcessId(void);
DWORD GetCurrentThreadId(void);
unsigned long long GetTickCount64(void);
void OutputDebugStringA(const char *Text);
DWORD GetLastError(void);

// ---------------------------------------------------------------------------
// Module lookup by address.

#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 0x00000002
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 0x00000004

BOOL GetModuleHandleExA(DWORD Flags, LPCSTR Address, HMODULE *Module);
DWORD GetModuleFileNameA(HMODULE Module, LPSTR FileName, DWORD Size);

// ---------------------------------------------------------------------------
// Registry / loader APIs used by supp.cpp. Needed so the host test can
// compile-check supp.cpp; they are not implemented (never called).

typedef unsigned int HKEY;
typedef BYTE *LPBYTE;

// Windows HKEY values are 32-bit handles; a 32-bit type also keeps the
// host-side cast checks (pointer -> DWORD) compiling like on Windows.
typedef void (*PIMAGE_TLS_CALLBACK)(PVOID, DWORD, PVOID);

#define HKEY_CURRENT_USER ((HKEY) 0x80000001u)
#define HKEY_LOCAL_MACHINE ((HKEY) 0x80000002u)
#define RRF_RT_REG_SZ 0x00000002
#define RRF_RT_DWORD 0x00000018
#define RRF_RT_ANY 0x0000FFFF
#define REG_SZ 1
#define REG_DWORD 4
#define KEY_ALL_ACCESS 0x000F003F
#define ERROR_SUCCESS 0

LONG RegGetValueA(HKEY Root, const char *SubKey, const char *Value, DWORD Flags,
                  DWORD *Type, void *Data, DWORD *DataSize);
LONG RegCreateKeyExA(HKEY Root, const char *SubKey, DWORD Reserved, char *Class,
                     DWORD Options, DWORD Access, void *Attr, HKEY *Key,
                     DWORD *Disposition);
LONG RegSetValueExA(HKEY Key, const char *Value, DWORD Reserved, DWORD Type,
                    const BYTE *Data, DWORD DataSize);
LONG RegCloseKey(HKEY Key);
HMODULE LoadLibraryA(const char *Name);
void *GetProcAddress(HMODULE Module, const char *Name);
HMODULE GetModuleHandleA(const char *Name);

// ---------------------------------------------------------------------------
// Secure CRT string functions used by the Win86emun sources (MSVC template
// overloads). Compile-check only.

#ifdef __cplusplus
template <size_t N>
int strcpy_s(char (&Dst)[N], const char *Src)
{
    if (N == 0)
        return -1;
    snprintf(Dst, N, "%s", Src);
    return 0;
}

template <size_t N>
int strcat_s(char (&Dst)[N], const char *Src)
{
    size_t Len = strlen(Dst);
    if (Len + strlen(Src) >= N)
        return -1;
    strcpy(Dst + Len, Src);
    return 0;
}

template <size_t N, typename... Args>
int sprintf_s(char (&Dst)[N], const char *Format, Args... A)
{
    return snprintf(Dst, N, Format, A...);
}
#endif

// ---------------------------------------------------------------------------
// Environment / files.

DWORD GetEnvironmentVariableA(LPCSTR Name, LPSTR Buffer, DWORD Size);
BOOL MoveFileExA(LPCSTR From, LPCSTR To, DWORD Flags);

#define MOVEFILE_REPLACE_EXISTING 0x00000001

// ---------------------------------------------------------------------------
// Unhandled exception filter.

#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct _EXCEPTION_RECORD
{
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    DWORD NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD;

typedef struct _EXCEPTION_POINTERS
{
    EXCEPTION_RECORD *ExceptionRecord;
    PVOID ContextRecord;
} EXCEPTION_POINTERS;

#define EXCEPTION_CONTINUE_SEARCH 0

typedef LONG(WINAPI *SHIM_EXCEPTION_FILTER)(EXCEPTION_POINTERS *Info);

SHIM_EXCEPTION_FILTER SetUnhandledExceptionFilter(SHIM_EXCEPTION_FILTER Filter);

// ---------------------------------------------------------------------------
// Test-only helpers implemented in shim_state.cpp.

// Debug output capture.
void ShimDebugOutputReset(void);
const char *ShimDebugOutputGet(void);

// Module name resolution used by GetModuleHandleExA/GetModuleFileNameA.
void ShimSetDefaultModule(const char *Name);
void ShimRegisterModule(unsigned long long Base, unsigned long long End, const char *Name);

// The filter installed by the code under test.
SHIM_EXCEPTION_FILTER ShimGetExceptionFilter(void);

#endif // _WIN32
