// supp.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <peldr.h>
#define UTIL_EXPORT __declspec(dllexport)
#include <util.h>
#include <regredir.h>
#include <emversion.h>

// Check if we are running under emulation
UTIL_EXPORT bool IsEmulatorPresent(void)
{
    static bool Checked = false;
    static bool Ret = false;

    if (Checked)
        return Ret;

    HMODULE HK = LoadLibraryA("kernel32.dll");	// no need to call FreeLibrary as kernel32 cannot unload
    if (HK == 0 || GetProcAddress(HK, "IsEmulatorPresent") == 0)
        Ret = false;
    else
        Ret = true;
    Checked = true;
    return Ret;
}

UTIL_EXPORT int  CfgGetInt(const char *Setting, int Default)
{
    DWORD Ret = Default;
    DWORD cbRet = sizeof(Ret);
    if (RegGetValueA(HKEY_CURRENT_USER, "Software\\x86node\\Settings", Setting, RRF_RT_DWORD, NULL, &Ret, &cbRet))
        RegGetValueA(HKEY_LOCAL_MACHINE, "Software\\x86node\\Settings", Setting, RRF_RT_DWORD, NULL, &Ret, &cbRet);
    return Ret;
}

UTIL_EXPORT bool CfgSetInt(const char *Setting, int Value)
{
    BOOL Ret = FALSE;
    DWORD cbValue = 4;
    HKEY KeyRoot;
    int retCode;
    DWORD   dwDisposition;
    retCode = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\x86node\\Settings", 0,
        NULL, 0, KEY_ALL_ACCESS, NULL, &KeyRoot, &dwDisposition);
    Ret = (ERROR_SUCCESS == RegSetValueExA(KeyRoot, Setting, 0, REG_DWORD,
        (LPBYTE) &Value, 4));
    RegCloseKey(KeyRoot);
    return Ret != 0;
}

// can be used to get not only strings
UTIL_EXPORT bool CfgGetString(const char *Setting, char *Out, int OutLen)
{
    DWORD cbRet = OutLen;
    BOOL Ret = TRUE;
    if (RegGetValueA(HKEY_CURRENT_USER, "Software\\x86node\\Settings", Setting, RRF_RT_ANY, NULL, Out, &cbRet))
        Ret = ERROR_SUCCESS == RegGetValueA(HKEY_LOCAL_MACHINE, "Software\\x86node\\Settings", Setting, RRF_RT_ANY, NULL, Out, &cbRet);
    return Ret != 0;
}

UTIL_EXPORT bool CfgSetString(const char *Setting, const char *Value)
{
    BOOL Ret = FALSE;
    DWORD cbValue = 4;
    HKEY KeyRoot;
    int retCode;
    DWORD   dwDisposition;
    retCode = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\x86node\\Settings", 0,
        NULL, 0, KEY_ALL_ACCESS, NULL, &KeyRoot, &dwDisposition);
    Ret = (ERROR_SUCCESS == RegSetValueExA(KeyRoot, Setting, 0, REG_SZ,
        (LPBYTE) Value, strlen(Value)));
    RegCloseKey(KeyRoot);
    return Ret != 0;
}

static char LogPath[MAX_PATH];
static int F = 0;
static void LogPuts(const char *str)
{
    if (LogPath[0] == 0)
    {
        char Name[256];
        GetEnvironmentVariableA("TEMP", LogPath, MAX_PATH);
        sprintf_s<256>(Name, "\\win86emu.log");
        strcat_s<MAX_PATH>(LogPath, Name);
        CfgGetString("LogFile", LogPath, MAX_PATH);
    }
    if (LogPath[0] == 0)
        strcpy_s(LogPath, "win86emu.log");
    if (str == 0)
        return;
    if (F == 0)
        if (_sopen_s(&F, LogPath, _O_RDWR | _O_APPEND, SH_DENYNO, _S_IREAD | _S_IWRITE))
            _sopen_s(&F, LogPath, _O_RDWR | _O_CREAT, SH_DENYNO, _S_IREAD | _S_IWRITE);

    if (F != 0)
    {
        _write(F, str, strlen(str));
        _commit(F);
    }
#ifndef _DEBUG
    else
#endif
        OutputDebugStringA(str);
}

UTIL_EXPORT const char *LogGetName()
{
    if (LogPath[0] == 0)
        LogPuts(0);
    return LogPath;
}

static int LogGetLevel()	// 4=all,3=info+warn+err,2=warn+err,1=err,0=off
{
    static int Level = 0;
    static BOOL Checked = FALSE;
    if (!Checked)
    {
        Level = CfgGetInt("LogLevel", 2);
        Checked = TRUE;
    }
    return Level;
}

static void LogvPrint(const char *mark, const char *fmt, va_list va)
{
    static __declspec(thread) char Buff[2048];	// save the stack but still be thread-safe
    char I = '[';
    char O = ']';

    if (IsEmulatorPresent())
    {
        I = '<';
        O = '>';
    }
    if (mark)
        sprintf_s(Buff, "%c%6d%c%s: ", I, GetCurrentProcessId(), O, mark);
    else
        sprintf_s(Buff, "%c%6d%c   ", I, GetCurrentProcessId(), O);
    vsprintf(Buff + strlen(Buff), fmt, va);	// possible buffer overflow
    if (Buff[strlen(Buff) - 1] != '\n')
        strcat_s(Buff, "\n");
    LogPuts(Buff);
}


