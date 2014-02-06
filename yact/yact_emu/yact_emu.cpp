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
    x86opcode *Op = new x86opcode;
    Op->CurrentPtr = (BYTE*) Addr;

    do
    InitCommand(Op);
    while (ProcessCommand(Op));

    delete Op;
    return 0;
}


