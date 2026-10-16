// shim_state.cpp : POSIX implementations of the Windows APIs declared in
// shim/windows.h / shim/io.h. Test-only (yact/supp/test_logging_host.py);
// never part of the product build.

#include "shim/windows.h"
#include "shim/io.h"
#include "shim/share.h"

#include <string>
#include <vector>
#include <mutex>

// ---------------------------------------------------------------------------
// Critical section

BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *Cs, DWORD SpinCount)
{
    (void) SpinCount;
    pthread_mutexattr_t Attr;
    if (pthread_mutexattr_init(&Attr) != 0)
        return FALSE;
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    int Rc = pthread_mutex_init(&Cs->Mutex, &Attr);
    pthread_mutexattr_destroy(&Attr);
    Cs->Initialized = (Rc == 0);
    return Cs->Initialized ? TRUE : FALSE;
}

void EnterCriticalSection(CRITICAL_SECTION *Cs)
{
    pthread_mutex_lock(&Cs->Mutex);
}

void LeaveCriticalSection(CRITICAL_SECTION *Cs)
{
    pthread_mutex_unlock(&Cs->Mutex);
}

BOOL TryEnterCriticalSection(CRITICAL_SECTION *Cs)
{
    return pthread_mutex_trylock(&Cs->Mutex) == 0 ? TRUE : FALSE;
}

void DeleteCriticalSection(CRITICAL_SECTION *Cs)
{
    pthread_mutex_destroy(&Cs->Mutex);
}

// ---------------------------------------------------------------------------
// One-time initialization

static pthread_mutex_t ShimOnceMutex = PTHREAD_MUTEX_INITIALIZER;

BOOL InitOnceExecuteOnce(INIT_ONCE *Once, PINIT_ONCE_FN Fn, PVOID Parameter, PVOID *Context)
{
    pthread_mutex_lock(&ShimOnceMutex);
    if (*Once == 0)
    {
        Fn(Once, Parameter, Context);
        *Once = 1;
    }
    pthread_mutex_unlock(&ShimOnceMutex);
    return TRUE;
}

// ---------------------------------------------------------------------------
// Time / ids / debug output

void GetLocalTime(SYSTEMTIME *St)
{
    struct timeval Tv;
    gettimeofday(&Tv, 0);
    struct tm Tm;
    time_t Sec = (time_t) Tv.tv_sec;
    localtime_r(&Sec, &Tm);
    St->wYear = (WORD) (Tm.tm_year + 1900);
    St->wMonth = (WORD) (Tm.tm_mon + 1);
    St->wDayOfWeek = (WORD) Tm.tm_wday;
    St->wDay = (WORD) Tm.tm_mday;
    St->wHour = (WORD) Tm.tm_hour;
    St->wMinute = (WORD) Tm.tm_min;
    St->wSecond = (WORD) Tm.tm_sec;
    St->wMilliseconds = (WORD) (Tv.tv_usec / 1000);
}

DWORD GetCurrentProcessId(void)
{
    return (DWORD) getpid();
}

static pthread_mutex_t ShimTidMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t ShimTidKey;
static pthread_once_t ShimTidOnce = PTHREAD_ONCE_INIT;
static DWORD ShimNextTid = 1;

static void ShimMakeTidKey(void)
{
    pthread_key_create(&ShimTidKey, 0);
}

DWORD GetCurrentThreadId(void)
{
    pthread_once(&ShimTidOnce, ShimMakeTidKey);
    DWORD Tid = (DWORD) (uintptr_t) pthread_getspecific(ShimTidKey);
    if (Tid == 0)
    {
        pthread_mutex_lock(&ShimTidMutex);
        Tid = ShimNextTid++;
        pthread_mutex_unlock(&ShimTidMutex);
        pthread_setspecific(ShimTidKey, (void *) (uintptr_t) Tid);
    }
    return Tid;
}

unsigned long long GetTickCount64(void)
{
    struct timespec Ts;
    clock_gettime(CLOCK_MONOTONIC, &Ts);
    return (unsigned long long) Ts.tv_sec * 1000ull + (unsigned long long) (Ts.tv_nsec / 1000000);
}

static std::mutex ShimOutputMutex;
static std::string ShimOutput;

void OutputDebugStringA(const char *Text)
{
    if (Text == 0)
        return;
    std::lock_guard<std::mutex> Lock(ShimOutputMutex);
    ShimOutput += Text;
    if (getenv("SHIM_ECHO_DEBUG") != 0)
        fputs(Text, stderr);
}

