#include "stdafx.h"
#include "yact_emu.h"

// Forward declarations from operands.cpp
extern DWORD ReadReg(PCPU_STATE cpu, int reg_idx);
extern void WriteReg(PCPU_STATE cpu, int reg_idx, DWORD value);
extern DWORD ReadMem32(DWORD addr);
extern void WriteMem32(DWORD addr, DWORD value);
extern int ResolveOperand(x86opcode *Op, int operand_index, DWORD *out_addr, BOOL *is_memory);

/* ------------------------------------------------------------------
 * Instruction helpers used by the handlers in this file.
 * ------------------------------------------------------------------ */

/* Group-1 ALU on an r/m operand: resolves the operand, reads the immediate
 * and stores the result unless the operation is CMP. */
static bool FASTCALL Group1(x86opcode *Op, int Size, BOOL ImmediateIsByte)
{
    x86operand Dst;
    DWORD a, b, res;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    a = LoadOperand(Op, &Dst, Size);
    if (ImmediateIsByte)
        b = (DWORD)(int)(signed char)FetchImm8(Op);   /* sign-extended imm8 */
    else
        b = FetchImmSized(Op, Size);
    res = AluOp(Op->Cpu, Op->Reg, Size, a, b);
    if (Op->Reg != ALU_CMP)
        StoreOperand(Op, &Dst, Size, res);
    return false;
}

/* ALU with the accumulator and an immediate. */
static bool FASTCALL AccumImmediate(x86opcode *Op, int Opcode, int Size)
{
    DWORD a = ReadRegSized(Op->Cpu, 0, Size);
    DWORD b = (Size == 1) ? (DWORD)FetchImm8(Op) : FetchImmSized(Op, Size);
    DWORD res = AluOp(Op->Cpu, Opcode, Size, a, b);
    if (Opcode != ALU_CMP)
        WriteRegSized(Op->Cpu, 0, Size, res);
    return false;
}

/* ALU with a register destination and an r/m source (02/03/0A/0B/...). */
static bool FASTCALL RegRmAlu(x86opcode *Op, int Opcode, int Size)
{
    x86operand Src;
    DWORD a, b, res;

    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    a = ReadRegSized(Op->Cpu, Op->Reg, Size);
    b = LoadOperand(Op, &Src, Size);
    res = AluOp(Op->Cpu, Opcode, Size, a, b);
    if (Opcode != ALU_CMP)
        WriteRegSized(Op->Cpu, Op->Reg, Size, res);
    return false;
}

/* Compare-and-swap the accumulator and an r/m operand (38..3B). */
static bool FASTCALL RmRegAlu(x86opcode *Op, int Opcode, int Size)
{
    x86operand Dst;
    DWORD a, b, res;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    a = LoadOperand(Op, &Dst, Size);
    b = ReadRegSized(Op->Cpu, Op->Reg, Size);
    res = AluOp(Op->Cpu, Opcode, Size, a, b);
    if (Opcode != ALU_CMP)
        StoreOperand(Op, &Dst, Size, res);
    return false;
}

/* TEST r/m, r: AND flags without storing the result. */
static bool FASTCALL RmRegTest(x86opcode *Op, int Size)
{
    x86operand Rm;
    DWORD a, b;

    FetchModRM(Op);
    GetRmOperand(Op, &Rm);
    a = LoadOperand(Op, &Rm, Size);
    b = ReadRegSized(Op->Cpu, Op->Reg, Size);
    FlagsLogical(Op->Cpu, Size, a & b);
    return false;
}

/* TEST AL/AX/EAX, imm (A8/A9): the accumulator is not modified. */
static bool FASTCALL AccumTest(x86opcode *Op, int Size)
{
    DWORD a = ReadRegSized(Op->Cpu, 0, Size);
    DWORD b = (Size == 1) ? (DWORD)FetchImm8(Op) : FetchImmSized(Op, Size);
    FlagsLogical(Op->Cpu, Size, a & b);
    return false;
}

/* XCHG r/m, r (86/87): swap the operands.  EFLAGS are not affected. */
static bool FASTCALL XchgRmReg(x86opcode *Op, int Size)
{
    x86operand Rm;
    DWORD a, b;

    FetchModRM(Op);
    GetRmOperand(Op, &Rm);
    a = LoadOperand(Op, &Rm, Size);
    b = ReadRegSized(Op->Cpu, Op->Reg, Size);
    StoreOperand(Op, &Rm, Size, b);
    WriteRegSized(Op->Cpu, Op->Reg, Size, a);
    return false;
}

/* XCHG EAX, r32 (91..97); 0x66 selects the 16-bit form. */
static bool FASTCALL XchgAcc(x86opcode *Op)
{
    int Size = OpSize32(Op);
    int Reg = Op->Current & 7;
    DWORD a, b;

    if (Reg == 0) return false;                 /* 90 is NOP */
    a = ReadRegSized(Op->Cpu, 0, Size);
    b = ReadRegSized(Op->Cpu, Reg, Size);
    WriteRegSized(Op->Cpu, 0, Size, b);
    WriteRegSized(Op->Cpu, Reg, Size, a);
    return false;
}

/* INC/DEC r16/r32 (40..4F).  CF is preserved. */
static bool FASTCALL IncDecReg(x86opcode *Op, BOOL Dec)
{
    int Size = OpSize32(Op);
    int Reg = Op->Current & 7;
    DWORD v = ReadRegSized(Op->Cpu, Reg, Size);
    DWORD res = Dec ? (v - 1) : (v + 1);

    WriteRegSized(Op->Cpu, Reg, Size, res);
    FlagsIncDec(Op->Cpu, Size, res, Dec);
    return false;
}


/* MOVS/STOS/LODS style string operation with REP support. */
static bool FASTCALL StringMove(x86opcode *Op, int Size, int Kind)
{
    int delta = (Op->Cpu->eflags & EFLAGS_DF) ? -(int)Size : (int)Size;
    DWORD n = Op->Rep ? Op->Cpu->ecx : 1;

    while (n != 0)
    {
        if (Kind == 0)              /* MOVS */
        {
            if (Size == 1) WriteMem8(Op->Cpu->edi, ReadMem8(Op->Cpu->esi));
            else if (Size == 2) WriteMem16(Op->Cpu->edi, ReadMem16(Op->Cpu->esi));
            else WriteMem32(Op->Cpu->edi, ReadMem32(Op->Cpu->esi));
            Op->Cpu->esi += delta;
            Op->Cpu->edi += delta;
        }
        else if (Kind == 1)         /* STOS */
        {
            DWORD v = ReadRegSized(Op->Cpu, 0, Size);
            if (Size == 1) WriteMem8(Op->Cpu->edi, (BYTE)v);
            else if (Size == 2) WriteMem16(Op->Cpu->edi, (WORD)v);
            else WriteMem32(Op->Cpu->edi, v);
            Op->Cpu->edi += delta;
        }
        else                        /* LODS */
        {
            DWORD v;
            if (Size == 1) v = ReadMem8(Op->Cpu->esi);
            else if (Size == 2) v = ReadMem16(Op->Cpu->esi);
            else v = ReadMem32(Op->Cpu->esi);
            WriteRegSized(Op->Cpu, 0, Size, v);
            Op->Cpu->esi += delta;
        }
        n--;
    }
    if (Op->Rep)
        Op->Cpu->ecx = 0;
    return false;
}

/* CMPS/SCAS with REPE/REPNE support. */
static bool FASTCALL StringCompare(x86opcode *Op, int Size, int Kind)
{
    int delta = (Op->Cpu->eflags & EFLAGS_DF) ? -(int)Size : (int)Size;
    DWORD n = Op->Rep ? Op->Cpu->ecx : 1;

    while (n != 0)
    {
        DWORD a, b;
        if (Kind == 0)              /* CMPS: [ESI] - [EDI] */
        {
            if (Size == 1) { a = ReadMem8(Op->Cpu->esi); b = ReadMem8(Op->Cpu->edi); }
            else if (Size == 2) { a = ReadMem16(Op->Cpu->esi); b = ReadMem16(Op->Cpu->edi); }
            else { a = ReadMem32(Op->Cpu->esi); b = ReadMem32(Op->Cpu->edi); }
            Op->Cpu->esi += delta;
            Op->Cpu->edi += delta;
        }
        else                        /* SCAS: AL/AX/EAX - [EDI] */
        {
            a = ReadRegSized(Op->Cpu, 0, Size);
            if (Size == 1) b = ReadMem8(Op->Cpu->edi);
            else if (Size == 2) b = ReadMem16(Op->Cpu->edi);
            else b = ReadMem32(Op->Cpu->edi);
            Op->Cpu->edi += delta;
        }
        FlagsSub(Op->Cpu, Size, a - b, a, b);
        n--;
        if (Op->Rep)
        {
            Op->Cpu->ecx = n;
            /* REPE stops on ZF==0, REPNE stops on ZF==1 */
            if ((Op->Rep == repe) != ((Op->Cpu->eflags & EFLAGS_ZF) != 0))
                break;
        }
    }
    return false;
}

