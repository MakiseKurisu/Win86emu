// logging.h : Common debug logging for Win86emun
//
// The logger is implemented once, inside supp.nt.dll, and is shared by every
// Win86emun component (x86_emu.dll, dosbox_emu.dll, bochs_emu.dll,
// cbacks.nt.dll, peldr.nt.dll, Launcher). All of them already import
// supp.nt.dll, so no new DLL and no new load order is introduced.
//
// The level-based helpers (LogErr/LogWarn/LogInfo/LogPrint/LogTrace) are
// declared in <util.h>. See LOGGING.md for the line format, default file
// location, levels, rotation and how to change them.
//

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Export macro for supp.nt.dll
#ifdef SUPP_EXPORTS
#define SUPP_API __declspec(dllexport)
#else
#define SUPP_API __declspec(dllimport)
#endif

// Initialize the logging subsystem: open (or retry opening) the log file and
// log the result, including the file path and the level. Safe to call more
// than once. Called from x86_emu.dll EmuInitialize.
SUPP_API void InitDebugLog(void);

// Flush and close the log file. Log calls after this still work and fall back
// to debug output only. Called on DLL_PROCESS_DETACH.
SUPP_API void ShutdownDebugLog(void);

// Compatibility wrapper: writes one DEBUG-level line.
SUPP_API void WriteDebugLog(const char* format, ...);

// Log a failed DLL load with its error code (ERROR level).
SUPP_API void LogDllLoadFailure(const char* dllName, DWORD errorCode);

// Log one emulated call entry/exit (DEBUG level). End adds the duration
// measured since the matching Start on the same thread.
SUPP_API void LogEmulationStart(DWORD entryPoint, int nParams);
SUPP_API void LogEmulationEnd(DWORD returnValue);

// Install a process-wide unhandled exception filter that records the
// exception code and address (with module+offset) to the log. The exception
// is never suppressed, so the default handling is unchanged.
SUPP_API void LogInstallUnhandledExceptionFilter(void);

// Diagnostics for the log file itself. LogGetFileError returns the last open
// or write error (0 = none), LogGetFileOpened tells whether the file is
// currently open. Both are meant for startup diagnostics and tests.
SUPP_API unsigned long LogGetFileError(void);
SUPP_API bool LogGetFileOpened(void);

#ifdef __cplusplus
}
#endif
