#include "stdafx.h"
#include <stdio.h>
#include "yact_emu.h"

bool FASTCALL ProcessCommand(x86opcode *Op)
{
    bool Ret;

    do
    {
#ifdef _DEBUG
        DWORD Addr=PtrToDword(Op->CurrentPtr);
#endif
        /* EIP bookkeeping: EIP is the address of the instruction being
         * executed, and becomes the address of the next instruction (or of
         * the branch target, for control transfers) once the handler has
         * finished consuming bytes from the instruction stream.  This makes
         * the EIP independent of the per-handler byte counting. */
        Op->StartPtr = Op->CurrentPtr;
        Op->Cpu->eip = PtrToDword(Op->CurrentPtr);
        Op->Current = FetchB(Op);
        Ret = (OneByteCmds[Op->Current])(Op);
        Op->Cpu->eip = PtrToDword(Op->CurrentPtr);
#ifdef _DEBUG
        printf("%08X: ",Addr);
        for(DWORD i=Addr; i<PtrToDword(Op->CurrentPtr); i++)
            printf("%02X ",*(BYTE*)i);
        printf("\t%s",Op->Mnemonics);
#endif
    } while (Ret);

    return !Op->Finished;
}