/* Group 4 (FE) and group 5 (FF). */
static bool FASTCALL IncDecRm(x86opcode *Op, int Size)
{
    x86operand Dst;
    DWORD v;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    v = LoadOperand(Op, &Dst, Size);
    if (Op->Reg == 0)
    {
        FlagsIncDec(Op->Cpu, Size, v + 1, FALSE);
        StoreOperand(Op, &Dst, Size, v + 1);
    }
    else if (Op->Reg == 1)
    {
        FlagsIncDec(Op->Cpu, Size, v - 1, TRUE);
        StoreOperand(Op, &Dst, Size, v - 1);
    }
    else
    {
        Op->Mnemonics = "invalid";
        Op->Finished = true;
    }
    return false;
}

static bool FASTCALL Group5(x86opcode *Op)
{
    x86operand Dst;
    int size;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    size = OpSize32(Op);

    switch (Op->Reg)
    {
        case 0:     /* INC r/m */
        {
            DWORD v = LoadOperand(Op, &Dst, size);
            FlagsIncDec(Op->Cpu, size, v + 1, FALSE);
            StoreOperand(Op, &Dst, size, v + 1);
            break;
        }
        case 1:     /* DEC r/m */
        {
            DWORD v = LoadOperand(Op, &Dst, size);
            FlagsIncDec(Op->Cpu, size, v - 1, TRUE);
            StoreOperand(Op, &Dst, size, v - 1);
            break;
        }
        case 2:     /* CALL r/m */
        {
            DWORD target = LoadOperand(Op, &Dst, size);
            PushStack(Op, size, PtrToDword(Op->CurrentPtr));
            Op->CurrentPtr = DwordToPtr(target);
            break;
        }
        case 4:     /* JMP r/m */
            Op->CurrentPtr = DwordToPtr(LoadOperand(Op, &Dst, size));
            break;
        case 6:     /* PUSH r/m */
            PushStack(Op, size, LoadOperand(Op, &Dst, size));
            break;
        default:
            Op->Mnemonics = "invalid";
            Op->Finished = true;
            break;
    }
    return false;
}

static bool FASTCALL Cmd00(x86opcode *Op) { return RmRegAlu(Op, ALU_ADD, 1); }

// ADD r/m, reg (Cmd01)
static bool FASTCALL Cmd01(x86opcode *Op) { return RmRegAlu(Op, ALU_ADD, OpSize32(Op)); }

static bool FASTCALL Cmd02(x86opcode *Op) { return RegRmAlu(Op, ALU_ADD, 1); }

static bool FASTCALL Cmd03(x86opcode *Op) { return RegRmAlu(Op, ALU_ADD, OpSize32(Op)); }

static bool FASTCALL Cmd04(x86opcode *Op) { return AccumImmediate(Op, ALU_ADD, 1); }

static bool FASTCALL Cmd05(x86opcode *Op) { return AccumImmediate(Op, ALU_ADD, OpSize32(Op)); }

static bool FASTCALL Cmd06(x86opcode *Op) // PUSH ES
{
    Op->Cpu->esp -= 4;
    WriteMem32(Op->Cpu->esp, (DWORD)Op->Cpu->es);
    Op->Cpu->eip += 1;
    return false;
}

static bool FASTCALL Cmd07(x86opcode *Op) // POP ES
{
    Op->Cpu->es = (WORD)ReadMem32(Op->Cpu->esp);
    Op->Cpu->esp += 4;
    Op->Cpu->eip += 1;
    return false;
}

// AND r/m, reg (Cmd20) ～ AND AL, imm8 (Cmd24)
static bool FASTCALL Cmd20(x86opcode *Op) { return RmRegAlu(Op, ALU_AND, 1); }
static bool FASTCALL Cmd21(x86opcode *Op) { return RmRegAlu(Op, ALU_AND, OpSize32(Op)); }
static bool FASTCALL Cmd22(x86opcode *Op) { return RegRmAlu(Op, ALU_AND, 1); }
static bool FASTCALL Cmd23(x86opcode *Op) { return RegRmAlu(Op, ALU_AND, OpSize32(Op)); }
static bool FASTCALL Cmd24(x86opcode *Op) { return AccumImmediate(Op, ALU_AND, 1); }
static bool FASTCALL Cmd25(x86opcode *Op) { return AccumImmediate(Op, ALU_AND, OpSize32(Op)); }

// OR r/m, reg (Cmd08) ～ OR AL, imm8 (Cmd0C)
static bool FASTCALL Cmd08(x86opcode *Op) { return RmRegAlu(Op, ALU_OR, 1); }
static bool FASTCALL Cmd09(x86opcode *Op) { return RmRegAlu(Op, ALU_OR, OpSize32(Op)); }
static bool FASTCALL Cmd0A(x86opcode *Op) { return RegRmAlu(Op, ALU_OR, 1); }
static bool FASTCALL Cmd0B(x86opcode *Op) { return RegRmAlu(Op, ALU_OR, OpSize32(Op)); }
static bool FASTCALL Cmd0C(x86opcode *Op) { return AccumImmediate(Op, ALU_OR, 1); }
static bool FASTCALL Cmd0D(x86opcode *Op) { return AccumImmediate(Op, ALU_OR, OpSize32(Op)); }

// XOR r/m, reg (Cmd30) ～ XOR AL, imm8 (Cmd34)
static bool FASTCALL Cmd30(x86opcode *Op) { return RmRegAlu(Op, ALU_XOR, 1); }
static bool FASTCALL Cmd31(x86opcode *Op) { return RmRegAlu(Op, ALU_XOR, OpSize32(Op)); }
static bool FASTCALL Cmd32(x86opcode *Op) { return RegRmAlu(Op, ALU_XOR, 1); }
static bool FASTCALL Cmd33(x86opcode *Op) { return RegRmAlu(Op, ALU_XOR, OpSize32(Op)); }
static bool FASTCALL Cmd34(x86opcode *Op) { return AccumImmediate(Op, ALU_XOR, 1); }
static bool FASTCALL Cmd35(x86opcode *Op) { return AccumImmediate(Op, ALU_XOR, OpSize32(Op)); }

// TEST r/m, reg (Cmd84) ～ TEST EAX, imm32 (Cmd85)
static bool FASTCALL Cmd84(x86opcode *Op) { return RmRegTest(Op, 1); }
static bool FASTCALL Cmd85(x86opcode *Op) { return RmRegTest(Op, OpSize32(Op)); }

static bool FASTCALL Cmd0E(x86opcode *Op) // PUSH CS
{
    Op->Cpu->esp -= 4;
    WriteMem32(Op->Cpu->esp, (DWORD)Op->Cpu->cs);
    Op->Cpu->eip += 1;
    return false;
}

static bool FASTCALL Cmd0F(x86opcode *Op)
{
    Op->Current = FetchB(Op);
    return (TwoByteCmds[Op->Current])(Op);
}


// ADC r/m, reg (Cmd10) ～ ADC EAX, imm32 (Cmd15)
static bool FASTCALL Cmd10(x86opcode *Op) { return RmRegAlu(Op, ALU_ADC, 1); }
static bool FASTCALL Cmd11(x86opcode *Op) { return RmRegAlu(Op, ALU_ADC, OpSize32(Op)); }
static bool FASTCALL Cmd12(x86opcode *Op) { return RegRmAlu(Op, ALU_ADC, 1); }
static bool FASTCALL Cmd13(x86opcode *Op) { return RegRmAlu(Op, ALU_ADC, OpSize32(Op)); }
static bool FASTCALL Cmd14(x86opcode *Op) { return AccumImmediate(Op, ALU_ADC, 1); }
static bool FASTCALL Cmd15(x86opcode *Op) { return AccumImmediate(Op, ALU_ADC, OpSize32(Op)); }

static bool FASTCALL Cmd16(x86opcode *Op) // PUSH SS
{
    Op->Cpu->esp -= 4;
    WriteMem32(Op->Cpu->esp, (DWORD)Op->Cpu->ss);
    Op->Cpu->eip += 1;
    return false;
}

