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
        BYTE *StartPtr;	// First byte of the instruction being executed (EIP bookkeeping)
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

        PCPU_STATE Cpu;  // CPU 状態へのポインタ
        
        // Added for operand resolution compatibility
        BYTE ModRM;      // Full ModRM byte value
        BYTE SIB;        // Full SIB byte value
        DWORD Imm32;     // 32-bit immediate value
        BYTE Imm8;       // 8-bit immediate value
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
        Op->ModRM = ModRM;  // Store full ModRM byte
        Op->Mod = (ModRM >> 6) & 3;
        Op->Reg = (ModRM >> 3) & 7;
        Op->Mem = (ModRM & 7);
    }

    // fetch next byte as SIB and fill structure fields
    void inline FASTCALL FetchSIB(x86opcode *Op)
    {
        BYTE SIB = FetchB(Op);
        Op->SIB = SIB;  // Store full SIB byte
        Op->Scale = (SIB >> 6) & 3;
        Op->Index = (SIB >> 3) & 7;
        Op->Base = (SIB & 7);
    }

    // fetch 32-bit immediate (byte-wise, see the note in operands.cpp about
    // unaligned access on the ARM32 target)
    DWORD inline FASTCALL FetchImm32(x86opcode *Op)
    {
        DWORD imm = (DWORD)Op->CurrentPtr[0]
                  | ((DWORD)Op->CurrentPtr[1] << 8)
                  | ((DWORD)Op->CurrentPtr[2] << 16)
                  | ((DWORD)Op->CurrentPtr[3] << 24);
        Op->CurrentPtr += 4;
        Op->Imm32 = imm;  // Store for later use
        return imm;
    }

    // fetch 8-bit immediate
    BYTE inline FASTCALL FetchImm8(x86opcode *Op)
    {
        BYTE imm = FetchB(Op);
        Op->Imm8 = imm;  // Store for later use
        return imm;
    }

    // fetch 16-bit immediate (byte-wise so that unaligned instruction
    // streams are safe on ARM hosts)
    WORD inline FASTCALL FetchImm16(x86opcode *Op)
    {
        WORD imm = (WORD)(Op->CurrentPtr[0] | (Op->CurrentPtr[1] << 8));
        Op->CurrentPtr += 2;
        return imm;
    }

    // fetch the immediate of a "v" (16- or 32-bit) instruction
    DWORD inline FASTCALL FetchImmSized(x86opcode *Op, int Size)
    {
        return Size == 4 ? FetchImm32(Op) : (DWORD)FetchImm16(Op);
    }

    // fetch a relative displacement of the current operand size.  A 16-bit
    // displacement is sign-extended: the branch target is the address of the
    // following instruction plus the sign-extended displacement (SDM).
    DWORD inline FASTCALL FetchRel(x86opcode *Op, int Size)
    {
        DWORD rel = FetchImmSized(Op, Size);
        return (Size == 2) ? (DWORD)(int)(short)rel : rel;
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

    /* ==================================================================
     * Instruction implementation helpers
     * ================================================================== */

    /* Operand size of the "32-bit" forms of the integer instructions.
     * The 0x66 prefix selects a 16-bit operand size. */
    static inline int OpSize32(x86opcode *Op) { return Op->Opsize ? 2 : 4; }

    /* ------------------------------------------------------------------
     * Guest address <-> host pointer conversions.
     *
     * The interpreter addresses guest memory with 32-bit linear addresses.
     * Going through size_t keeps the conversion explicit (and therefore
     * warning-free) when the interpreter core is compiled on a 64-bit host
     * for regression testing, while being a no-op on the 32-bit Windows
     * targets.
     * ------------------------------------------------------------------ */
    static inline DWORD PtrToDword(const void *p) { return (DWORD)(size_t)p; }
    static inline BYTE *DwordToPtr(DWORD v) { return (BYTE*)(size_t)v; }

    /* Mask and sign bit of a 1, 2 or 4 byte result. */
    static inline DWORD SizeMask(int Size)
    {
        return Size == 1 ? 0xFFu : (Size == 2 ? 0xFFFFu : 0xFFFFFFFFu);
    }
    static inline DWORD SizeSign(int Size)
    {
        return Size == 1 ? 0x80u : (Size == 2 ? 0x8000u : 0x80000000u);
    }

    /* Carry out of A + B + C, where A/B are already masked to Size bytes. */
    static inline BOOL CarryOut(int Size, DWORD A, DWORD B, DWORD C)
    {
        if (Size == 4)
        {
            DWORD Sum1 = A + B;
            DWORD Sum2 = Sum1 + C;
            return (Sum1 < A) || (Sum2 < Sum1) ? TRUE : FALSE;
        }
        return (A + B + C) > SizeMask(Size) ? TRUE : FALSE;
    }

    /* Borrow out of A - B - C, where A/B are already masked to Size bytes. */
    static inline BOOL BorrowOut(int Size, DWORD A, DWORD B, DWORD C)
    {
        if (Size == 4)
            return (A < B) || ((A - B) < C) ? TRUE : FALSE;
        return A < (B + C) ? TRUE : FALSE;
    }

    /* EFLAGS update helpers.
     *
     * These mirror the 32-bit-only helpers in x86emul.h, but mask every
     * value to Size bytes first so that the same code yields correct
     * CF/PF/AF/ZF/SF/OF for 8-, 16- and 32-bit operations. */
    static inline void FlagsAdd(PCPU_STATE Cpu, int Size, DWORD Res, DWORD A, DWORD B)
    {
        DWORD m = SizeMask(Size), s = SizeSign(Size);
        Res &= m; A &= m; B &= m;
        Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
        if (Res == 0) Cpu->eflags |= EFLAGS_ZF;
        if (Res & s) Cpu->eflags |= EFLAGS_SF;
        if (CalcParity(Res)) Cpu->eflags |= EFLAGS_PF;
        if (CarryOut(Size, A, B, 0)) Cpu->eflags |= EFLAGS_CF;
        if (((A & 0xF) + (B & 0xF)) > 0xF) Cpu->eflags |= EFLAGS_AF;
        if (!((A ^ B) & s) && ((A ^ Res) & s)) Cpu->eflags |= EFLAGS_OF;
    }

    static inline void FlagsAdc(PCPU_STATE Cpu, int Size, DWORD Res, DWORD A, DWORD B, DWORD CarryIn)
    {
        DWORD m = SizeMask(Size), s = SizeSign(Size);
        DWORD c = CarryIn ? 1u : 0u;
        Res &= m; A &= m; B &= m;
        Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
        if (Res == 0) Cpu->eflags |= EFLAGS_ZF;
        if (Res & s) Cpu->eflags |= EFLAGS_SF;
        if (CalcParity(Res)) Cpu->eflags |= EFLAGS_PF;
        if (CarryOut(Size, A, B, c)) Cpu->eflags |= EFLAGS_CF;
        if (((A & 0xF) + (B & 0xF) + c) > 0xF) Cpu->eflags |= EFLAGS_AF;
        if (!((A ^ B) & s) && ((A ^ Res) & s)) Cpu->eflags |= EFLAGS_OF;
    }

    static inline void FlagsSub(PCPU_STATE Cpu, int Size, DWORD Res, DWORD A, DWORD B)
    {
        DWORD m = SizeMask(Size), s = SizeSign(Size);
        Res &= m; A &= m; B &= m;
        Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
        if (Res == 0) Cpu->eflags |= EFLAGS_ZF;
        if (Res & s) Cpu->eflags |= EFLAGS_SF;
        if (CalcParity(Res)) Cpu->eflags |= EFLAGS_PF;
        if (BorrowOut(Size, A, B, 0)) Cpu->eflags |= EFLAGS_CF;
        if ((A & 0xF) < (B & 0xF)) Cpu->eflags |= EFLAGS_AF;
        if (((A ^ B) & s) && ((A ^ Res) & s)) Cpu->eflags |= EFLAGS_OF;
    }

    static inline void FlagsSbb(PCPU_STATE Cpu, int Size, DWORD Res, DWORD A, DWORD B, DWORD BorrowIn)
    {
        DWORD m = SizeMask(Size), s = SizeSign(Size);
        DWORD c = BorrowIn ? 1u : 0u;
        Res &= m; A &= m; B &= m;
        Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
        if (Res == 0) Cpu->eflags |= EFLAGS_ZF;
        if (Res & s) Cpu->eflags |= EFLAGS_SF;
        if (CalcParity(Res)) Cpu->eflags |= EFLAGS_PF;
        if (BorrowOut(Size, A, B, c)) Cpu->eflags |= EFLAGS_CF;
        if ((A & 0xF) < ((B & 0xF) + c)) Cpu->eflags |= EFLAGS_AF;
        if (((A ^ B) & s) && ((A ^ Res) & s)) Cpu->eflags |= EFLAGS_OF;
    }

    /* AND/OR/XOR/TEST: CF and OF are cleared, AF is undefined (we clear it). */
    static inline void FlagsLogical(PCPU_STATE Cpu, int Size, DWORD Res)
    {
        DWORD m = SizeMask(Size), s = SizeSign(Size);
        Res &= m;
        Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_OF | EFLAGS_AF | EFLAGS_PF | EFLAGS_ZF | EFLAGS_SF);
        if (Res == 0) Cpu->eflags |= EFLAGS_ZF;
        if (Res & s) Cpu->eflags |= EFLAGS_SF;
        if (CalcParity(Res)) Cpu->eflags |= EFLAGS_PF;
    }

    /* INC/DEC: CF is preserved, OF/SF/ZF/AF/PF are updated. */
    static inline void FlagsIncDec(PCPU_STATE Cpu, int Size, DWORD Res, BOOL Dec)
    {
        DWORD m = SizeMask(Size), s = SizeSign(Size);
        DWORD OldCF = Cpu->eflags & EFLAGS_CF;
        Res &= m;
        Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
        if (Res == 0) Cpu->eflags |= EFLAGS_ZF;
        if (Res & s) Cpu->eflags |= EFLAGS_SF;
        if (CalcParity(Res)) Cpu->eflags |= EFLAGS_PF;
        if (!Dec)
        {
            if (((Res - 1) & 0xF) == 0xF) Cpu->eflags |= EFLAGS_AF;
            if (Res == s) Cpu->eflags |= EFLAGS_OF;  /* 0x7F.. -> 0x80.. */
        }
        else
        {
            if ((Res & 0xF) == 0xF) Cpu->eflags |= EFLAGS_AF;
            if (Res == (s - 1)) Cpu->eflags |= EFLAGS_OF;  /* 0x80.. -> 0x7F.. */
        }
        Cpu->eflags |= OldCF;  /* CF is left unchanged */
    }

    /* ==================================================================
     * Decoded r/m operand
     * ================================================================== */
    struct x86operand
    {
        BOOL  IsMem;   /* TRUE  -> Addr holds the linear address           */
        int   Reg;     /* FALSE -> register index 0..7                     */
        DWORD Addr;    /* linear address when IsMem                        */
    };

    /* Resolve operand 0 (the ModR/M r/m field).  Consumes the SIB byte
     * and/or the displacement from the instruction stream, so it must be
     * called after FetchModRM() and before any immediate is fetched. */
    void GetRmOperand(x86opcode *Op, x86operand *Out);

    /* Resolve operand 1 (the ModR/M reg field).  Consumes no bytes. */
    void GetRegOperand(x86opcode *Op, x86operand *Out);

    /* Load/store an operand with an explicit width (1, 2 or 4 bytes). */
    DWORD LoadOperand(x86opcode *Op, const x86operand *O, int Size);
    void StoreOperand(x86opcode *Op, const x86operand *O, int Size, DWORD Value);

    /* Sized register access.  For Size==1 indices 4..7 select AH..BH. */
    DWORD ReadRegSized(PCPU_STATE Cpu, int Reg, int Size);
    void WriteRegSized(PCPU_STATE Cpu, int Reg, int Size, DWORD Value);

    /* Sized memory access. */
    BYTE ReadMem8(DWORD Addr);
    WORD ReadMem16(DWORD Addr);
    void WriteMem8(DWORD Addr, BYTE Value);
    void WriteMem16(DWORD Addr, WORD Value);

    /* Push/pop with an explicit width, updating ESP. */
    void PushStack(x86opcode *Op, int Size, DWORD Value);
    DWORD PopStack(x86opcode *Op, int Size);

    /* ==================================================================
     * Condition codes (the tttn encoding used by Jcc / SETcc / CMOVcc)
     * ================================================================== */
    enum x86_cc
    {
        CC_O = 0, CC_NO, CC_B, CC_AE, CC_E, CC_NE, CC_BE, CC_A,
        CC_S, CC_NS, CC_P, CC_NP, CC_L, CC_GE, CC_LE, CC_G
    };

    /* Evaluate a condition code against the current EFLAGS. */
    BOOL TestCondition(PCPU_STATE Cpu, int Cond);

    /* ==================================================================
     * Group-1 ALU operations (ADD/OR/ADC/SBB/AND/SUB/XOR/CMP)
     * ================================================================== */
    enum x86_alu
    {
        ALU_ADD = 0, ALU_OR, ALU_ADC, ALU_SBB, ALU_AND, ALU_SUB, ALU_XOR, ALU_CMP
    };

    /* Apply an ALU operation of the given width and update EFLAGS.
     * The result is returned; the caller stores it unless Op==ALU_CMP. */
    DWORD AluOp(PCPU_STATE Cpu, int Op, int Size, DWORD A, DWORD B);

    /* ==================================================================
     * Shared control-transfer / conditional helpers
     * ================================================================== */

    /* Jcc rel8 / Jcc rel32.  A taken branch moves CurrentPtr to the target so
     * that the interpreter resumes execution there. */
    bool FASTCALL DoJcc8(x86opcode *Op, int Cond);
    bool FASTCALL DoJcc32(x86opcode *Op, int Cond);

    /* SETcc r/m8 */
    bool FASTCALL DoSetcc(x86opcode *Op, int Cond);

    /* CMOVcc r32, r/m32 */
    bool FASTCALL DoCmovcc(x86opcode *Op, int Cond);

    /* MOVZX / MOVSX: load SrcSize bytes and zero- or sign-extend them into
     * the destination register (whose width is OpSize32). */
    bool FASTCALL MovExtend(x86opcode *Op, int SrcSize, BOOL SignExtend);

    bool FASTCALL ProcessCommand(x86opcode *Op);	// process command, return true==continue

    typedef bool FASTCALL COMMAND(x86opcode *Op);

    extern COMMAND *OneByteCmds[256];
    extern COMMAND *TwoByteCmds[256];

#ifdef __cplusplus
}
#endif

#endif // __YACTEMU_H
