// logging.cpp : Common debug logging for Win86emun
//
// Single process-wide logger, implemented in supp.nt.dll and shared by all
// Win86emun components through their existing imports from supp.nt.dll.
//
// Log line format (one line per event, '\n' terminated):
//   2026-10-16 21:03:11.482 [INFO][pid=8123 tid=4412][dosbox_emu.dll] message
//
// Levels (registry HKCU/HKLM\Software\x86node\Settings\LogLevel, default 2):
//   0=off 1=ERROR 2=WARN 3=INFO 4=DEBUG 5=TRACE
//
// Log file: registry "LogFile" (REG_SZ) if set, otherwise %TEMP%\win86emu.log.
// Rotation: registry "LogMaxSizeKB" (default 8192, 0 disables). One previous
// file is kept as <file>.1.
//
// Thread safety: one CRITICAL_SECTION protects the file handle, the path, the
// size and the module-name cache. Every line is written with a single _write,
// so lines from different threads (and, in append mode, from different
// processes) do not interleave. Logger errors never stop emulation: when the
// file cannot be used, messages go to OutputDebugStringA and the failure
// reason is reported there and through LogGetFileError().
//

#include "stdafx.h"
#include "logging.h"

#define UTIL_EXPORT __declspec(dllexport)
#include <util.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <intrin.h>
#define LOG_RETURN_ADDRESS() _ReturnAddress()
#define LOG_THREAD_LOCAL __declspec(thread)
#else
#define LOG_RETURN_ADDRESS() __builtin_return_address(0)
#define LOG_THREAD_LOCAL __thread
#endif

// Log levels; the values are also the registry LogLevel values.
#define LOG_LEVEL_OFF    0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4
#define LOG_LEVEL_TRACE  5
#define LOG_LEVEL_MAX    5

#define LOG_MSG_MAX        1024
#define LOG_LINE_MAX       1400
#define LOG_MODULE_MAX     48
#define LOG_MODULE_CACHE   24
#define LOG_DEFAULT_MAX_KB 8192

static INIT_ONCE LogInitOnce = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION LogLock;
static volatile LONG LogLockReady = 0;

static char LogPath[MAX_PATH];
static bool LogPathBuilt = false;
static int LogFile = -1;
static bool LogOpenFailed = false;
static bool LogShutdown = false;
static bool LogRotateWarned = false;
static unsigned long LogFileError = 0;
static long long LogSize = 0;

// Cached registry settings. The races here are benign: every thread writes the
// same value, and a repeated registry read is harmless.
static volatile LONG LogLevelCache = -1;
static volatile LONG LogMaxKBCache = -1;

struct LogModuleEntry
{
    HMODULE Handle;
    char Name[LOG_MODULE_MAX];
};
static LogModuleEntry LogModules[LOG_MODULE_CACHE];
static int LogModuleCount = 0;
static int LogModuleNext = 0;

static LOG_THREAD_LOCAL int LogInLogger = 0;
static LOG_THREAD_LOCAL unsigned long long LogEmuStartTick = 0;

static void LogEmit(int Level, void *Caller, const char *Format, va_list Args);

// ---------------------------------------------------------------------------

static const char *LogLevelName(int Level)
{
    switch (Level)
    {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_TRACE: return "TRACE";
        default:              return "?";
    }
}

static unsigned long long LogTickNow(void)
{
    return (unsigned long long) GetTickCount64();
}

// Runs once per process; the CRITICAL_SECTION lives until process exit.
static BOOL CALLBACK LogInitOnceProc(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context)
{
    if (InitializeCriticalSectionAndSpinCount(&LogLock, 0x400))
        LogLockReady = 1;
    return TRUE;
}

static void LogEnsureInit(void)
{
    if (LogLockReady)
        return;
    InitOnceExecuteOnce(&LogInitOnce, LogInitOnceProc, 0, 0);
}

// ---------------------------------------------------------------------------
// Registry-backed settings (cached after the first read).

static int LogGetLevelCached(void)
{
    LONG Level = LogLevelCache;
    if (Level < 0)
    {
        int Value = CfgGetInt("LogLevel", LOG_LEVEL_WARN);
        if (Value < LOG_LEVEL_OFF)
            Value = LOG_LEVEL_OFF;
        if (Value > LOG_LEVEL_MAX)
            Value = LOG_LEVEL_MAX;
        LogLevelCache = Value;
        Level = LogLevelCache;
    }
    return (int) Level;
}