UTIL_EXPORT void LogPrint(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    if (LogGetLevel() < 4)
        return;

    LogvPrint(NULL, fmt, va);
}

UTIL_EXPORT void LogInfo(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    if (LogGetLevel() < 3)
        return;

    LogvPrint("I", fmt, va);
}

UTIL_EXPORT void LogWarn(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    if (LogGetLevel() < 2)
        return;

    LogvPrint("W", fmt, va);
}

UTIL_EXPORT void LogErr(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    if (LogGetLevel() < 1)
        return;

    LogvPrint("E", fmt, va);
}

// Get int value from compatibility section (EXE file), return 0 if not present
UTIL_EXPORT int CompatGetInt(const char *Setting)
{
    static bool GotExe = false;
    static char FileName[256] = "invalid.exe";
    static char *Exe = FileName;
    if (!GotExe)	// Thread unsafe!
    {
        if (IsEmulatorPresent())
            GetModuleFileNameA(GetModuleHandleA(0), FileName, 256);
        else
        {
            static HMODULE peldr = LoadLibraryA("peldr.nt.dll");
            if (peldr)
            {
                typedef DWORD t_PeLdrGetModuleFileNameA(HANDLE Pe, LPSTR FileName, DWORD Size);
                typedef HANDLE t_PeLdrFindModuleA(LPCSTR FileName);
                t_PeLdrGetModuleFileNameA *PeLdrGetModuleFileNameA = (t_PeLdrGetModuleFileNameA*) GetProcAddress(peldr, "PeLdrGetModuleFileNameA");
                t_PeLdrFindModuleA *PeLdrFindModuleA = (t_PeLdrFindModuleA*) GetProcAddress(peldr, "PeLdrFindModuleA");
                if (PeLdrGetModuleFileNameA && PeLdrFindModuleA)
                    PeLdrGetModuleFileNameA(PeLdrFindModuleA(0), FileName, 256);
            }
        }

        Exe = strrchr(FileName, '\\');
        if (Exe == 0)
            Exe = FileName;
        else
            Exe++;
        GotExe = true;
        LogInfo("Reading compatibility settings for %s", Exe);
    }

    DWORD Ret = 0;
    DWORD cbRet = sizeof(Ret);
    char Buff[128] = "Software\\x86node\\Compatibility\\";
    strcat_s(Buff, Exe);
    if (RegGetValueA(HKEY_CURRENT_USER, Buff, Setting, RRF_RT_DWORD, NULL, &Ret, &cbRet))		// local per prog
        if (RegGetValueA(HKEY_LOCAL_MACHINE, Buff, Setting, RRF_RT_DWORD, NULL, &Ret, &cbRet))	// global per prog
            if (RegGetValueA(HKEY_CURRENT_USER, "Software\\x86node\\Compatibility", Setting, RRF_RT_DWORD, NULL, &Ret, &cbRet))	// local for all progs
                RegGetValueA(HKEY_LOCAL_MACHINE, "Software\\x86node\\Compatibility", Setting, RRF_RT_DWORD, NULL, &Ret, &cbRet);	// global for all progs
    return Ret;
}

typedef DWORD t_Redirect(DWORD Reg);
UTIL_EXPORT HKEY RegRedirect(HKEY Reg)
{
    static t_Redirect *Redirect = 0;
    if (Redirect == 0)
    {
        char Tmp[1024];
        DWORD Ret = GetModuleFileNameA(0, Tmp, 1024);
        char *P = strrchr(Tmp, '\\');
        if (P)
            *P = 0;
        strcat_s(Tmp, "\\SystemNT\\advapi32.nt.dll");
        HMODULE HM = LoadLibraryA(Tmp);
        if (HM)
            //#ifdef _ARM_
            Redirect = (t_Redirect*) GetProcAddress(HM, "Redirect");
        //#else
        //			Redirect=(t_Redirect*)GetProcAddress(HM,"@Redirect@4");
        //#endif
        else
            LogErr("Can't load %s, error %d", Tmp, GetLastError());
    }
    if (Redirect)
        return (HKEY) Redirect((DWORD) Reg);
    return (HKEY) Reg;
}

// Get emulator version, 0xAABBCCCC - version is AA.BB build CCCC. AA,BB,CCCC=decimal
UTIL_EXPORT unsigned int EmGetVersion(void)
{
    return (EM_MAJOR << 24) | (EM_MINOR << 16) | (EM_BUILD & 0xFFFF);
}

// Get emulator version as string
UTIL_EXPORT const char *EmGetVersionString(void)
{
    static char Version[128];
    if (Version[0])
        return Version;

    sprintf_s(Version, "%d.%02d %s, build %d at %s", EM_MAJOR, EM_MINOR, EM_STRING, EM_BUILD & 0xFFFF, __DATE__);

    return Version;
}
