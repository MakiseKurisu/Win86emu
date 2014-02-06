// cpihook.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <malloc.h>
#include <stdio.h>

void DbgPrint(
    _In_  PCHAR Format,
    ...
    )
{
    char Buff[512];
    va_list va;
    va_start(va, Format);
    vsprintf_s(Buff, Format, va);

    OutputDebugStringA(Buff);
}

int PeLdrIsValidX86(LPCWSTR FileName)
{
    char Buff1[512];
    wchar_t Buff[1024];
    if (SearchPathW(0, FileName, L".DLL", 1024, Buff, 0) == 0)
        wcscpy_s(Buff, FileName);

    HANDLE H = CreateFileW(Buff, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (H == INVALID_HANDLE_VALUE)
    {
        DbgPrint("IsValidX86: can't open file: %S\n", Buff);
        SetLastError(ERROR_NOT_FOUND);
        return 0;
    }

    // Validate PE
    DWORD Len = 0;
    if (ReadFile(H, Buff1, 512, &Len, 0) == FALSE || Len != 512 || Buff1[0] != 'M' || Buff1[1] != 'Z')
    {
        DbgPrint("IsValidX86: invalid file format (no MZ): %S\n", Buff);
        CloseHandle(H);
        SetLastError(ERROR_INVALID_EXE_SIGNATURE);
        return 0;
    }
    if (SetFilePointer(H, ((PIMAGE_DOS_HEADER) Buff1)->e_lfanew, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        DbgPrint("IsValidX86: invalid file format (bad e_lfanew): %S (%d)\n", Buff, GetLastError());
        CloseHandle(H);
        SetLastError(ERROR_INVALID_EXE_SIGNATURE);
        return 0;
    }
    if (ReadFile(H, Buff1, 512, &Len, 0) == FALSE || Len != 512 || Buff1[0] != 'P' || Buff1[1] != 'E' || Buff1[2] != 0 || Buff1[3] != 0)
    {
        DbgPrint("IsValidX86: invalid file format (no PE): %S\n", Buff);
        CloseHandle(H);
        SetLastError(ERROR_INVALID_EXE_SIGNATURE);
        return 0;
    }
    if (Buff1[4] != 0x4c || Buff1[5] != 1)
    {
        DbgPrint("IsValidX86: invalid machine (not 0x14c): %S\n", Buff);
        CloseHandle(H);
        SetLastError(ERROR_INVALID_EXE_SIGNATURE);
        return 0;
    }

    CloseHandle(H);

    bool IsGUI = ((PIMAGE_NT_HEADERS) Buff1)->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI;

    return IsGUI ? 1 : -1;
}

extern HMODULE CpiHookModule;

DWORD CPIHookThread(void* Arr[2])
{
    __try
    {
        DWORD *Arg = (DWORD*) Arr[0];
        wchar_t *Path = (wchar_t*) Arr[1];
        LPWSTR App = (LPWSTR) Arg[1];
        LPWSTR Cmdline = (LPWSTR) Arg[2];

        if (App == 0 && Cmdline == 0)
            return 0;

        if (App == 0)		// extract app name from cmd line
        {
            wcscpy_s(Path, 32768, Cmdline);
            wchar_t *TmpCmd = Path;
            wchar_t *P = 0;
            if (Path[0] == '\"')
            {
                TmpCmd++;
                P = wcschr(TmpCmd, '\"');
            }
            else
                P = wcschr(TmpCmd, ' ');
            if (P)
                *P = 0;
            App = TmpCmd;
        }

        int Type = PeLdrIsValidX86(App);
        if (Type != 0)
        {
            SetEnvironmentVariable(L"EMU_PROGRAM", App);
            Arg[1] = (DWORD) Path;
        }

        wcscpy_s(Path, 32768, L"c:\\x86");
        DWORD Ret = GetModuleFileNameW(CpiHookModule, Path, MAX_PATH);
        wchar_t *Pp = wcsrchr(Path, '\\');
        if (Pp)
            *Pp = 0;
        if (Type > 0)
            wcscat_s(Path, 32768, L"\\thunk86.exe");
        else
            wcscat_s(Path, 32768, L"\\thunk86c.exe");

        HWND Hw = FindWindowW(L"X86AutoHook", 0);
        if (Hw)
            PostMessageW(Hw, WM_COMMAND, 0, 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return 0;
}

static __declspec(thread) wchar_t Path[32768];	// Thread-safe buffer
void MyCPIHook(DWORD* Arg)
{
    void *Arr[2];
    Arr[0] = Arg;
    Arr[1] = Path;
    HANDLE H = CreateThread(0, 256 * 1024, (LPTHREAD_START_ROUTINE) CPIHookThread, (void*) Arr, STACK_SIZE_PARAM_IS_A_RESERVATION, 0);
    if (H)
    {
        WaitForSingleObject(H, INFINITE);
        CloseHandle(H);
    }
}

bool HookInstalled = false;

extern "C" __declspec(dllexport) bool CpiMakeHook()
{
    if (HookInstalled)
        return true;

    HookInstalled = true;

    HMODULE HM = LoadLibraryA("kernelbase.dll");
    if (HM == 0)
        return false;

    BYTE* cpi = (BYTE*) GetProcAddress(HM, "CreateProcessInternalW");
    if (cpi == 0)
    {
        HM = LoadLibraryA("kernel32.dll");
        if (HM == 0)
            return false;

        cpi = (BYTE*) GetProcAddress(HM, "CreateProcessInternalW");
        if (cpi == 0)
            return false;
    }
#ifdef _ARM_
    /*
    6DE28040 E92D 000F            push.w      {r0-r3}  
    6DE28044 E92D 4FF0            push        {r4-r11,lr}  
    6DE28048 F8DF 101C            ldr.w       r1,CBCode+28h (6DE28068h)  
    6DE2804C A800                 add         r0,sp,#0  
    6DE2804E 4788                 blx         r1  
    6DE28050 F8DF B018            ldr         r11,CBCode+2Ch (6DE2806Ch)  
    6DE28054 F8DD 0020            ldr.w       r0,[sp,#0x20]  
    6DE28058 F8DD 1024            ldr.w       r1,[sp,#0x24]  
    6DE2805C F8DD 2028            ldr.w       r2,[sp,#0x28]  
    6DE28060 F8DD 302C            ldr.w       r3,[sp,#0x2C]  
    6DE28064 4758                 bx          r11  
    6DE28066 0000                 movs        r0,r0  
    6DE28068 6655                 ...
    6DE2806A 8877                 ...
    6DE2806C 2211                 ...
    6DE2806E 4433                 ...
    */
    cpi--;	// THUMB
    static BYTE CBCode[]={0x2D, 0xE9, 0xF, 0, 0x2D, 0xE9, 0xF0, 0x4F, 9, 0xA8,
        0xDF, 0xF8, 0x1C, 0x10, 0x88, 0x47, 0xDD, 0xF8, 0x24,
        0, 0xDD, 0xF8, 0x28, 0x10, 0xDD, 0xF8, 0x2C, 0x20,
        0xDD, 0xF8, 0x30, 0x30, 0xD, 0xF2, 0x1C, 0xB, 0xDF,
        0xF8, 4, 0xF0, 0x88, 0x77, 0x66, 0x55, 0x11, 0x22,
        0x33, 0x44};
    BYTE JMPCode[]={0xDF, 0xF8, 0, 0xF0, 0x11, 0x22, 0x33, 0x44};	// should be 8 bytes exactly!
    if(cpi[0]!=0x0f || cpi[1]!=0xb4 || cpi[2]!=0x2d || cpi[3]!=0xe9 || cpi[4]!=0xf0 || cpi[5]!=0x4f || cpi[6]!=0x0d || cpi[7]!=0xf2 || cpi[9]!=0x0b)
        return false;

#else
    if (cpi[0] != 0x68 || cpi[5] != 0x68)
        return false;
    /*
    55                   push        ebp
    8B EC                mov         ebp,esp
    8D 45 08             lea         eax,[ebp+8]
    50                   push        eax
    B8 CC CC CC CC       mov         eax,0CCCCCCCCh
    FF D0                call        eax
    89 EC                mov         esp,ebp
    5D                   pop         ebp
    CC CC CC CC cc
    CC CC CC CC cc
    B8 CC CC CC CC       mov         eax,0CCCCCCCCh
    ff e0				 jmp		 eax
    */
    static BYTE CBCode[] = { 0x55, 0x8B, 0xEC, 0x8D, 0x45, 0x08, 0x50, 0xb8, 0xcc, 0xcc, 0xcc, 0xcc, 0xff, 0xd0, 0x89, 0xec, 0x5d, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xb8, 0xcc, 0xcc, 0xcc, 0xcc, 0xff, 0xe0 };
    BYTE JMPCode[] = { 0xb8, 0xcc, 0xcc, 0xcc, 0xcc, 0xff, 0xe0, 0xcc };	// should be 8 bytes exactly!
#endif
    DWORD Tmp;
    BYTE *thunk = (BYTE*) malloc(sizeof(CBCode));
    memcpy(thunk, CBCode, sizeof(CBCode));
#ifdef _ARM_
    thunk[34]=cpi[8];
    *(DWORD*)(thunk+sizeof(CBCode)-8)=1|(DWORD)MyCPIHook;
    *(DWORD*)(thunk+sizeof(CBCode)-4)=1|(((DWORD)cpi)+10);
    *(DWORD*)(JMPCode+4)=1|((DWORD)thunk);
#else
    memcpy(thunk + sizeof(CBCode) -17, cpi, 10);
    *(DWORD*) (thunk + 8) = (DWORD) MyCPIHook;
    *(DWORD*) (thunk + sizeof(CBCode) -6) = ((DWORD) cpi) + 10;
    *(DWORD*) (JMPCode + 1) = ((DWORD) thunk);
#endif
    VirtualProtect(cpi, sizeof(JMPCode), PAGE_EXECUTE_READWRITE, &Tmp);

    //	InterlockedExchange64((volatile LONGLONG*)cpi,*(LONGLONG*)JMPCode);
    memcpy(cpi, JMPCode, 8);

    VirtualProtect(thunk, sizeof(CBCode), PAGE_EXECUTE_READWRITE, &Tmp);
    FlushInstructionCache(GetCurrentProcess(), thunk, sizeof(CBCode));
    FlushInstructionCache(GetCurrentProcess(), cpi, sizeof(JMPCode));

    return true;
}

DWORD WINAPI ThreadProc(		// this thread would start after the process finishes its initialization
    _In_  LPVOID lpParameter
    )
{
    __try
    {
        if (CpiMakeHook())
            OutputDebugStringA("cpihook.dll: CPI hook installed\n");
        else
            OutputDebugStringA("cpihook.dll: Failed to install CPI hook\n");
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return 0;
}

bool HookThisProcess()
{
    CloseHandle(CreateThread(0, 256 * 1024, ThreadProc, 0, STACK_SIZE_PARAM_IS_A_RESERVATION, 0));
    return true;
}
