// yact_emu.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#define EMU_EXPORT __declspec(dllexport)
#include <x86emul.h>
#include "yact_emu.h"

// Initialize engine. Return true == success.
EMU_EXPORT BOOL EmuInitialize(void)
{
    return TRUE;
}

// Emulate code starting from Addr, passing NParams on stack (like stdcall/cdecl)
EMU_EXPORT DWORD EmuExecute(DWORD Addr, int NParams, ...)
{
    /* Every handler dereferences Op->Cpu, so the opcode structure has to be
     * given a CPU state before the first instruction is dispatched.
     *
     * NOTE: this entry point is still incomplete - it does not build a stack
     * frame for the NParams arguments, has no return-to-host detection and
     * does not report the emulated EAX.  It is also not reachable from
     * x86_emu.dll, which loads dosbox_emu.dll / bochs_emu.dll instead. */
    CPU_STATE Cpu = { 0 };
    x86opcode *Op = new x86opcode;

    Cpu.eflags = 0x00000002;            /* EFLAGS bit 1 always reads as 1 */
    Op->Cpu = &Cpu;
    Op->CurrentPtr = (BYTE*) Addr;

    do
    InitCommand(Op);
    while (ProcessCommand(Op));

    delete Op;
    return 0;
}