static int LogGetMaxKBCached(void)
{
    LONG MaxKB = LogMaxKBCache;
    if (MaxKB < 0)
    {
        int Value = CfgGetInt("LogMaxSizeKB", LOG_DEFAULT_MAX_KB);
        if (Value < 0)
            Value = 0;
        if (Value > 1024 * 1024)
            Value = 1024 * 1024;
        LogMaxKBCache = Value;
        MaxKB = LogMaxKBCache;
    }
    return (int) MaxKB;
}

// ---------------------------------------------------------------------------
// Log file path. Must be called with LogLock held.

static bool LogAppendStr(char *Buffer, size_t Size, size_t *Used, const char *Text)
{
    size_t Len = strlen(Text);
    if (*Used + Len >= Size)
        return false;
    memcpy(Buffer + *Used, Text, Len + 1);
    *Used += Len;
    return true;
}

static void LogBuildPathLocked(void)
{
    if (LogPathBuilt)
        return;
    LogPathBuilt = true;
    LogPath[0] = 0;

    // Default: %TEMP%\win86emu.log (writable on Windows RT for the app).
    char Temp[MAX_PATH];
    DWORD TempLen = GetEnvironmentVariableA("TEMP", Temp, MAX_PATH);
    if (TempLen > 0 && TempLen < (DWORD) MAX_PATH)
    {
        size_t Used = 0;
        if (!LogAppendStr(LogPath, sizeof(LogPath), &Used, Temp))
            LogPath[0] = 0;
        else
        {
            if (Used > 0 && Temp[Used - 1] != '\\' && Temp[Used - 1] != '/')
                LogAppendStr(LogPath, sizeof(LogPath), &Used, "\\");
            if (!LogAppendStr(LogPath, sizeof(LogPath), &Used, "win86emu.log"))
                LogPath[0] = 0;
        }
    }

    // Existing Win86emun setting; on success it replaces the %TEMP% default.
    CfgGetString("LogFile", LogPath, MAX_PATH);
    LogPath[MAX_PATH - 1] = 0;  // RegGetValueA may leave no terminator on ERROR_MORE_DATA

    if (LogPath[0] == 0)
    {
        size_t Used = 0;
        if (!LogAppendStr(LogPath, sizeof(LogPath), &Used, "win86emu.log"))
            LogPath[0] = 0;
    }
}

// ---------------------------------------------------------------------------
// Caller module name cache. Must be called with LogLock held.

static const char *LogModuleNameLocked(void *Address)
{
    if (Address == 0)
        return "?";

    HMODULE Module = 0;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR) Address, &Module) || Module == 0)
        return "?";

    for (int i = 0; i < LogModuleCount; i++)
        if (LogModules[i].Handle == Module)
            return LogModules[i].Name;

    char Full[MAX_PATH];
    if (GetModuleFileNameA(Module, Full, MAX_PATH) == 0)
        return "?";

    const char *Base = strrchr(Full, '\\');
    if (Base == 0)
        Base = strrchr(Full, '/');
    Base = Base ? Base + 1 : Full;

    int Slot;
    if (LogModuleCount < LOG_MODULE_CACHE)
        Slot = LogModuleCount++;
    else
    {
        Slot = LogModuleNext;
        LogModuleNext = (LogModuleNext + 1) % LOG_MODULE_CACHE;
    }
    LogModules[Slot].Handle = Module;
    _snprintf_s(LogModules[Slot].Name, sizeof(LogModules[Slot].Name), _TRUNCATE, "%s", Base);
    LogModules[Slot].Name[sizeof(LogModules[Slot].Name) - 1] = 0;
    return LogModules[Slot].Name;
}

// Module+offset for a code address, without touching the module cache (used
// from the unhandled exception filter).
static void LogFormatAddress(void *Address, char *Out, size_t OutLen)
{
    HMODULE Module = 0;
    if (Address != 0 &&
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR) Address, &Module) && Module != 0)
    {
        char Full[MAX_PATH];
        const char *Base = "?";
        if (GetModuleFileNameA(Module, Full, MAX_PATH) != 0)
        {
            Base = strrchr(Full, '\\');
            if (Base == 0)
                Base = strrchr(Full, '/');
            Base = Base ? Base + 1 : Full;
        }
        _snprintf_s(Out, OutLen, _TRUNCATE, "%s+0x%lX", Base,
                    (unsigned long) ((unsigned long long) (size_t) Address -
                                     (unsigned long long) (size_t) Module));
    }
    else
        _snprintf_s(Out, OutLen, _TRUNCATE, "0x%p", Address);
}

