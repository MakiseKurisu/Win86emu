/*
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * Supplementary functions (settings/log/etc)
 *
 */

#ifndef __UTIL_H
#define __UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UTIL_EXPORT
#define UTIL_EXPORT
#endif

    // Check if we are running under emulation
    UTIL_EXPORT bool IsEmulatorPresent(void);

    // Print string to log. Levels match the registry LogLevel setting
    // (HKCU/HKLM\Software\x86node\Settings, default 2): 1=ERROR 2=WARN
    // 3=INFO 4=DEBUG 5=TRACE, 0 disables logging. Implemented in
    // supp.nt.dll; see LOGGING.md.
    UTIL_EXPORT void LogPrint(const char *Fmt, ...);	// LogLevel>3 (DEBUG)
    UTIL_EXPORT void LogInfo(const char *Fmt, ...);  // LogLevel>2 (INFO)
    UTIL_EXPORT void LogWarn(const char *Fmt, ...);  // LogLevel>1 (WARN)
    UTIL_EXPORT void LogErr(const char *Fmt, ...);   // LogLevel>0 (ERROR)
    UTIL_EXPORT void LogTrace(const char *Fmt, ...); // LogLevel>4 (TRACE, per-instruction detail)
    // TRUE if the given level (1=ERROR..5=TRACE) is currently logged; use it
    // to skip expensive message construction for TRACE.
    UTIL_EXPORT bool LogIsEnabled(int Level);
    // Effective log file path (may differ from the default; see LOGGING.md).
    UTIL_EXPORT const char *LogGetName();

    // Get int value from registry
    UTIL_EXPORT int CfgGetInt(const char *Setting, int Default = 0);
    UTIL_EXPORT bool CfgSetInt(const char *Setting, int Val);

    // Get string param from registry, if error - data at Out is not touched
    // Returns TRUE == ok
    UTIL_EXPORT bool CfgGetString(const char *Setting, char *Out, int OutLen);
    UTIL_EXPORT bool CfgSetString(const char *Setting, const char *Val);

    // Get int value from compatibility section (EXE file), return 0 if not present
    UTIL_EXPORT int CompatGetInt(const char *Setting);

    // Get emulator version, 0xAABBCCCC - version is AA.BB build CCCC. AA,BB,CCCC=decimal
    UTIL_EXPORT unsigned int EmGetVersion(void);
    // Get emulator version as string
    UTIL_EXPORT const char *EmGetVersionString(void);

#ifdef __cplusplus
}
#endif

#endif // __EMUL_H