static bool FASTCALL Cmd17(x86opcode *Op) // POP SS
{
    Op->Cpu->ss = (WORD)ReadMem32(Op->Cpu->esp);
    Op->Cpu->esp += 4;
    Op->Cpu->eip += 1;
    return false;
}

// SBB r/m, reg (Cmd18) ～ SBB EAX, imm32 (Cmd1D)
static bool FASTCALL Cmd18(x86opcode *Op) { return RmRegAlu(Op, ALU_SBB, 1); }
static bool FASTCALL Cmd19(x86opcode *Op) { return RmRegAlu(Op, ALU_SBB, OpSize32(Op)); }
static bool FASTCALL Cmd1A(x86opcode *Op) { return RegRmAlu(Op, ALU_SBB, 1); }
static bool FASTCALL Cmd1B(x86opcode *Op) { return RegRmAlu(Op, ALU_SBB, OpSize32(Op)); }
static bool FASTCALL Cmd1C(x86opcode *Op) { return AccumImmediate(Op, ALU_SBB, 1); }
static bool FASTCALL Cmd1D(x86opcode *Op) { return AccumImmediate(Op, ALU_SBB, OpSize32(Op)); }

static bool FASTCALL Cmd1E(x86opcode *Op) // PUSH DS
{
    Op->Cpu->esp -= 4;
    WriteMem32(Op->Cpu->esp, (DWORD)Op->Cpu->ds);
    Op->Cpu->eip += 1;
    return false;
}

static bool FASTCALL Cmd1F(x86opcode *Op) // POP DS
{
    Op->Cpu->ds = (WORD)ReadMem32(Op->Cpu->esp);
    Op->Cpu->esp += 4;
    Op->Cpu->eip += 1;
    return false;
}

// Removed duplicate definitions - Cmd20-Cmd25 already defined above at lines 75-80

static bool FASTCALL Cmd26(x86opcode *Op) // ES segment override prefix
{
    /* Flat 32-bit address space: the override is accepted and ignored. */
    return true;
}

static bool FASTCALL Cmd27(x86opcode *Op) // DAA
{
    DWORD al = ReadRegSized(Op->Cpu, 0, 1);
    DWORD old_al = al;
    DWORD old_cf = (Op->Cpu->eflags & EFLAGS_CF) ? 1u : 0u;
    DWORD af = (Op->Cpu->eflags & EFLAGS_AF) ? 1u : 0u;

    Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_AF);
    if ((al & 0x0F) > 9 || af)
    {
        al += 6;
        Op->Cpu->eflags |= EFLAGS_AF;
    }
    if (old_al > 0x99 || old_cf)
    {
        al += 0x60;
        Op->Cpu->eflags |= EFLAGS_CF;
    }
    WriteRegSized(Op->Cpu, 0, 1, al);
    if (al & 0x80) Op->Cpu->eflags |= EFLAGS_SF; else Op->Cpu->eflags &= ~EFLAGS_SF;
    if ((al & 0xFF) == 0) Op->Cpu->eflags |= EFLAGS_ZF; else Op->Cpu->eflags &= ~EFLAGS_ZF;
    if (CalcParity(al)) Op->Cpu->eflags |= EFLAGS_PF; else Op->Cpu->eflags &= ~EFLAGS_PF;
    return false;
}

static bool FASTCALL Cmd28(x86opcode *Op) { return RmRegAlu(Op, ALU_SUB, 1); }

// SUB r/m, reg (Cmd29)
static bool FASTCALL Cmd29(x86opcode *Op) { return RmRegAlu(Op, ALU_SUB, OpSize32(Op)); }

static bool FASTCALL Cmd2A(x86opcode *Op) { return RegRmAlu(Op, ALU_SUB, 1); }

static bool FASTCALL Cmd2B(x86opcode *Op) { return RegRmAlu(Op, ALU_SUB, OpSize32(Op)); }

static bool FASTCALL Cmd2C(x86opcode *Op) { return AccumImmediate(Op, ALU_SUB, 1); }

static bool FASTCALL Cmd2D(x86opcode *Op) { return AccumImmediate(Op, ALU_SUB, OpSize32(Op)); }

// SegCS
static bool FASTCALL Cmd2E(x86opcode *Op) // CS segment override prefix
{
    /* Flat 32-bit address space: the override is accepted and ignored. */
    return true;
}

static bool FASTCALL Cmd2F(x86opcode *Op) // DAS
{
    DWORD al = ReadRegSized(Op->Cpu, 0, 1);
    DWORD old_al = al;
    DWORD old_cf = (Op->Cpu->eflags & EFLAGS_CF) ? 1u : 0u;
    DWORD af = (Op->Cpu->eflags & EFLAGS_AF) ? 1u : 0u;

    Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_AF);
    if ((al & 0x0F) > 9 || af)
    {
        al -= 6;
        Op->Cpu->eflags |= EFLAGS_AF;
    }
    if (old_al > 0x99 || old_cf)
    {
        al -= 0x60;
        Op->Cpu->eflags |= EFLAGS_CF;
    }
    WriteRegSized(Op->Cpu, 0, 1, al);
    if (al & 0x80) Op->Cpu->eflags |= EFLAGS_SF; else Op->Cpu->eflags &= ~EFLAGS_SF;
    if ((al & 0xFF) == 0) Op->Cpu->eflags |= EFLAGS_ZF; else Op->Cpu->eflags &= ~EFLAGS_ZF;
    if (CalcParity(al)) Op->Cpu->eflags |= EFLAGS_PF; else Op->Cpu->eflags &= ~EFLAGS_PF;
    return false;
}

// Removed duplicate definitions - Cmd30-Cmd35 already defined above at lines 91-96

static bool FASTCALL Cmd36(x86opcode *Op) // SS segment override prefix
{
    /* Flat 32-bit address space: the override is accepted and ignored. */
    return true;
}

static bool FASTCALL Cmd37(x86opcode *Op) // AAA
{
    DWORD al = ReadRegSized(Op->Cpu, 0, 1);
    DWORD ah = ReadRegSized(Op->Cpu, 4, 1);
    DWORD af = (Op->Cpu->eflags & EFLAGS_AF) ? 1u : 0u;

    Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_AF);
    if ((al & 0x0F) > 9 || af)
    {
        al += 6;
        ah = (ah + 1) & 0xFF;
        Op->Cpu->eflags |= EFLAGS_CF | EFLAGS_AF;
    }
    WriteRegSized(Op->Cpu, 0, 1, al & 0x0F);
    WriteRegSized(Op->Cpu, 4, 1, ah);
    return false;
}

static bool FASTCALL Cmd38(x86opcode *Op) { return RmRegAlu(Op, ALU_CMP, 1); }

// CMP r/m, reg (Cmd39)
static bool FASTCALL Cmd39(x86opcode *Op) { return RmRegAlu(Op, ALU_CMP, OpSize32(Op)); }

static bool FASTCALL Cmd3A(x86opcode *Op) { return RegRmAlu(Op, ALU_CMP, 1); }

static bool FASTCALL Cmd3B(x86opcode *Op) { return RegRmAlu(Op, ALU_CMP, OpSize32(Op)); }

static bool FASTCALL Cmd3C(x86opcode *Op) { return AccumImmediate(Op, ALU_CMP, 1); }

static bool FASTCALL Cmd3D(x86opcode *Op) { return AccumImmediate(Op, ALU_CMP, OpSize32(Op)); }

// SegDS
static bool FASTCALL Cmd3E(x86opcode *Op) // DS segment override prefix
{
    /* Flat 32-bit address space: the override is accepted and ignored. */
    return true;
}

static bool FASTCALL Cmd3F(x86opcode *Op) // AAS
{
    DWORD al = ReadRegSized(Op->Cpu, 0, 1);
    DWORD ah = ReadRegSized(Op->Cpu, 4, 1);
    DWORD af = (Op->Cpu->eflags & EFLAGS_AF) ? 1u : 0u;

    Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_AF);
    if ((al & 0x0F) > 9 || af)
    {
        al -= 6;
        ah = (ah - 1) & 0xFF;
        Op->Cpu->eflags |= EFLAGS_CF | EFLAGS_AF;
    }
    WriteRegSized(Op->Cpu, 0, 1, al & 0x0F);
    WriteRegSized(Op->Cpu, 4, 1, ah);
    return false;
}

