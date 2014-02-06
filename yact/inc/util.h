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

    // Print string to log
    UTIL_EXPORT void LogPrint(const char *Fmt, ...);	// LogLevel>3
    UTIL_EXPORT void LogInfo(const char *Fmt, ...);  // LogLevel>2
    UTIL_EXPORT void LogWarn(const char *Fmt, ...);  // LogLevel>1
    UTIL_EXPORT void LogErr(const char *Fmt, ...);   // LogLevel>0
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