// ---------------------------------------------------------------------------
// File handling. All of these must be called with LogLock held.

static int LogWriteRawLocked(const char *Data, size_t Len)
{
    if (LogFile < 0)
        return -1;

    int Written = _write(LogFile, Data, (unsigned int) Len);
    if (Written < 0 || (size_t) Written != Len)
    {
        LogFileError = (unsigned long) errno;
        if (LogFileError == 0)
            LogFileError = (unsigned long) GetLastError();
        return -1;
    }
    LogSize += (long long) Written;
    return Written;
}

static void LogWriteHeaderLocked(void)
{
    if (LogFile < 0 || LogSize != 0)
        return;

    SYSTEMTIME St;
    GetLocalTime(&St);
    char Line[512];
    _snprintf_s(Line, sizeof(Line), _TRUNCATE,
        "=== Win86emun debug log ===\n"
        "started: %04d-%02d-%02d %02d:%02d:%02d.%03d, pid=%lu, version %s\n"
        "file: %s, LogLevel=%d, LogMaxSizeKB=%d\n"
        "format: <date> <time> [LEVEL][pid=N tid=N][module] message\n",
        (int) St.wYear, (int) St.wMonth, (int) St.wDay,
        (int) St.wHour, (int) St.wMinute, (int) St.wSecond, (int) St.wMilliseconds,
        (unsigned long) GetCurrentProcessId(), EmGetVersionString(),
        LogPath, LogGetLevelCached(), LogGetMaxKBCached());
    LogWriteRawLocked(Line, strlen(Line));
}

