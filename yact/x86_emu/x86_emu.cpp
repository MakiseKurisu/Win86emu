// x86_emu.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#define EMU_EXPORT __declspec(dllexport)
#include <peldr.h>
#include <x86emul.h>
#include <util.h>
#include <stdio.h>
#include <classes.h>
//#include <callbacks.h>

typedef void* t_CbCallAtThreadExit(void* Func, void*Param);
typedef bool t_CbRemoveCallAtThreadExit(void* Func, void*Param);
t_CbCallAtThreadExit *CbCallAtThreadExit = 0;
t_CbRemoveCallAtThreadExit *CbRemoveCallAtThreadExit = 0;


typedef BOOL t_func0(void);
typedef DWORD t_func1(DWORD Addr, int NParams, ...);

#ifdef _DEBUG
#define NUM_DOSBOXES 1
#else
#define NUM_DOSBOXES 16
#endif
static t_func0 *EmuDB_Initialize[NUM_DOSBOXES];
static t_func1 *EmuDB_Execute[NUM_DOSBOXES];

t_func0 *EmuBS_Initialize = 0;
t_func1 *EmuBS_Execute = 0;

void LoadEmulatorDll()
{
    static bool Loaded = false;
    if (Loaded)
        return;

    char DllDB[] = "dosbox_emu.dll";
    char DllBS[] = "bochs_emu.dll";
    HMODULE HM = LoadLibraryA("cbacks.nt.dll");
    if (HM == 0)
    {
        LogErr("Unable to load cbacks.nt.dll, error %d", GetLastError());
        ExitProcess(-10);
    }

    CbRemoveCallAtThreadExit = (t_CbRemoveCallAtThreadExit*) GetProcAddress(HM, "CbRemoveCallAtThreadExit");
    CbCallAtThreadExit = (t_CbCallAtThreadExit*) GetProcAddress(HM, "CbCallAtThreadExit");
    if (CbRemoveCallAtThreadExit == 0 || CbCallAtThreadExit == 0)
    {
        LogErr("Bad cbacks.nt.dll");
        ExitProcess(-10);
    }

    for (int i = 0; i < NUM_DOSBOXES; i++)
    {
        char Buff[32];
        sprintf_s(Buff, "dosbox_emu%d.dll", i);
        if (i == 0)
            strcpy_s(Buff, DllDB);
        //		else
        //			CopyFileA(DllDB,Buff,FALSE);

        HM = LoadLibraryA(Buff);

        if (HM == 0)
        {
            LogErr("Unable to load x86 emulator engine %s, error %d", Buff, GetLastError());
            ExitProcess(-1);
        }

        EmuDB_Initialize[i] = (t_func0*) GetProcAddress(HM, "EmuInitialize");
        if (EmuDB_Initialize[i] == 0)
        {
            LogErr("Invalid x86 emulator engine %s, error %d", Buff, GetLastError());
            ExitProcess(-1);
        }
        EmuDB_Execute[i] = (t_func1*) GetProcAddress(HM, "EmuExecute");
    }

    HM = LoadLibraryA(DllBS);

    if (HM == 0)
    {
        LogErr("Unable to load x86 emulator engine %s, error %d", DllBS, GetLastError());
        ExitProcess(-1);
    }

    EmuBS_Initialize = (t_func0*) GetProcAddress(HM, "EmuInitialize");
    EmuBS_Execute = (t_func1*) GetProcAddress(HM, "EmuExecute");

    if (EmuBS_Initialize == 0 || EmuDB_Initialize[0] == 0 || EmuBS_Execute == 0 || EmuDB_Execute[0] == 0)
    {
        LogErr("Invalid x86 emulator engines");
        ExitProcess(-1);
    }

    Loaded = true;
    LogInfo("Loaded x86 emulator engines");
}

EMU_EXPORT BOOL EmuInitialize(void)
{
    LoadEmulatorDll();
    return EmuDB_Initialize[0]() && EmuBS_Initialize();
}

int __declspec(thread) FPU_cw = 0x37F;
EMU_EXPORT int GetFPUCW()
{
    return FPU_cw;
}

EMU_EXPORT void SetFPUCW(int cw)
{
    FPU_cw = cw;
}

static DWORD DBInUse[NUM_DOSBOXES];
LPCRITICAL_SECTION DBCs = 0;
CLock DosboxLock(&DBCs);

void FreeDB(void *Ptr)
{
    DosboxLock.Lock();
    DBInUse[(int) Ptr] = 0;
    DosboxLock.Unlock();
}

EMU_EXPORT DWORD EmuExecute(DWORD Addr, int NParams, ...)
{
#define MAXARGS 8
    DWORD Tmp[MAXARGS];

    va_list argptr;
    va_start(argptr, NParams);
    for (int i = 0; i < NParams; i++)
        Tmp[i] = va_arg(argptr, DWORD);
    va_end(argptr);

    t_func1 *Emu_Execute = 0;
    DosboxLock.Lock();
    int UsedDosBox = -1;
    Emu_Execute = EmuBS_Execute;
#ifndef _DEBUG
    for (int i = 0; i < NUM_DOSBOXES; i++)
        if (DBInUse[i] == 0)
        {
            DBInUse[i] = GetCurrentThreadId();
            UsedDosBox = i;
            Emu_Execute = EmuDB_Execute[i];
            CbCallAtThreadExit(FreeDB, (void*) i);
            EmuDB_Initialize[i]();
            break;
        }
#endif
    DosboxLock.Unlock();

    if (UsedDosBox < 0)
    {
        static bool Logged = false;
        if (!Logged)
        {
            Logged = true;
            LogWarn("Failing back to BOCHS CPU core, more than %d DosBox cores used", NUM_DOSBOXES);
        }
    }

    DWORD Ret = 0;

    switch (NParams)
    {
        case 0:
            Ret = Emu_Execute(Addr, 0);
            break;
        case 1:
            Ret = Emu_Execute(Addr, 1, Tmp[0]);
            break;
        case 2:
            Ret = Emu_Execute(Addr, 2, Tmp[0], Tmp[1]);
            break;
        case 3:
            Ret = Emu_Execute(Addr, 3, Tmp[0], Tmp[1], Tmp[2]);
            break;
        case 4:
            Ret = Emu_Execute(Addr, 4, Tmp[0], Tmp[1], Tmp[2], Tmp[3]);
            break;
        case 5:
            Ret = Emu_Execute(Addr, 5, Tmp[0], Tmp[1], Tmp[2], Tmp[3], Tmp[4]);
            break;
        case 6:
            Ret = Emu_Execute(Addr, 6, Tmp[0], Tmp[1], Tmp[2], Tmp[3], Tmp[4], Tmp[5]);
            break;
        case 7:
            Ret = Emu_Execute(Addr, 7, Tmp[0], Tmp[1], Tmp[2], Tmp[3], Tmp[4], Tmp[5], Tmp[6]);
            break;
        default:
            LogErr("EmuExecute: too many params %d", NParams);
            printf("EmuExecute: too many params %d\n", NParams);
            ExitProcess(0);
    }

    if (UsedDosBox >= 0)
    {
        CbRemoveCallAtThreadExit(FreeDB, (void*) UsedDosBox);
        DosboxLock.Lock();
        DBInUse[UsedDosBox] = 0;
        DosboxLock.Unlock();
    }
    return Ret;
}