// INC EAX (Cmd40) ～ INC EDI (Cmd47)
static bool FASTCALL Cmd40(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd41(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd42(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd43(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd44(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd45(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd46(x86opcode *Op) { return IncDecReg(Op, FALSE); }
static bool FASTCALL Cmd47(x86opcode *Op) { return IncDecReg(Op, FALSE); }

// DEC EAX (Cmd48) ～ DEC EDI (Cmd4F)
static bool FASTCALL Cmd48(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd49(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd4A(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd4B(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd4C(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd4D(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd4E(x86opcode *Op) { return IncDecReg(Op, TRUE); }
static bool FASTCALL Cmd4F(x86opcode *Op) { return IncDecReg(Op, TRUE); }

// PUSH EAX (Cmd50) ～ PUSH EDI (Cmd57)
/* PUSH/POP of a general register.  The operand size is 32 bits unless the
 * 0x66 prefix selects 16 bits. */

static bool FASTCALL PushReg(x86opcode *Op, int Reg)
{
    int size = OpSize32(Op);
    PushStack(Op, size, ReadRegSized(Op->Cpu, Reg, size));
    return false;
}

static bool FASTCALL PopReg(x86opcode *Op, int Reg)
{
    int size = OpSize32(Op);
    DWORD v = PopStack(Op, size);

    if (Reg == 4)
    {
        /* POP ESP: the popped value replaces ESP (the implicit ESP += size
         * that PopStack() performed is overwritten). */
        if (size == 2)
            Op->Cpu->esp = (Op->Cpu->esp & 0xFFFF0000u) | (v & 0xFFFF);
        else
            Op->Cpu->esp = v;
        return false;
    }
    WriteRegSized(Op->Cpu, Reg, size, v);
    return false;
}

static bool FASTCALL Cmd50(x86opcode *Op) { return PushReg(Op, 0); }
static bool FASTCALL Cmd51(x86opcode *Op) { return PushReg(Op, 1); }
static bool FASTCALL Cmd52(x86opcode *Op) { return PushReg(Op, 2); }
static bool FASTCALL Cmd53(x86opcode *Op) { return PushReg(Op, 3); }
static bool FASTCALL Cmd54(x86opcode *Op) { return PushReg(Op, 4); }
static bool FASTCALL Cmd55(x86opcode *Op) { return PushReg(Op, 5); }
static bool FASTCALL Cmd56(x86opcode *Op) { return PushReg(Op, 6); }
static bool FASTCALL Cmd57(x86opcode *Op) { return PushReg(Op, 7); }

// POP EAX (Cmd58) ～ POP EDI (Cmd5F)
static bool FASTCALL Cmd58(x86opcode *Op) { return PopReg(Op, 0); }
static bool FASTCALL Cmd59(x86opcode *Op) { return PopReg(Op, 1); }
static bool FASTCALL Cmd5A(x86opcode *Op) { return PopReg(Op, 2); }
static bool FASTCALL Cmd5B(x86opcode *Op) { return PopReg(Op, 3); }

static bool FASTCALL Cmd5C(x86opcode *Op) { return PopReg(Op, 4); }

static bool FASTCALL Cmd5D(x86opcode *Op) { return PopReg(Op, 5); }

static bool FASTCALL Cmd5E(x86opcode *Op) { return PopReg(Op, 6); }

static bool FASTCALL Cmd5F(x86opcode *Op) { return PopReg(Op, 7); }

static bool FASTCALL Cmd60(x86opcode *Op) // PUSHA - Push All General Purpose Registers
{
    DWORD esp_temp = Op->Cpu->esp;
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->eax);
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->ecx);
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->edx);
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->ebx);
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->esp); // Original ESP
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->ebp);
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->esi);
    esp_temp -= 4; WriteMem32(esp_temp, Op->Cpu->edi);
    Op->Cpu->esp = esp_temp;
    Op->Cpu->eip += 1;
    return false;
}

static bool FASTCALL Cmd61(x86opcode *Op) // POPA - Pop All General Purpose Registers
{
    DWORD esp_temp = Op->Cpu->esp;
    Op->Cpu->edi = ReadMem32(esp_temp); esp_temp += 4;
    Op->Cpu->esi = ReadMem32(esp_temp); esp_temp += 4;
    Op->Cpu->ebp = ReadMem32(esp_temp); esp_temp += 4;
    esp_temp += 4; // Skip ESP (popped value is discarded)
    Op->Cpu->ebx = ReadMem32(esp_temp); esp_temp += 4;
    Op->Cpu->edx = ReadMem32(esp_temp); esp_temp += 4;
    Op->Cpu->ecx = ReadMem32(esp_temp); esp_temp += 4;
    Op->Cpu->eax = ReadMem32(esp_temp); esp_temp += 4;
    Op->Cpu->esp = esp_temp;
    Op->Cpu->eip += 1;
    return false;
}

static bool FASTCALL Cmd62(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd63(x86opcode *Op)
{
    return false;
}

// SegFS
static bool FASTCALL Cmd64(x86opcode *Op)
{
    Op->SegFS = true;
    return true;
}

// SegGS
static bool FASTCALL Cmd65(x86opcode *Op)
{
    return true;
}

// OpSize
static bool FASTCALL Cmd66(x86opcode *Op)
{
    Op->Opsize = true;
    return true;
}

// AddrSize
static bool FASTCALL Cmd67(x86opcode *Op)
{
    Op->Addrsize = true;
    return true;
}

static bool FASTCALL Cmd68(x86opcode *Op) // PUSH imm16/imm32
{
    int size = OpSize32(Op);
    DWORD imm = FetchImmSized(Op, size);
    PushStack(Op, size, imm);
    return false;
}

static bool FASTCALL Cmd69(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd6A(x86opcode *Op) // PUSH imm8 (sign-extended)
{
    int size = OpSize32(Op);
    DWORD imm = (DWORD)(int)(signed char)FetchImm8(Op);
    PushStack(Op, size, imm);
    return false;
}

static bool FASTCALL Cmd6B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd6C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd6D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd6E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd6F(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd70(x86opcode *Op) { return DoJcc8(Op, CC_O); }

static bool FASTCALL Cmd71(x86opcode *Op) { return DoJcc8(Op, CC_NO); }

static bool FASTCALL Cmd72(x86opcode *Op) { return DoJcc8(Op, CC_B); }

static bool FASTCALL Cmd73(x86opcode *Op) { return DoJcc8(Op, CC_AE); }

static bool FASTCALL Cmd74(x86opcode *Op) { return DoJcc8(Op, CC_E); }

static bool FASTCALL Cmd75(x86opcode *Op) { return DoJcc8(Op, CC_NE); }

static bool FASTCALL Cmd76(x86opcode *Op) { return DoJcc8(Op, CC_BE); }

static bool FASTCALL Cmd77(x86opcode *Op) { return DoJcc8(Op, CC_A); }

static bool FASTCALL Cmd78(x86opcode *Op) { return DoJcc8(Op, CC_S); }

static bool FASTCALL Cmd79(x86opcode *Op) { return DoJcc8(Op, CC_NS); }

static bool FASTCALL Cmd7A(x86opcode *Op) { return DoJcc8(Op, CC_P); }

static bool FASTCALL Cmd7B(x86opcode *Op) { return DoJcc8(Op, CC_NP); }

static bool FASTCALL Cmd7C(x86opcode *Op) { return DoJcc8(Op, CC_L); }

static bool FASTCALL Cmd7D(x86opcode *Op) { return DoJcc8(Op, CC_GE); }

static bool FASTCALL Cmd7E(x86opcode *Op) { return DoJcc8(Op, CC_LE); }

static bool FASTCALL Cmd7F(x86opcode *Op) { return DoJcc8(Op, CC_G); }

static bool FASTCALL Cmd80(x86opcode *Op) { return Group1(Op, 1, TRUE); }

static bool FASTCALL Cmd81(x86opcode *Op) { return Group1(Op, OpSize32(Op), FALSE); }

static bool FASTCALL Cmd82(x86opcode *Op) // undocumented alias of 80h
{
    return Cmd80(Op);
}

static bool FASTCALL Cmd83(x86opcode *Op) { return Group1(Op, OpSize32(Op), TRUE); }

// Removed - already defined above at line 88-89

// XCHG r/m, reg (Cmd86/Cmd87) and XCHG EAX, reg (Cmd90-Cmd97)
static bool FASTCALL Cmd86(x86opcode *Op) { return XchgRmReg(Op, 1); }
static bool FASTCALL Cmd87(x86opcode *Op) { return XchgRmReg(Op, OpSize32(Op)); }

static bool FASTCALL Cmd88(x86opcode *Op) // MOV r/m8, r8
{
    x86operand Dst;
    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    StoreOperand(Op, &Dst, 1, ReadRegSized(Op->Cpu, Op->Reg, 1));
    return false;
}

static bool FASTCALL Cmd89(x86opcode *Op) // MOV r/m16/32, r16/32
{
    x86operand Dst;
    int size;
    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    size = OpSize32(Op);
    StoreOperand(Op, &Dst, size, ReadRegSized(Op->Cpu, Op->Reg, size));
    return false;
}

static bool FASTCALL Cmd8A(x86opcode *Op) // MOV r8, r/m8
{
    x86operand Src;
    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    WriteRegSized(Op->Cpu, Op->Reg, 1, LoadOperand(Op, &Src, 1));
    return false;
}

static bool FASTCALL Cmd8B(x86opcode *Op) // MOV r16/32, r/m16/32
{
    x86operand Src;
    int size;
    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    size = OpSize32(Op);
    WriteRegSized(Op->Cpu, Op->Reg, size, LoadOperand(Op, &Src, size));
    return false;
}

static bool FASTCALL Cmd8C(x86opcode *Op) // MOV r/m16, Sreg
{
    x86operand Dst;
    DWORD v;
    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    switch (Op->Reg)
    {
        case 0: v = Op->Cpu->es; break;
        case 1: v = Op->Cpu->cs; break;
        case 2: v = Op->Cpu->ss; break;
        case 3: v = Op->Cpu->ds; break;
        case 4: v = Op->Cpu->fs; break;
        case 5: v = Op->Cpu->gs; break;
        default: v = 0; break;
    }
    StoreOperand(Op, &Dst, 2, v);
    return false;
}

// LEA reg, r/m (Cmd8D) - Load Effective Address
static bool FASTCALL Cmd8D(x86opcode *Op) { FetchModRM(Op); DWORD addr; BOOL is_mem; int reg_idx = ResolveOperand(Op, 1, &addr, &is_mem); ResolveOperand(Op, 0, &addr, &is_mem); WriteReg(Op->Cpu, reg_idx, addr); Op->Cpu->eip += 2; return false; }

// MOV r/m16, Sreg (Cmd8C) and MOV Sreg, r/m16 (Cmd8E), POP r/m (Cmd8F)

static bool FASTCALL Cmd8E(x86opcode *Op) // MOV Sreg, r/m16
{
    x86operand Src;
    DWORD v;
    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    v = LoadOperand(Op, &Src, 2);
    switch (Op->Reg)
    {
        case 0: Op->Cpu->es = (WORD)v; break;
        case 2: Op->Cpu->ss = (WORD)v; break;
        case 3: Op->Cpu->ds = (WORD)v; break;
        case 4: Op->Cpu->fs = (WORD)v; break;
        case 5: Op->Cpu->gs = (WORD)v; break;
        default: break;      /* 1 (CS) is not a valid destination */
    }
    return false;
}

static bool FASTCALL Cmd8F(x86opcode *Op) // POP r/m
{
    x86operand Dst;
    int size = OpSize32(Op);
    BYTE *ModrmPtr = Op->CurrentPtr;        /* the ModR/M byte */

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);

    DWORD val = PopStack(Op, size);

    if (Dst.IsMem)
    {
        /* SDM: if ESP is used as the base register of a memory destination,
         * the effective address is computed after ESP has been incremented.
         * Re-decode the r/m operand with the updated ESP; the decode is
         * deterministic, so it consumes exactly the same bytes again. */
        Op->CurrentPtr = ModrmPtr;
        FetchModRM(Op);
        GetRmOperand(Op, &Dst);
        StoreOperand(Op, &Dst, size, val);
    }
    else
    {
        WriteRegSized(Op->Cpu, Dst.Reg, size, val);
    }
    return false;
}

// XCHG EAX, reg (Cmd90-Cmd97) - NOP when reg=EAX
static bool FASTCALL Cmd90(x86opcode *Op) { /* XCHG EAX, EAX = NOP */ Op->Cpu->eip += 1; return false; }
static bool FASTCALL Cmd91(x86opcode *Op) { return XchgAcc(Op); }
static bool FASTCALL Cmd92(x86opcode *Op) { return XchgAcc(Op); }
static bool FASTCALL Cmd93(x86opcode *Op) { return XchgAcc(Op); }
static bool FASTCALL Cmd94(x86opcode *Op) { return XchgAcc(Op); }
static bool FASTCALL Cmd95(x86opcode *Op) { return XchgAcc(Op); }
static bool FASTCALL Cmd96(x86opcode *Op) { return XchgAcc(Op); }
static bool FASTCALL Cmd97(x86opcode *Op) { return XchgAcc(Op); }

static bool FASTCALL Cmd98(x86opcode *Op) // CBW / CWDE
{
    if (Op->Opsize)
        WriteRegSized(Op->Cpu, 0, 2, (DWORD)(int)(signed char)ReadRegSized(Op->Cpu, 0, 1));
    else
        WriteRegSized(Op->Cpu, 0, 4, (DWORD)(int)(signed short)ReadRegSized(Op->Cpu, 0, 2));
    return false;
}

static bool FASTCALL Cmd99(x86opcode *Op) // CWD / CDQ
{
    if (Op->Opsize)
        WriteRegSized(Op->Cpu, 2, 2, (Op->Cpu->eax & 0x8000) ? 0xFFFF : 0x0000);
    else
        Op->Cpu->edx = (Op->Cpu->eax & 0x80000000) ? 0xFFFFFFFF : 0x00000000;
    return false;
}

static bool FASTCALL Cmd9A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd9B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd9C(x86opcode *Op) // PUSHF
{
    if (Op->Opsize)
        PushStack(Op, 2, Op->Cpu->eflags & 0xFFFF);
    else
        PushStack(Op, 4, Op->Cpu->eflags);
    return false;
}

static bool FASTCALL Cmd9D(x86opcode *Op) // POPF
{
    if (Op->Opsize)
    {
        DWORD v = PopStack(Op, 2);
        Op->Cpu->eflags = (Op->Cpu->eflags & 0xFFFF0000u) | (v & 0xFFFF);
    }
    else
    {
        DWORD v = PopStack(Op, 4);
        /* bit 1 always reads as 1; the IOPL/NT/RF/VM/VIF/VIP bits are not
         * modelled, so they are cleared. */
        Op->Cpu->eflags = (v & 0x00040DD5u) | 0x00000002u;
    }
    return false;
}

static bool FASTCALL Cmd9E(x86opcode *Op) // SAHF
{
    DWORD ah = ReadRegSized(Op->Cpu, 4, 1);
    DWORD f = Op->Cpu->eflags & ~(DWORD)(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF);
    if (ah & 0x01) f |= EFLAGS_CF;
    if (ah & 0x04) f |= EFLAGS_PF;
    if (ah & 0x10) f |= EFLAGS_AF;
    if (ah & 0x40) f |= EFLAGS_ZF;
    if (ah & 0x80) f |= EFLAGS_SF;
    Op->Cpu->eflags = f;
    return false;
}

static bool FASTCALL Cmd9F(x86opcode *Op) // LAHF
{
    DWORD f = Op->Cpu->eflags;
    DWORD ah = 0x02;
    if (f & EFLAGS_CF) ah |= 0x01;
    if (f & EFLAGS_PF) ah |= 0x04;
    if (f & EFLAGS_AF) ah |= 0x10;
    if (f & EFLAGS_ZF) ah |= 0x40;
    if (f & EFLAGS_SF) ah |= 0x80;
    WriteRegSized(Op->Cpu, 4, 1, ah);
    return false;
}

static bool FASTCALL CmdA0(x86opcode *Op) // MOV AL, moffs8
{
    DWORD addr = FetchImm32(Op);
    WriteRegSized(Op->Cpu, 0, 1, ReadMem8(addr));
    return false;
}

static bool FASTCALL CmdA1(x86opcode *Op) // MOV EAX, moffs32
{
    DWORD addr = FetchImm32(Op);
    int size = OpSize32(Op);
    DWORD v = (size == 2) ? (DWORD)ReadMem16(addr) : ReadMem32(addr);
    WriteRegSized(Op->Cpu, 0, size, v);
    return false;
}

static bool FASTCALL CmdA2(x86opcode *Op) // MOV moffs8, AL
{
    DWORD addr = FetchImm32(Op);
    WriteMem8(addr, (BYTE)ReadRegSized(Op->Cpu, 0, 1));
    return false;
}

static bool FASTCALL CmdA3(x86opcode *Op) // MOV moffs32, EAX
{
    DWORD addr = FetchImm32(Op);
    int size = OpSize32(Op);
    DWORD v = ReadRegSized(Op->Cpu, 0, size);
    if (size == 2) WriteMem16(addr, (WORD)v);
    else WriteMem32(addr, v);
    return false;
}

static bool FASTCALL CmdA4(x86opcode *Op) { return StringMove(Op, 1, 0); }

static bool FASTCALL CmdA5(x86opcode *Op) { return StringMove(Op, OpSize32(Op), 0); }

static bool FASTCALL CmdA6(x86opcode *Op) { return StringCompare(Op, 1, 0); }

static bool FASTCALL CmdA7(x86opcode *Op) { return StringCompare(Op, OpSize32(Op), 0); }

static bool FASTCALL CmdA8(x86opcode *Op) { return AccumTest(Op, 1); }

static bool FASTCALL CmdA9(x86opcode *Op) { return AccumTest(Op, OpSize32(Op)); }

static bool FASTCALL CmdAA(x86opcode *Op) { return StringMove(Op, 1, 1); }

static bool FASTCALL CmdAB(x86opcode *Op) { return StringMove(Op, OpSize32(Op), 1); }

static bool FASTCALL CmdAC(x86opcode *Op) { return StringMove(Op, 1, 2); }

static bool FASTCALL CmdAD(x86opcode *Op) { return StringMove(Op, OpSize32(Op), 2); }

static bool FASTCALL CmdAE(x86opcode *Op) { return StringCompare(Op, 1, 1); }

static bool FASTCALL CmdAF(x86opcode *Op) { return StringCompare(Op, OpSize32(Op), 1); }

static bool FASTCALL CmdB0(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 0, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB1(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 1, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB2(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 2, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB3(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 3, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB4(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 4, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB5(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 5, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB6(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 6, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB7(x86opcode *Op) // MOV r8, imm8
{
    WriteRegSized(Op->Cpu, 7, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdB8(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 0, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdB9(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 1, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdBA(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 2, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdBB(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 3, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdBC(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 4, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdBD(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 5, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdBE(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 6, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdBF(x86opcode *Op) // MOV r16/32, imm16/32
{
    int size = OpSize32(Op);
    WriteRegSized(Op->Cpu, 7, size, FetchImmSized(Op, size));
    return false;
}

// SHL/SHR/ROL/ROR etc r/m8, imm8 (CmdC0)
/* ------------------------------------------------------------------
 * Group 2: shift / rotate r/m by 1, CL or imm8.
 *
 *   /0 ROL   /1 ROR   /2 RCL   /3 RCR
 *   /4 SHL   /5 SHR   /6 SAL   /7 SAR
 *
 * The hardware masks the count to 5 bits.  All shifts are evaluated with
 * 64-bit intermediates so that no count can cause undefined behaviour on the
 * host, and out-of-range bit positions read as zero.
 * ------------------------------------------------------------------ */

static inline DWORD ShlWide(DWORD v, DWORD n)
{
    return (DWORD)(((unsigned long long)v << (n & 63)) & 0xFFFFFFFFull);
}

static inline DWORD ShrWide(DWORD v, DWORD n)
{
    if (n >= 32) return 0;
    return v >> n;
}

/* Bit idx of v; idx outside [0,31] reads as zero. */
static inline DWORD BitWide(DWORD v, int idx)
{
    if (idx < 0 || idx >= 32) return 0;
    return (v >> idx) & 1;
}

/* CountSource: 0 = implicit 1, 1 = CL, 2 = imm8 */
static bool FASTCALL ShiftRotate(x86opcode *Op, int Size, int CountSource)
{
    x86operand Dst;
    DWORD bits = (DWORD)(Size * 8);
    DWORD mask = SizeMask(Size);
    DWORD sign = SizeSign(Size);
    DWORD v, res = 0, cf = 0, of = 0, Count;
    BOOL set_cf = FALSE, set_of = FALSE, set_szp = FALSE;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    v = LoadOperand(Op, &Dst, Size) & mask;

    if (CountSource == 0) Count = 1;
    else if (CountSource == 1) Count = ReadRegSized(Op->Cpu, 1, 1);
    else Count = FetchImm8(Op);

    Count &= 31;                    /* the hardware masks the count to 5 bits */
    if (Count == 0)
        return false;               /* nothing happens and no flags change */

    switch (Op->Reg)
    {
        case 0:     /* ROL */
        {
            DWORD n = Count % bits;
            if (n == 0) return false;
            res = (ShlWide(v, n) | ShrWide(v, bits - n)) & mask;
            cf = res & 1;
            set_cf = TRUE;
            set_of = (n == 1);
            of = (((res & sign) != 0) != (cf != 0)) ? 1u : 0u;
            break;
        }
        case 1:     /* ROR */
        {
            DWORD n = Count % bits;
            if (n == 0) return false;
            res = (ShrWide(v, n) | ShlWide(v, bits - n)) & mask;
            cf = (res & sign) ? 1u : 0u;
            set_cf = TRUE;
            set_of = (n == 1);
            of = (((res & sign) != 0) != (((res << 1) & sign) != 0)) ? 1u : 0u;
            break;
        }
        case 2:     /* RCL */
        case 3:     /* RCR */
        {
            unsigned long long wide, mask64, r;
            DWORD n = Count % (bits + 1);
            if (n == 0) return false;
            DWORD c = (Op->Cpu->eflags & EFLAGS_CF) ? 1u : 0u;
            /* The rotate-through-carry value is (bits + 1) wide: the carry
             * lives in bit `bits`, so the 32-bit case needs 33 bits. */
            mask64 = (bits == 32) ? 0x1FFFFFFFFull : ((1ull << (bits + 1)) - 1ull);
            wide = ((unsigned long long)c << bits) | v;
            if (Op->Reg == 2)
                r = ((wide << n) | (wide >> (bits + 1 - n))) & mask64;
            else
                r = ((wide >> n) | (wide << (bits + 1 - n))) & mask64;
            res = (DWORD)(r & mask);
            cf = (DWORD)((r >> bits) & 1);
            set_cf = TRUE;
            set_of = (n == 1);
            if (Op->Reg == 2)
                of = (((res & sign) != 0) != (cf != 0)) ? 1u : 0u;
            else
                of = (((res & sign) != 0) != ((res & (sign >> 1)) != 0)) ? 1u : 0u;
            break;
        }
        case 4:     /* SHL */
        case 6:     /* SAL */
            res = ShlWide(v, Count) & mask;
            cf = BitWide(v, (int)(bits - Count));
            set_cf = TRUE;
            set_of = TRUE;
            of = (((res & sign) != 0) != (cf != 0)) ? 1u : 0u;
            set_szp = TRUE;
            break;
        case 5:     /* SHR */
            res = ShrWide(v, Count);
            cf = BitWide(v, (int)Count - 1);
            set_cf = TRUE;
            set_of = TRUE;
            of = ((v & sign) != 0) ? 1u : 0u;
            set_szp = TRUE;
            break;
        case 7:     /* SAR */
            res = ShrWide(v, Count);
            if (v & sign)
                res |= (~(mask >> Count)) & mask;
            cf = BitWide(v, (int)Count - 1);
            set_cf = TRUE;
            set_of = TRUE;              /* OF is always cleared for SAR */
            of = 0;
            set_szp = TRUE;
            break;
        default:
            return false;
    }

    StoreOperand(Op, &Dst, Size, res & mask);

    if (set_cf)
    {
        if (cf) Op->Cpu->eflags |= EFLAGS_CF; else Op->Cpu->eflags &= ~EFLAGS_CF;
    }
    if (set_of)
    {
        if (of) Op->Cpu->eflags |= EFLAGS_OF; else Op->Cpu->eflags &= ~EFLAGS_OF;
    }
    if (set_szp)
    {
        DWORD f = Op->Cpu->eflags & ~(DWORD)(EFLAGS_SF | EFLAGS_ZF | EFLAGS_PF);
        DWORD r = res & mask;
        if (r == 0) f |= EFLAGS_ZF;
        if (r & sign) f |= EFLAGS_SF;
        if (CalcParity(r)) f |= EFLAGS_PF;
        Op->Cpu->eflags = f;
    }
    return false;
}

static bool FASTCALL CmdC0(x86opcode *Op) { return ShiftRotate(Op, 1, 2); }

// SHL/SHR/ROL/ROR etc r/m32, imm8 (CmdC1)
static bool FASTCALL CmdC1(x86opcode *Op) { return ShiftRotate(Op, OpSize32(Op), 2); }

static bool FASTCALL CmdC2(x86opcode *Op) // RET imm16
{
    WORD n = FetchImm16(Op);
    DWORD target = PopStack(Op, 4);
    Op->Cpu->esp += n;
    Op->CurrentPtr = DwordToPtr(target);
    return false;
}

static bool FASTCALL CmdC3(x86opcode *Op) // RET
{
    Op->CurrentPtr = DwordToPtr(PopStack(Op, 4));
    return false;
}

static bool FASTCALL CmdC4(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdC5(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdC6(x86opcode *Op) // MOV r/m8, imm8
{
    x86operand Dst;
    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    StoreOperand(Op, &Dst, 1, FetchImm8(Op));
    return false;
}

static bool FASTCALL CmdC7(x86opcode *Op) // MOV r/m16/32, imm16/32
{
    x86operand Dst;
    int size;
    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    size = OpSize32(Op);
    StoreOperand(Op, &Dst, size, FetchImmSized(Op, size));
    return false;
}

static bool FASTCALL CmdC8(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdC9(x86opcode *Op) // LEAVE
{
    Op->Cpu->esp = Op->Cpu->ebp;
    Op->Cpu->ebp = PopStack(Op, 4);
    return false;
}

static bool FASTCALL CmdCA(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdCB(x86opcode *Op)
{
    return false;
}

// Int3
static bool FASTCALL CmdCC(x86opcode *Op)
{
    Op->Finished = true;
    return false;
}

// Int X
static bool FASTCALL CmdCD(x86opcode *Op)
{
    Op->Finished = true;
    return false;
}

static bool FASTCALL CmdCE(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdCF(x86opcode *Op)
{
    return false;
}

// D0-D3: Shift/Rotate by 1 or by CL
static bool FASTCALL CmdD0(x86opcode *Op) { return ShiftRotate(Op, 1, 0); }

static bool FASTCALL CmdD1(x86opcode *Op) { return ShiftRotate(Op, OpSize32(Op), 0); }

static bool FASTCALL CmdD2(x86opcode *Op) { return ShiftRotate(Op, 1, 1); }

static bool FASTCALL CmdD3(x86opcode *Op) { return ShiftRotate(Op, OpSize32(Op), 1); }

static bool FASTCALL CmdD4(x86opcode *Op) // AAM
{
    BYTE base = FetchImm8(Op);
    DWORD al;
    if (base == 0)
    {
        /* division by zero: no exception model, so stop the interpreter */
        Op->Mnemonics = "invalid";
        Op->Finished = true;
        return false;
    }
    al = ReadRegSized(Op->Cpu, 0, 1);
    /* AAM: AH = AL / base, AL = AL % base (the quotient goes to AH). */
    WriteRegSized(Op->Cpu, 4, 1, al / base);
    WriteRegSized(Op->Cpu, 0, 1, al % base);
    FlagsLogical(Op->Cpu, 1, al % base);
    return false;
}

static bool FASTCALL CmdD5(x86opcode *Op) // AAD
{
    BYTE base = FetchImm8(Op);
    DWORD al = ReadRegSized(Op->Cpu, 0, 1);
    DWORD ah = ReadRegSized(Op->Cpu, 4, 1);
    DWORD res = (al + ah * base) & 0xFF;
    WriteRegSized(Op->Cpu, 0, 1, res);
    WriteRegSized(Op->Cpu, 4, 1, 0);
    FlagsLogical(Op->Cpu, 1, res);
    return false;
}

static bool FASTCALL CmdD6(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD7(x86opcode *Op) // XLAT
{
    DWORD addr = Op->Cpu->ebx + ReadRegSized(Op->Cpu, 0, 1);
    WriteRegSized(Op->Cpu, 0, 1, ReadMem8(addr));
    return false;
}

static bool FASTCALL CmdD8(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD9(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdDA(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdDB(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdDC(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdDD(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdDE(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdDF(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE0(x86opcode *Op) // LOOPNE/LOOPNZ
{
    signed char rel = (signed char)FetchImm8(Op);
    Op->Cpu->ecx--;
    if (Op->Cpu->ecx != 0 && !(Op->Cpu->eflags & EFLAGS_ZF))
        Op->CurrentPtr += rel;
    return false;
}

static bool FASTCALL CmdE1(x86opcode *Op) // LOOPE/LOOPZ
{
    signed char rel = (signed char)FetchImm8(Op);
    Op->Cpu->ecx--;
    if (Op->Cpu->ecx != 0 && (Op->Cpu->eflags & EFLAGS_ZF))
        Op->CurrentPtr += rel;
    return false;
}

static bool FASTCALL CmdE2(x86opcode *Op) // LOOP
{
    signed char rel = (signed char)FetchImm8(Op);
    Op->Cpu->ecx--;
    if (Op->Cpu->ecx != 0)
        Op->CurrentPtr += rel;
    return false;
}

static bool FASTCALL CmdE3(x86opcode *Op) // JCXZ/JECXZ
{
    signed char rel = (signed char)FetchImm8(Op);
    if (Op->Cpu->ecx == 0)
        Op->CurrentPtr += rel;
    return false;
}

static bool FASTCALL CmdE4(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE5(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE6(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE7(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE8(x86opcode *Op) // CALL rel16/32
{
    int size = OpSize32(Op);
    DWORD rel = FetchRel(Op, size);
    DWORD next = PtrToDword(Op->CurrentPtr);
    PushStack(Op, size, next);
    Op->CurrentPtr = DwordToPtr(next + rel);
    return false;
}

static bool FASTCALL CmdE9(x86opcode *Op) // JMP rel16/32
{
    int size = OpSize32(Op);
    DWORD rel = FetchRel(Op, size);
    Op->CurrentPtr = DwordToPtr(PtrToDword(Op->CurrentPtr) + rel);
    return false;
}

static bool FASTCALL CmdEA(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdEB(x86opcode *Op) // JMP rel8
{
    signed char rel = (signed char)FetchImm8(Op);
    Op->CurrentPtr += rel;
    return false;
}

static bool FASTCALL CmdEC(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdED(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdEE(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdEF(x86opcode *Op)
{
    return false;
}

// Lock
static bool FASTCALL CmdF0(x86opcode *Op)
{
    Op->Lock = true;
    return true;
}

// ICEBP
static bool FASTCALL CmdF1(x86opcode *Op)
{
    Op->Finished = true;
    return false;
}

// RepNE
static bool FASTCALL CmdF2(x86opcode *Op)
{
    Op->Rep = repne;
    return true;
}

// RepE
static bool FASTCALL CmdF3(x86opcode *Op)
{
    Op->Rep = repe;
    return true;
}

// Hlt
static bool FASTCALL CmdF4(x86opcode *Op)
{
    Op->Finished = true;
    return false;
}

static bool FASTCALL CmdF5(x86opcode *Op) // CMC
{
    Op->Cpu->eflags ^= EFLAGS_CF;
    return false;
}

// F6/F7: TEST, NOT, NEG, MUL, IMUL, DIV, IDIV (group 3)
/* F6 /0,/1 TEST r/m8,imm8  /2 NOT  /3 NEG  /4 MUL
 *    /5 IMUL, /6 DIV, /7 IDIV are still unimplemented (see the note below). */
static bool FASTCALL CmdF6(x86opcode *Op)
{
    x86operand Rm;
    DWORD val;

    FetchModRM(Op);
    GetRmOperand(Op, &Rm);
    val = LoadOperand(Op, &Rm, 1);

    switch (Op->Reg)
    {
        case 0: case 1:                 /* TEST r/m8, imm8 */
            FlagsLogical(Op->Cpu, 1, val & (DWORD)FetchImm8(Op));
            break;
        case 2:                         /* NOT r/m8 */
            StoreOperand(Op, &Rm, 1, ~val);
            break;
        case 3:                         /* NEG r/m8: 0 - val */
            StoreOperand(Op, &Rm, 1, AluOp(Op->Cpu, ALU_SUB, 1, 0, val));
            break;
        case 4:                         /* MUL AL, r/m8: AX = AL * r/m8 */
        {
            DWORD res = (Op->Cpu->eax & 0xFF) * val;
            Op->Cpu->eax = (Op->Cpu->eax & 0xFFFF0000u) | (res & 0xFFFF);
            Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_OF);
            if (res & 0xFF00) Op->Cpu->eflags |= EFLAGS_CF | EFLAGS_OF;
            break;
        }
        case 5:                         /* IMUL r/m8 - not implemented */
        case 6:                         /* DIV  r/m8 - not implemented */
        case 7:                         /* IDIV r/m8 - not implemented */
            /* These three forms are unimplemented.  They consume their
             * operand bytes (the length is correct) but produce no result;
             * they are deliberately NOT faked with a NOP that pretends to
             * compute something.  DOSBox and Bochs both implement them. */
            break;
    }
    return false;
}


/* F7 /0,/1 TEST r/m32,imm32  /2 NOT  /3 NEG  /4 MUL
 *    /5 IMUL, /6 DIV, /7 IDIV are still unimplemented (see the note below). */
static bool FASTCALL CmdF7(x86opcode *Op)
{
    x86operand Rm;
    int Size = OpSize32(Op);
    DWORD val;

    FetchModRM(Op);
    GetRmOperand(Op, &Rm);
    val = LoadOperand(Op, &Rm, Size);

    switch (Op->Reg)
    {
        case 0: case 1:                 /* TEST r/m32, imm32 */
            FlagsLogical(Op->Cpu, Size, val & FetchImmSized(Op, Size));
            break;
        case 2:                         /* NOT r/m32 */
            StoreOperand(Op, &Rm, Size, ~val);
            break;
        case 3:                         /* NEG r/m32: 0 - val */
            StoreOperand(Op, &Rm, Size, AluOp(Op->Cpu, ALU_SUB, Size, 0, val));
            break;
        case 4:                         /* MUL EAX, r/m32: EDX:EAX */
        {
            DWORD op = Op->Cpu->eax;
            DWORD lo = op * val;
            DWORD hi = (DWORD)(((unsigned long long)op * (unsigned long long)val) >> 32);
            Op->Cpu->eax = lo;
            Op->Cpu->edx = hi;
            Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_OF);
            if (hi != 0) Op->Cpu->eflags |= EFLAGS_CF | EFLAGS_OF;
            break;
        }
        case 5:                         /* IMUL r/m32 - not implemented */
        case 6:                         /* DIV  r/m32 - not implemented */
        case 7:                         /* IDIV r/m32 - not implemented */
            /* Unimplemented, as for F6 /5 /6 /7 above. */
            break;
    }
    return false;
}


static bool FASTCALL CmdF8(x86opcode *Op) { Op->Cpu->eflags &= ~EFLAGS_CF; return false; }

static bool FASTCALL CmdF9(x86opcode *Op) { Op->Cpu->eflags |= EFLAGS_CF; return false; }

static bool FASTCALL CmdFA(x86opcode *Op) { Op->Cpu->eflags &= ~EFLAGS_IF; return false; }

static bool FASTCALL CmdFB(x86opcode *Op) { Op->Cpu->eflags |= EFLAGS_IF; return false; }

static bool FASTCALL CmdFC(x86opcode *Op) { Op->Cpu->eflags &= ~EFLAGS_DF; return false; }

static bool FASTCALL CmdFD(x86opcode *Op) { Op->Cpu->eflags |= EFLAGS_DF; return false; }

static bool FASTCALL CmdFE(x86opcode *Op) { return IncDecRm(Op, 1); }

static bool FASTCALL CmdFF(x86opcode *Op) { return Group5(Op); }

/* Dispatch table for the one-byte opcode space.
 *
 * The handlers listed below are still placeholders (they return false without
 * changing any CPU state); they are NOT implemented instructions:
 *
 *   62 63      BOUND / ARPL            69 6B      IMUL r,r/m,imm
 *   6C-6F      INS / OUTS              9A         CALL far
 *   9B         FWAIT                    C4 C5      LES / LDS
 *   C8         ENTER                    CA CB      RETF
 *   CE         INTO                     CF         IRET
 *   D6         SALC                     D8-DF      x87 escapes
 *   E4-E7      IN/OUT imm8              EA         JMP far
 *   EC-EF      IN/OUT DX
 *
 * They are intentionally left as placeholders: the engines that are actually
 * loaded at runtime (dosbox_emu.dll, bochs_emu.dll) implement all of them, so
 * re-implementing them here would only duplicate existing code.  Executing one
 * of them in this engine is a silent no-op, not "support".
 */
COMMAND *OneByteCmds[256] =
{
    Cmd00, Cmd01, Cmd02, Cmd03, Cmd04, Cmd05, Cmd06, Cmd07, Cmd08, Cmd09, Cmd0A, Cmd0B, Cmd0C, Cmd0D, Cmd0E, Cmd0F,
    Cmd10, Cmd11, Cmd12, Cmd13, Cmd14, Cmd15, Cmd16, Cmd17, Cmd18, Cmd19, Cmd1A, Cmd1B, Cmd1C, Cmd1D, Cmd1E, Cmd1F,
    Cmd20, Cmd21, Cmd22, Cmd23, Cmd24, Cmd25, Cmd26, Cmd27, Cmd28, Cmd29, Cmd2A, Cmd2B, Cmd2C, Cmd2D, Cmd2E, Cmd2F,
    Cmd30, Cmd31, Cmd32, Cmd33, Cmd34, Cmd35, Cmd36, Cmd37, Cmd38, Cmd39, Cmd3A, Cmd3B, Cmd3C, Cmd3D, Cmd3E, Cmd3F,
    Cmd40, Cmd41, Cmd42, Cmd43, Cmd44, Cmd45, Cmd46, Cmd47, Cmd48, Cmd49, Cmd4A, Cmd4B, Cmd4C, Cmd4D, Cmd4E, Cmd4F,
    Cmd50, Cmd51, Cmd52, Cmd53, Cmd54, Cmd55, Cmd56, Cmd57, Cmd58, Cmd59, Cmd5A, Cmd5B, Cmd5C, Cmd5D, Cmd5E, Cmd5F,
    Cmd60, Cmd61, Cmd62, Cmd63, Cmd64, Cmd65, Cmd66, Cmd67, Cmd68, Cmd69, Cmd6A, Cmd6B, Cmd6C, Cmd6D, Cmd6E, Cmd6F,
    Cmd70, Cmd71, Cmd72, Cmd73, Cmd74, Cmd75, Cmd76, Cmd77, Cmd78, Cmd79, Cmd7A, Cmd7B, Cmd7C, Cmd7D, Cmd7E, Cmd7F,
    Cmd80, Cmd81, Cmd82, Cmd83, Cmd84, Cmd85, Cmd86, Cmd87, Cmd88, Cmd89, Cmd8A, Cmd8B, Cmd8C, Cmd8D, Cmd8E, Cmd8F,
    Cmd90, Cmd91, Cmd92, Cmd93, Cmd94, Cmd95, Cmd96, Cmd97, Cmd98, Cmd99, Cmd9A, Cmd9B, Cmd9C, Cmd9D, Cmd9E, Cmd9F,
    CmdA0, CmdA1, CmdA2, CmdA3, CmdA4, CmdA5, CmdA6, CmdA7, CmdA8, CmdA9, CmdAA, CmdAB, CmdAC, CmdAD, CmdAE, CmdAF,
    CmdB0, CmdB1, CmdB2, CmdB3, CmdB4, CmdB5, CmdB6, CmdB7, CmdB8, CmdB9, CmdBA, CmdBB, CmdBC, CmdBD, CmdBE, CmdBF,
    CmdC0, CmdC1, CmdC2, CmdC3, CmdC4, CmdC5, CmdC6, CmdC7, CmdC8, CmdC9, CmdCA, CmdCB, CmdCC, CmdCD, CmdCE, CmdCF,
    CmdD0, CmdD1, CmdD2, CmdD3, CmdD4, CmdD5, CmdD6, CmdD7, CmdD8, CmdD9, CmdDA, CmdDB, CmdDC, CmdDD, CmdDE, CmdDF,
    CmdE0, CmdE1, CmdE2, CmdE3, CmdE4, CmdE5, CmdE6, CmdE7, CmdE8, CmdE9, CmdEA, CmdEB, CmdEC, CmdED, CmdEE, CmdEF,
    CmdF0, CmdF1, CmdF2, CmdF3, CmdF4, CmdF5, CmdF6, CmdF7, CmdF8, CmdF9, CmdFA, CmdFB, CmdFC, CmdFD, CmdFE, CmdFF,
};