static void LogOpenLocked(bool ExplicitRetry)
{
    if (LogFile >= 0)
        return;
    if (LogOpenFailed && !ExplicitRetry)
        return;

    LogBuildPathLocked();

    int File = -1;
    errno_t Err = _sopen_s(&File, LogPath, _O_RDWR | _O_APPEND | _O_CREAT,
                           SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (Err != 0 || File < 0)
    {
        LogOpenFailed = true;
        LogFileError = (unsigned long) (Err != 0 ? Err : errno);
        char Msg[512];
        _snprintf_s(Msg, sizeof(Msg), _TRUNCATE,
            "[supp.nt.dll] cannot open log file '%s' (errno=%lu, GetLastError=%lu); "
            "logging to debug output only\n",
            LogPath, LogFileError, (unsigned long) GetLastError());
        OutputDebugStringA(Msg);
        return;
    }

    LogFile = File;
    LogOpenFailed = false;
    LogFileError = 0;
    long long Size = _lseeki64(File, 0, SEEK_END);
    LogSize = Size > 0 ? Size : 0;
    LogWriteHeaderLocked();
}

static void LogRotateLocked(void)
{
    char Backup[MAX_PATH + 4];
    _snprintf_s(Backup, sizeof(Backup), _TRUNCATE, "%s.1", LogPath);

    int Old = LogFile;
    LogFile = -1;
    if (Old >= 0)
        _close(Old);

    if (MoveFileExA(LogPath, Backup, MOVEFILE_REPLACE_EXISTING))
    {
        LogSize = 0;
        LogOpenLocked(true);
        return;
    }

    // Rotation failed (for example because another process keeps the file
    // open). Keep appending instead of losing messages and retry after
    // another LogMaxSizeKB worth of data.
    LogFileError = (unsigned long) GetLastError();
    if (!LogRotateWarned)
    {
        LogRotateWarned = true;
        char Msg[512];
        _snprintf_s(Msg, sizeof(Msg), _TRUNCATE,
            "[supp.nt.dll] log rotation failed for '%s' (GetLastError=%lu); "
            "continuing to append\n", LogPath, LogFileError);
        OutputDebugStringA(Msg);
    }
    LogOpenLocked(true);
    LogSize = 0;
}

static bool LogWriteLineLocked(int Level, const char *Line, size_t Len)
{
    if (LogShutdown)
        return false;

    if (LogFile < 0)
    {
        LogOpenLocked(false);
        if (LogFile < 0)
            return false;
    }

    int MaxKB = LogGetMaxKBCached();
    if (MaxKB > 0 && LogSize + (long long) Len > (long long) MaxKB * 1024)
        LogRotateLocked();

    if (LogFile < 0)
        return false;

    if (LogWriteRawLocked(Line, Len) < 0)
    {
        char Msg[512];
        _snprintf_s(Msg, sizeof(Msg), _TRUNCATE,
            "[supp.nt.dll] log write failed (errno=%lu) for '%s'\n",
            LogFileError, LogPath);
        OutputDebugStringA(Msg);
        return false;
    }

    // Keep important events on disk; the rest stays in the OS cache and is
    // flushed on shutdown (see LOGGING.md).
    if (Level <= LOG_LEVEL_WARN)
        _commit(LogFile);
    return true;
}

// ---------------------------------------------------------------------------
// Line assembly and dispatch.

static void LogEmit(int Level, void *Caller, const char *Format, va_list Args)
{
    char Message[LOG_MSG_MAX];
    int Ret = _vsnprintf_s(Message, sizeof(Message), _TRUNCATE, Format, Args);
    if (Ret < 0 || Ret >= (int) sizeof(Message))
    {
        size_t Len = strlen(Message);
        if (Len > 3)
            memcpy(Message + Len - 3, "...", 3);
    }
    // The logger terminates every line itself; drop caller line breaks.
    size_t MsgLen = strlen(Message);
    while (MsgLen > 0 && (Message[MsgLen - 1] == '\n' || Message[MsgLen - 1] == '\r'))
        Message[--MsgLen] = 0;

    SYSTEMTIME St;
    GetLocalTime(&St);

    char Module[LOG_MODULE_MAX];
    _snprintf_s(Module, sizeof(Module), _TRUNCATE, "?");

    bool WroteToFile = false;

    LogEnsureInit();
    if (LogLockReady && !LogInLogger)
    {
        LogInLogger = 1;
        EnterCriticalSection(&LogLock);
        _snprintf_s(Module, sizeof(Module), _TRUNCATE, "%s", LogModuleNameLocked(Caller));

        char Line[LOG_LINE_MAX];
        int Prefix = _snprintf_s(Line, sizeof(Line), _TRUNCATE,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d [%s][pid=%lu tid=%lu][%s] ",
            (int) St.wYear, (int) St.wMonth, (int) St.wDay,
            (int) St.wHour, (int) St.wMinute, (int) St.wSecond, (int) St.wMilliseconds,
            LogLevelName(Level), (unsigned long) GetCurrentProcessId(),
            (unsigned long) GetCurrentThreadId(), Module);
        if (Prefix > 0 && Prefix < (int) sizeof(Line))
        {
            size_t Room = sizeof(Line) - (size_t) Prefix - 2;
            size_t Copy = MsgLen < Room ? MsgLen : Room;
            memcpy(Line + Prefix, Message, Copy);
            Line[Prefix + Copy] = '\n';
            Line[Prefix + Copy + 1] = 0;
            WroteToFile = LogWriteLineLocked(Level, Line, (size_t) Prefix + Copy + 1);
        }
        LeaveCriticalSection(&LogLock);
        LogInLogger = 0;
    }

    // Errors and warnings always reach the debug output; everything else only
    // when the file is not available.
    if (!WroteToFile || Level <= LOG_LEVEL_WARN)
    {
        char Line[LOG_LINE_MAX];
        _snprintf_s(Line, sizeof(Line), _TRUNCATE,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d [%s][pid=%lu tid=%lu][%s] %s\n",
            (int) St.wYear, (int) St.wMonth, (int) St.wDay,
            (int) St.wHour, (int) St.wMinute, (int) St.wSecond, (int) St.wMilliseconds,
            LogLevelName(Level), (unsigned long) GetCurrentProcessId(),
            (unsigned long) GetCurrentThreadId(), Module, Message);
        OutputDebugStringA(Line);
    }
}

static void LogEmitV(int Level, void *Caller, const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    LogEmit(Level, Caller, Format, Args);
    va_end(Args);
}

// ---------------------------------------------------------------------------
// Public level-based API (declared in util.h). The wrappers capture the
// caller address here, so the module name in the line is the DLL that called
// the logger, not supp.nt.dll.

UTIL_EXPORT bool LogIsEnabled(int Level)
{
    if (Level < LOG_LEVEL_ERROR || Level > LOG_LEVEL_TRACE)
        return false;
    return LogGetLevelCached() >= Level;
}

UTIL_EXPORT void LogPrint(const char *Format, ...)
{
    if (!LogIsEnabled(LOG_LEVEL_DEBUG))
        return;
    va_list Args;
    va_start(Args, Format);
    LogEmit(LOG_LEVEL_DEBUG, LOG_RETURN_ADDRESS(), Format, Args);
    va_end(Args);
}

UTIL_EXPORT void LogInfo(const char *Format, ...)
{
    if (!LogIsEnabled(LOG_LEVEL_INFO))
        return;
    va_list Args;
    va_start(Args, Format);
    LogEmit(LOG_LEVEL_INFO, LOG_RETURN_ADDRESS(), Format, Args);
    va_end(Args);
}

UTIL_EXPORT void LogWarn(const char *Format, ...)
{
    if (!LogIsEnabled(LOG_LEVEL_WARN))
        return;
    va_list Args;
    va_start(Args, Format);
    LogEmit(LOG_LEVEL_WARN, LOG_RETURN_ADDRESS(), Format, Args);
    va_end(Args);
}

UTIL_EXPORT void LogErr(const char *Format, ...)
{
    if (!LogIsEnabled(LOG_LEVEL_ERROR))
        return;
    va_list Args;
    va_start(Args, Format);
    LogEmit(LOG_LEVEL_ERROR, LOG_RETURN_ADDRESS(), Format, Args);
    va_end(Args);
}

UTIL_EXPORT void LogTrace(const char *Format, ...)
{
    if (!LogIsEnabled(LOG_LEVEL_TRACE))
        return;
    va_list Args;
    va_start(Args, Format);
    LogEmit(LOG_LEVEL_TRACE, LOG_RETURN_ADDRESS(), Format, Args);
    va_end(Args);
}

// Effective log file path (also used by the Launcher to delete/open the log).
UTIL_EXPORT const char *LogGetName(void)
{
    LogEnsureInit();
    if (!LogLockReady)
        return "win86emu.log";

    EnterCriticalSection(&LogLock);
    LogBuildPathLocked();
    const char *Result = LogPath;
    LeaveCriticalSection(&LogLock);
    return Result;
}

// ---------------------------------------------------------------------------
// Compatibility API (originally declared in logging.h).

SUPP_API void InitDebugLog(void)
{
    LogEnsureInit();
    if (!LogLockReady)
    {
        OutputDebugStringA("[supp.nt.dll] logger init failed: critical section unavailable\n");
        return;
    }

    char Path[MAX_PATH];
    int Level;
    unsigned long Error;
    bool Opened;

    EnterCriticalSection(&LogLock);
    LogOpenLocked(true);        // explicit init retries an earlier failure
    _snprintf_s(Path, sizeof(Path), _TRUNCATE, "%s", LogPath);
    Level = LogGetLevelCached();
    Error = LogFileError;
    Opened = LogFile >= 0;
    LeaveCriticalSection(&LogLock);

    if (Opened)
        LogInfo("logger initialized: file='%s', level=%d", Path, Level);
    else
        LogErr("logger file unavailable: '%s' (error=%lu); using debug output only", Path, Error);
}

SUPP_API void ShutdownDebugLog(void)
{
    LogEnsureInit();
    if (!LogLockReady)
        return;

    EnterCriticalSection(&LogLock);
    LogShutdown = true;
    if (LogFile >= 0)
    {
        SYSTEMTIME St;
        GetLocalTime(&St);
        char Line[256];
        _snprintf_s(Line, sizeof(Line), _TRUNCATE,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d [INFO][pid=%lu tid=%lu][supp.nt.dll] logger shutdown\n",
            (int) St.wYear, (int) St.wMonth, (int) St.wDay,
            (int) St.wHour, (int) St.wMinute, (int) St.wSecond, (int) St.wMilliseconds,
            (unsigned long) GetCurrentProcessId(), (unsigned long) GetCurrentThreadId());
        LogWriteRawLocked(Line, strlen(Line));
        _commit(LogFile);
        _close(LogFile);
        LogFile = -1;
    }
    LeaveCriticalSection(&LogLock);
}

SUPP_API void WriteDebugLog(const char *Format, ...)
{
    if (!LogIsEnabled(LOG_LEVEL_DEBUG))
        return;
    va_list Args;
    va_start(Args, Format);
    LogEmit(LOG_LEVEL_DEBUG, LOG_RETURN_ADDRESS(), Format, Args);
    va_end(Args);
}

SUPP_API void LogDllLoadFailure(const char *DllName, DWORD ErrorCode)
{
    if (!LogIsEnabled(LOG_LEVEL_ERROR))
        return;
    LogEmitV(LOG_LEVEL_ERROR, LOG_RETURN_ADDRESS(),
             "failed to load DLL '%s' (GetLastError=%lu)",
             DllName ? DllName : "?", (unsigned long) ErrorCode);
}

SUPP_API void LogEmulationStart(DWORD EntryPoint, int NParams)
{
    if (!LogIsEnabled(LOG_LEVEL_DEBUG))
        return;
    LogEmuStartTick = LogTickNow();
    LogEmitV(LOG_LEVEL_DEBUG, LOG_RETURN_ADDRESS(),
             "emulation start: addr=0x%08X NParams=%d tid=%lu",
             (unsigned int) EntryPoint, NParams, (unsigned long) GetCurrentThreadId());
}

SUPP_API void LogEmulationEnd(DWORD ReturnValue)
{
    if (!LogIsEnabled(LOG_LEVEL_DEBUG))
        return;
    unsigned long long Start = LogEmuStartTick;
    LogEmuStartTick = 0;
    if (Start != 0)
        LogEmitV(LOG_LEVEL_DEBUG, LOG_RETURN_ADDRESS(),
                 "emulation end: ret=0x%08X duration=%lums",
                 (unsigned int) ReturnValue, (unsigned long) (LogTickNow() - Start));
    else
        LogEmitV(LOG_LEVEL_DEBUG, LOG_RETURN_ADDRESS(),
                 "emulation end: ret=0x%08X", (unsigned int) ReturnValue);
}

SUPP_API unsigned long LogGetFileError(void)
{
    LogEnsureInit();
    unsigned long Error = 0;
    if (LogLockReady)
    {
        EnterCriticalSection(&LogLock);
        Error = LogFileError;
        LeaveCriticalSection(&LogLock);
    }
    return Error;
}

SUPP_API bool LogGetFileOpened(void)
{
    LogEnsureInit();
    bool Opened = false;
    if (LogLockReady)
    {
        EnterCriticalSection(&LogLock);
        Opened = LogFile >= 0;
        LeaveCriticalSection(&LogLock);
    }
    return Opened;
}

// ---------------------------------------------------------------------------
// Unhandled exception filter. Records what is known and lets the default
// handling continue; a filter cannot report crashes that never reach it
// (see LOGGING.md for the limits).

static LONG WINAPI LogUnhandledExceptionFilter(EXCEPTION_POINTERS *Info)
{
    if (Info != 0 && Info->ExceptionRecord != 0)
    {
        EXCEPTION_RECORD *Record = Info->ExceptionRecord;
        SYSTEMTIME St;
        GetLocalTime(&St);

        char Where[2 * MAX_PATH];
        LogFormatAddress(Record->ExceptionAddress, Where, sizeof(Where));

        char Line[LOG_LINE_MAX];
        int Used = _snprintf_s(Line, sizeof(Line), _TRUNCATE,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d [ERROR][pid=%lu tid=%lu][supp.nt.dll] "
            "unhandled exception 0x%08X at 0x%p (%s)",
            (int) St.wYear, (int) St.wMonth, (int) St.wDay,
            (int) St.wHour, (int) St.wMinute, (int) St.wSecond, (int) St.wMilliseconds,
            (unsigned long) GetCurrentProcessId(), (unsigned long) GetCurrentThreadId(),
            (unsigned int) Record->ExceptionCode, Record->ExceptionAddress, Where);
        if (Used > 0 && Record->ExceptionCode == 0xC0000005 &&
            Record->NumberParameters >= 2)
        {
            _snprintf_s(Line + Used, sizeof(Line) - (size_t) Used, _TRUNCATE,
                " access=%s address=0x%p",
                Record->ExceptionInformation[0] ? "write" : "read",
                (void *) Record->ExceptionInformation[1]);
        }

        size_t Len = strlen(Line);
        if (Len + 1 < sizeof(Line))
        {
            Line[Len] = '\n';
            Line[Len + 1] = 0;
            Len++;
        }

        LogEnsureInit();
        bool Written = false;
        if (LogLockReady && TryEnterCriticalSection(&LogLock))
        {
            Written = LogWriteLineLocked(LOG_LEVEL_ERROR, Line, Len);
            LeaveCriticalSection(&LogLock);
        }
        OutputDebugStringA(Line);
        (void) Written;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

SUPP_API void LogInstallUnhandledExceptionFilter(void)
{
    SetUnhandledExceptionFilter(LogUnhandledExceptionFilter);
}