void ShimDebugOutputReset(void)
{
    std::lock_guard<std::mutex> Lock(ShimOutputMutex);
    ShimOutput.clear();
}

const char *ShimDebugOutputGet(void)
{
    // Only call when no logging thread is running (the tests do).
    return ShimOutput.c_str();
}

DWORD GetLastError(void)
{
    return (DWORD) errno;
}

// ---------------------------------------------------------------------------
// Module lookup by address

struct ShimModule
{
    unsigned long long Base;
    unsigned long long End;
    std::string Name;
    HMODULE Handle;
};

static std::vector<ShimModule> ShimModules;
static std::string ShimDefaultModule;
static const uintptr_t ShimDefaultHandle = 0x1111000;

void ShimSetDefaultModule(const char *Name)
{
    ShimDefaultModule = Name ? Name : "";
}

void ShimRegisterModule(unsigned long long Base, unsigned long long End, const char *Name)
{
    ShimModule M;
    M.Base = Base;
    M.End = End;
    M.Name = Name ? Name : "";
    // On Windows an HMODULE is the module base address; the logger relies on
    // that to compute "module+offset" for exception addresses.
    M.Handle = (HMODULE) (uintptr_t) Base;
    ShimModules.push_back(M);
}

BOOL GetModuleHandleExA(DWORD Flags, LPCSTR Address, HMODULE *Module)
{
    (void) Flags;
    unsigned long long A = (unsigned long long) (uintptr_t) Address;
    for (size_t i = 0; i < ShimModules.size(); i++)
        if (A >= ShimModules[i].Base && A < ShimModules[i].End)
        {
            *Module = ShimModules[i].Handle;
            return TRUE;
        }
    if (!ShimDefaultModule.empty())
    {
        *Module = (HMODULE) ShimDefaultHandle;
        return TRUE;
    }
    return FALSE;
}

DWORD GetModuleFileNameA(HMODULE Module, LPSTR FileName, DWORD Size)
{
    const std::string *Name = 0;
    if ((uintptr_t) Module == ShimDefaultHandle)
        Name = &ShimDefaultModule;
    else
        for (size_t i = 0; i < ShimModules.size(); i++)
            if (ShimModules[i].Handle == Module)
            {
                Name = &ShimModules[i].Name;
                break;
            }
    if (Name == 0 || Name->empty())
        return 0;
    snprintf(FileName, Size, "%s", Name->c_str());
    return (DWORD) strlen(FileName);
}

// ---------------------------------------------------------------------------
// Environment / files

DWORD GetEnvironmentVariableA(LPCSTR Name, LPSTR Buffer, DWORD Size)
{
    const char *Value = getenv(Name);
    if (Value == 0)
        return 0;
    size_t Len = strlen(Value);
    if (Len + 1 > Size)
        return (DWORD) (Len + 1);
    memcpy(Buffer, Value, Len + 1);
    return (DWORD) Len;
}

BOOL MoveFileExA(LPCSTR From, LPCSTR To, DWORD Flags)
{
    (void) Flags;
    return rename(From, To) == 0 ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// Unhandled exception filter

static SHIM_EXCEPTION_FILTER ShimFilter = 0;

SHIM_EXCEPTION_FILTER SetUnhandledExceptionFilter(SHIM_EXCEPTION_FILTER Filter)
{
    SHIM_EXCEPTION_FILTER Old = ShimFilter;
    ShimFilter = Filter;
    return Old;
}

SHIM_EXCEPTION_FILTER ShimGetExceptionFilter(void)
{
    return ShimFilter;
}

// ---------------------------------------------------------------------------
// CRT file functions (shim/io.h)

errno_t _sopen_s(int *File, const char *Path, int Flags, int Share, int Mode)
{
    (void) Share;
    int Fd = open(Path, Flags, Mode);
    if (Fd < 0)
    {
        if (File != 0)
            *File = -1;
        return errno ? errno : EINVAL;
    }
    *File = Fd;
    return 0;
}

int _write(int File, const void *Buffer, unsigned int Count)
{
    return (int) write(File, Buffer, Count);
}

int _commit(int File)
{
    return fsync(File) == 0 ? 0 : -1;
}

int _close(int File)
{
    return close(File);
}

long long _lseeki64(int File, long long Offset, int Whence)
{
    return (long long) lseek(File, (off_t) Offset, Whence);
}
