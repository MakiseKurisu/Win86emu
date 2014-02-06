/*
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * Code translator internal header
 *
 */

#ifndef __YACTEMU_H
#define __YACTEMU_H

#include <x86emul.h>

#define FASTCALL __fastcall

#ifdef __cplusplus
extern "C" {
#endif

    enum rep_op
    {
        none = 0, rep = 1, repe = 1, repne = 2
    };

    struct x86opcode
    {
        BYTE Current;	// Current opcode byte (byte1 or byte2 of instruction code)
        BYTE *CurrentPtr;	// Current opcode byte
        const char *Mnemonics;	// "mov", "add", etc
        bool Finished;	// true == can't continue execution (fault/return to host/error)

        // prefixes:
        bool SegFS;		// We are only interested in FS: override, other segs are ignored
        rep_op Rep;		// none, rep/repe, repne
        bool Lock;		// Lock prefix
        bool Opsize;	// Operand size override
        bool Addrsize;	// Address size override

        // mod r/m byte decrypted, only for those opcodes where it is present
        int Mod;
        int Reg;
        int Mem;
        // sib byte (where present)
        int Scale;
        int Index;
        int Base;
    };


    // fetch next byte
    BYTE inline FASTCALL FetchB(x86opcode *Op)
    {
        return *(Op->CurrentPtr++);
    }

    // fetch next byte as mod r/m and fill structure fields
    void inline FASTCALL FetchModRM(x86opcode *Op)
    {
        BYTE ModRM = FetchB(Op);
        Op->Mod = ModRM >> 6;
        Op->Reg = (ModRM & 7) >> 3;
        Op->Mem = (ModRM & 7);
    }

    // fetch next byte as SIB and fill structure fields
    void inline FASTCALL FetchSIB(x86opcode *Op)
    {
        BYTE SIB = FetchB(Op);
        Op->Scale = SIB >> 6;
        Op->Index = (SIB & 7) >> 3;
        Op->Base = (SIB & 7);
    }

    // prepare to processing next command (clen all structure fields)
    void inline FASTCALL InitCommand(x86opcode *Op)
    {
        Op->Mnemonics = "invalid";
        Op->Finished = false;
        Op->SegFS = false;
        Op->Rep = none;
        Op->Lock = false;
        Op->Opsize = false;
        Op->Addrsize = false;
#ifdef _DEBUG
        Op->Mod=-1;
        Op->Reg=-1;
        Op->Mem=-1;
        Op->Scale=-1;
        Op->Index=-1;
        Op->Base=-1;
#endif
    }

    bool FASTCALL ProcessCommand(x86opcode *Op);	// process command, return true==continue

    typedef bool FASTCALL COMMAND(x86opcode *Op);

    extern COMMAND *OneByteCmds[256];
    extern COMMAND *TwoByteCmds[256];

#ifdef __cplusplus
}
#endif

#endif // __YACTEMU_H
