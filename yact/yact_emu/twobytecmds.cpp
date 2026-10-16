#include "stdafx.h"
#include "yact_emu.h"

// Forward declarations from operands.cpp
extern DWORD ReadReg(PCPU_STATE cpu, int reg_idx);
extern void WriteReg(PCPU_STATE cpu, int reg_idx, DWORD value);
extern DWORD ReadMem32(DWORD addr);
extern void WriteMem32(DWORD addr, DWORD value);
extern int ResolveOperand(x86opcode *Op, int operand_index, DWORD *out_addr, BOOL *is_memory);

/* ------------------------------------------------------------------
 * Helpers for the 0F-prefixed instructions.
 * ------------------------------------------------------------------ */

/* SHLD/SHRD: shift Dst by Count bits, filling from Src. */
static DWORD ShiftDual(int Size, DWORD Dst, DWORD Src, DWORD Count, BOOL Left, DWORD *pCF)
{
    DWORD bits = (DWORD)(Size * 8);
    DWORD res;

    Count &= 31;                       /* the count is masked to 5 bits */
    if (Count == 0 || Count >= bits)   /* Count >= operand size is undefined */
        return Dst & SizeMask(Size);

    if (Left)
    {
        res = (Dst << Count) | (Src >> (bits - Count));
        *pCF = (Dst >> (bits - Count)) & 1;
    }
    else
    {
        res = (Dst >> Count) | (Src << (bits - Count));
        *pCF = (Dst >> (Count - 1)) & 1;
    }
    return res & SizeMask(Size);
}

/* BT/BTS/BTR/BTC with a bit index taken from a register. */
static bool FASTCALL BitTestReg(x86opcode *Op, int Kind)
{
    x86operand Dst;
    int size;
    DWORD idx, bitval = 0, byteoff = 0, bitpos = 0;
    BOOL is_mem;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    size = OpSize32(Op);
    idx = ReadRegSized(Op->Cpu, Op->Reg, size);
    is_mem = Dst.IsMem;

    if (is_mem)
    {
        /* Bit-string form: the register bit offset is signed and addresses
         * bits before or after the operand (SDM: the byte displacement is
         * BitOffset DIV 8 rounded towards minus infinity). */
        int soff = (int)idx;
        byteoff = (DWORD)(soff >> 3);
        bitpos = (DWORD)(soff & 7);
        bitval = (ReadMem8(Dst.Addr + byteoff) >> bitpos) & 1;
    }
    else
    {
        bitpos = idx & (DWORD)(size * 8 - 1);
        bitval = (ReadRegSized(Op->Cpu, Dst.Reg, size) >> bitpos) & 1;
    }

    if (bitval) Op->Cpu->eflags |= EFLAGS_CF;
    else Op->Cpu->eflags &= ~EFLAGS_CF;

    if (Kind != 0)
    {
        if (is_mem)
        {
            BYTE b = ReadMem8(Dst.Addr + byteoff);
            if (Kind == 1) b |= (BYTE)(1 << bitpos);
            else if (Kind == 2) b &= (BYTE)~(1 << bitpos);
            else b ^= (BYTE)(1 << bitpos);
            WriteMem8(Dst.Addr + byteoff, b);
        }
        else
        {
            DWORD v = ReadRegSized(Op->Cpu, Dst.Reg, size);
            if (Kind == 1) v |= (1u << bitpos);
            else if (Kind == 2) v &= ~(1u << bitpos);
            else v ^= (1u << bitpos);
            WriteRegSized(Op->Cpu, Dst.Reg, size, v);
        }
    }
    return false;
}

/* BT/BTS/BTR/BTC with an 8-bit immediate bit index (group 8). */
static bool FASTCALL BitTestImm(x86opcode *Op, int Kind)
{
    x86operand Dst;
    int size;
    DWORD idx, bitval = 0, byteoff = 0, bitpos = 0;
    BOOL is_mem;

    /* FetchModRM() has already been done by CmdBA, which needs Op->Reg to
     * select the operation. */
    GetRmOperand(Op, &Dst);
    size = OpSize32(Op);
    idx = FetchImm8(Op);
    is_mem = Dst.IsMem;

    if (is_mem)
    {
        byteoff = idx >> 3;
        bitpos = idx & 7;
        bitval = (ReadMem8(Dst.Addr + byteoff) >> bitpos) & 1;
    }
    else
    {
        bitpos = idx & (DWORD)(size * 8 - 1);
        bitval = (ReadRegSized(Op->Cpu, Dst.Reg, size) >> bitpos) & 1;
    }

    if (bitval) Op->Cpu->eflags |= EFLAGS_CF;
    else Op->Cpu->eflags &= ~EFLAGS_CF;

    if (Kind != 0)
    {
        if (is_mem)
        {
            BYTE b = ReadMem8(Dst.Addr + byteoff);
            if (Kind == 1) b |= (BYTE)(1 << bitpos);
            else if (Kind == 2) b &= (BYTE)~(1 << bitpos);
            else b ^= (BYTE)(1 << bitpos);
            WriteMem8(Dst.Addr + byteoff, b);
        }
        else
        {
            DWORD v = ReadRegSized(Op->Cpu, Dst.Reg, size);
            if (Kind == 1) v |= (1u << bitpos);
            else if (Kind == 2) v &= ~(1u << bitpos);
            else v ^= (1u << bitpos);
            WriteRegSized(Op->Cpu, Dst.Reg, size, v);
        }
    }
    return false;
}

/* SHLD/SHRD r/m, r, imm8 or CL */
static bool FASTCALL ShiftDualOp(x86opcode *Op, BOOL Left, BOOL CountInCL)
{
    x86operand Dst;
    int size;
    DWORD count, v, src, cf = 0, res, msb;

    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    size = OpSize32(Op);
    count = CountInCL ? ReadRegSized(Op->Cpu, 1, 1) : (DWORD)FetchImm8(Op);
    v = LoadOperand(Op, &Dst, size);
    src = ReadRegSized(Op->Cpu, Op->Reg, size);

    if ((count & 31) == 0 || (count & 31) >= (DWORD)(size * 8))
        return false;                       /* no shift: flags are not affected */

    res = ShiftDual(size, v, src, count, Left, &cf);
    StoreOperand(Op, &Dst, size, res);

    FlagsLogical(Op->Cpu, size, res);       /* SF/ZF/PF from the result */
    if (cf) Op->Cpu->eflags |= EFLAGS_CF;
    if ((count & 31) == 1)                  /* OF is defined only for count==1 */
    {
        /* SDM: OF is set when the MSB of the original destination differs
         * from the MSB of the result (for SHLD this is also written as
         * MSB(result) XOR CF, because for count==1 CF is the old MSB). */
        msb = (v & SizeSign(size)) ? 1u : 0u;
        if (msb != ((res & SizeSign(size)) ? 1u : 0u)) Op->Cpu->eflags |= EFLAGS_OF;
    }
    return false;
}

/* BSF / BSR */
static bool FASTCALL BitScan(x86opcode *Op, BOOL Reverse)
{
    x86operand Src;
    int size;
    DWORD v, i;

    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    size = OpSize32(Op);
    v = LoadOperand(Op, &Src, size) & SizeMask(size);

    if (v == 0)
    {
        Op->Cpu->eflags |= EFLAGS_ZF;       /* destination is undefined */
        return false;
    }
    if (Reverse)
    {
        for (i = size * 8 - 1; i > 0; i--)
            if (v & (1u << i)) break;
    }
    else
    {
        for (i = 0; i < (DWORD)(size * 8); i++)
            if (v & (1u << i)) break;
    }
    WriteRegSized(Op->Cpu, Op->Reg, size, i);
    Op->Cpu->eflags &= ~EFLAGS_ZF;
    return false;
}

/* IMUL r, r/m */
static bool FASTCALL ImulReg(x86opcode *Op)
{
    x86operand Src;
    int size;
    DWORD a, b, res;
    long long prod;

    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    size = OpSize32(Op);
    a = ReadRegSized(Op->Cpu, Op->Reg, size);
    b = LoadOperand(Op, &Src, size);

    if (size == 2)
    {
        prod = (long long)(int)(short)a * (long long)(int)(short)b;
        res = (DWORD)prod;
        WriteRegSized(Op->Cpu, Op->Reg, 2, res);
        if ((long long)(int)(short)res != prod) Op->Cpu->eflags |= (EFLAGS_CF | EFLAGS_OF);
        else Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_OF);
    }
    else
    {
        prod = (long long)(int)a * (long long)(int)b;
        res = (DWORD)prod;
        WriteRegSized(Op->Cpu, Op->Reg, 4, res);
        if ((long long)(int)res != prod) Op->Cpu->eflags |= (EFLAGS_CF | EFLAGS_OF);
        else Op->Cpu->eflags &= ~(EFLAGS_CF | EFLAGS_OF);
    }
    return false;
}

static bool FASTCALL Cmd00(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd01(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd02(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd03(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd04(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd05(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd06(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd07(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd08(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd09(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd0A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd0B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd0C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd0D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd0E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd0F(x86opcode *Op)
{
    return false;
}


static bool FASTCALL Cmd10(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd11(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd12(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd13(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd14(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd15(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd16(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd17(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd18(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd19(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd1A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd1B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd1C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd1D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd1E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd1F(x86opcode *Op) // NOP r/m (multi-byte NOP)
{
    x86operand Ignored;
    FetchModRM(Op);
    GetRmOperand(Op, &Ignored);   /* consume ModR/M, SIB and displacement */
    Op->Mnemonics = "nop";
    return false;
}

static bool FASTCALL Cmd20(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd21(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd22(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd23(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd24(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd25(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd26(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd27(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd28(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd29(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd2A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd2B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd2C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd2D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd2E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd2F(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd30(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd31(x86opcode *Op)
{
    // RDTSC (0F 31) - Read Time-Stamp Counter
    // Simplified: return 0 in EDX:EAX
    Op->Cpu->eax = 0;
    Op->Cpu->edx = 0;
    Op->Cpu->eip += 2;
    return false;
}

static bool FASTCALL Cmd32(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd33(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd34(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd35(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd36(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd37(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd38(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd39(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd3A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd3B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd3C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd3D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd3E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd3F(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd40(x86opcode *Op) { return DoCmovcc(Op, CC_O); }

static bool FASTCALL Cmd41(x86opcode *Op) { return DoCmovcc(Op, CC_NO); }

static bool FASTCALL Cmd42(x86opcode *Op) { return DoCmovcc(Op, CC_B); }

static bool FASTCALL Cmd43(x86opcode *Op) { return DoCmovcc(Op, CC_AE); }

static bool FASTCALL Cmd44(x86opcode *Op) { return DoCmovcc(Op, CC_E); }

static bool FASTCALL Cmd45(x86opcode *Op) { return DoCmovcc(Op, CC_NE); }

static bool FASTCALL Cmd46(x86opcode *Op) { return DoCmovcc(Op, CC_BE); }

static bool FASTCALL Cmd47(x86opcode *Op) { return DoCmovcc(Op, CC_A); }

static bool FASTCALL Cmd48(x86opcode *Op) { return DoCmovcc(Op, CC_S); }

static bool FASTCALL Cmd49(x86opcode *Op) { return DoCmovcc(Op, CC_NS); }

static bool FASTCALL Cmd4A(x86opcode *Op) { return DoCmovcc(Op, CC_P); }

static bool FASTCALL Cmd4B(x86opcode *Op) { return DoCmovcc(Op, CC_NP); }

static bool FASTCALL Cmd4C(x86opcode *Op) { return DoCmovcc(Op, CC_L); }

static bool FASTCALL Cmd4D(x86opcode *Op) { return DoCmovcc(Op, CC_GE); }

static bool FASTCALL Cmd4E(x86opcode *Op) { return DoCmovcc(Op, CC_LE); }

static bool FASTCALL Cmd4F(x86opcode *Op) { return DoCmovcc(Op, CC_G); }

static bool FASTCALL Cmd50(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd51(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd52(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd53(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd54(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd55(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd56(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd57(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd58(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd59(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd5A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd5B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd5C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd5D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd5E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd5F(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd60(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd61(x86opcode *Op)
{
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

static bool FASTCALL Cmd64(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd65(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd66(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd67(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd68(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd69(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd6A(x86opcode *Op)
{
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

static bool FASTCALL Cmd70(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd71(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd72(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd73(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd74(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd75(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd76(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd77(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd78(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd79(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd7A(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd7B(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd7C(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd7D(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd7E(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd7F(x86opcode *Op)
{
    return false;
}

static bool FASTCALL Cmd80(x86opcode *Op) { return DoJcc32(Op, CC_O); }

static bool FASTCALL Cmd81(x86opcode *Op) { return DoJcc32(Op, CC_NO); }

static bool FASTCALL Cmd82(x86opcode *Op) { return DoJcc32(Op, CC_B); }

static bool FASTCALL Cmd83(x86opcode *Op) { return DoJcc32(Op, CC_AE); }

static bool FASTCALL Cmd84(x86opcode *Op) { return DoJcc32(Op, CC_E); }

static bool FASTCALL Cmd85(x86opcode *Op) { return DoJcc32(Op, CC_NE); }

static bool FASTCALL Cmd86(x86opcode *Op) { return DoJcc32(Op, CC_BE); }

static bool FASTCALL Cmd87(x86opcode *Op) { return DoJcc32(Op, CC_A); }

static bool FASTCALL Cmd88(x86opcode *Op) { return DoJcc32(Op, CC_S); }

static bool FASTCALL Cmd89(x86opcode *Op) { return DoJcc32(Op, CC_NS); }

static bool FASTCALL Cmd8A(x86opcode *Op) { return DoJcc32(Op, CC_P); }

static bool FASTCALL Cmd8B(x86opcode *Op) { return DoJcc32(Op, CC_NP); }

static bool FASTCALL Cmd8C(x86opcode *Op) { return DoJcc32(Op, CC_L); }

static bool FASTCALL Cmd8D(x86opcode *Op) { return DoJcc32(Op, CC_GE); }

static bool FASTCALL Cmd8E(x86opcode *Op) { return DoJcc32(Op, CC_LE); }

static bool FASTCALL Cmd8F(x86opcode *Op) { return DoJcc32(Op, CC_G); }

static bool FASTCALL Cmd90(x86opcode *Op) { return DoSetcc(Op, CC_O); }

static bool FASTCALL Cmd91(x86opcode *Op) { return DoSetcc(Op, CC_NO); }

static bool FASTCALL Cmd92(x86opcode *Op) { return DoSetcc(Op, CC_B); }

static bool FASTCALL Cmd93(x86opcode *Op) { return DoSetcc(Op, CC_AE); }

static bool FASTCALL Cmd94(x86opcode *Op) { return DoSetcc(Op, CC_E); }

static bool FASTCALL Cmd95(x86opcode *Op) { return DoSetcc(Op, CC_NE); }

static bool FASTCALL Cmd96(x86opcode *Op) { return DoSetcc(Op, CC_BE); }

static bool FASTCALL Cmd97(x86opcode *Op) { return DoSetcc(Op, CC_A); }

static bool FASTCALL Cmd98(x86opcode *Op) { return DoSetcc(Op, CC_S); }

static bool FASTCALL Cmd99(x86opcode *Op) { return DoSetcc(Op, CC_NS); }

static bool FASTCALL Cmd9A(x86opcode *Op) { return DoSetcc(Op, CC_P); }

static bool FASTCALL Cmd9B(x86opcode *Op) { return DoSetcc(Op, CC_NP); }

static bool FASTCALL Cmd9C(x86opcode *Op) { return DoSetcc(Op, CC_L); }

static bool FASTCALL Cmd9D(x86opcode *Op) { return DoSetcc(Op, CC_GE); }

static bool FASTCALL Cmd9E(x86opcode *Op) { return DoSetcc(Op, CC_LE); }

static bool FASTCALL Cmd9F(x86opcode *Op) { return DoSetcc(Op, CC_G); }

static bool FASTCALL CmdA0(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdA1(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdA2(x86opcode *Op)
{
    // CPUID (0F A2) - CPU Identification
    // Simplified: return basic Intel-compatible dummy values
    switch (Op->Cpu->eax) {
        case 0:
            // EAX=0: Maximum standard function number
            Op->Cpu->eax = 1;
            Op->Cpu->ebx = 0x756E6547; // "Genu"
            Op->Cpu->ecx = 0x6C65746E; // "ntel"
            Op->Cpu->edx = 0x49656E69; // "ineI"
            break;
        case 1:
            // EAX=1: Processor info and feature flags
            Op->Cpu->eax = 0x000006D8; // Family 6, Model 13, Stepping 8 (Pentium M-ish)
            Op->Cpu->ebx = 0;
            Op->Cpu->ecx = 0;
            Op->Cpu->edx = 0;
            break;
        default:
            // Other functions: return zeros
            Op->Cpu->eax = 0;
            Op->Cpu->ebx = 0;
            Op->Cpu->ecx = 0;
            Op->Cpu->edx = 0;
            break;
    }
    Op->Cpu->eip += 2;
    return false;
}

static bool FASTCALL CmdA3(x86opcode *Op) { return BitTestReg(Op, 0); }

static bool FASTCALL CmdA4(x86opcode *Op) { return ShiftDualOp(Op, TRUE, FALSE); }

static bool FASTCALL CmdA5(x86opcode *Op) { return ShiftDualOp(Op, TRUE, TRUE); }

static bool FASTCALL CmdA6(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdA7(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdA8(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdA9(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdAA(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdAB(x86opcode *Op) { return BitTestReg(Op, 1); }

static bool FASTCALL CmdAC(x86opcode *Op) { return ShiftDualOp(Op, FALSE, FALSE); }

static bool FASTCALL CmdAD(x86opcode *Op) { return ShiftDualOp(Op, FALSE, TRUE); }

static bool FASTCALL CmdAE(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdAF(x86opcode *Op) { return ImulReg(Op); }

/* CMPXCHG r/m, r (0F B0/B1).  The accumulator is compared with r/m: when
 * they are equal r is stored into r/m, otherwise r/m is loaded into the
 * accumulator.  The comparison and the stores use the operand size. */
static bool FASTCALL Cmpxchg(x86opcode *Op, int Size)
{
    x86operand Rm;
    DWORD a, b;

    FetchModRM(Op);
    GetRmOperand(Op, &Rm);
    a = ReadRegSized(Op->Cpu, 0, Size);        /* AL / AX / EAX */
    b = LoadOperand(Op, &Rm, Size);
    FlagsSub(Op->Cpu, Size, a - b, a, b);

    if (Op->Cpu->eflags & EFLAGS_ZF)
        StoreOperand(Op, &Rm, Size, ReadRegSized(Op->Cpu, Op->Reg, Size));
    else
        WriteRegSized(Op->Cpu, 0, Size, b);
    return false;
}

static bool FASTCALL CmdB0(x86opcode *Op) { return Cmpxchg(Op, 1); }

static bool FASTCALL CmdB1(x86opcode *Op) { return Cmpxchg(Op, OpSize32(Op)); }

static bool FASTCALL CmdB2(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdB3(x86opcode *Op) { return BitTestReg(Op, 2); }

static bool FASTCALL CmdB4(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdB5(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdB6(x86opcode *Op) { return MovExtend(Op, 1, FALSE); }

static bool FASTCALL CmdB7(x86opcode *Op) { return MovExtend(Op, 2, FALSE); }

static bool FASTCALL CmdB8(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdB9(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdBA(x86opcode *Op) // group 8: BT/BTS/BTR/BTC r/m, imm8
{
    FetchModRM(Op);
    switch (Op->Reg)
    {
        case 4: return BitTestImm(Op, 0);
        case 5: return BitTestImm(Op, 1);
        case 6: return BitTestImm(Op, 2);
        case 7: return BitTestImm(Op, 3);
    }
    Op->Mnemonics = "invalid";
    Op->Finished = true;
    return false;
}

static bool FASTCALL CmdBB(x86opcode *Op) { return BitTestReg(Op, 3); }

static bool FASTCALL CmdBC(x86opcode *Op) { return BitScan(Op, FALSE); }

static bool FASTCALL CmdBD(x86opcode *Op) { return BitScan(Op, TRUE); }

static bool FASTCALL CmdBE(x86opcode *Op) { return MovExtend(Op, 1, TRUE); }

static bool FASTCALL CmdBF(x86opcode *Op) { return MovExtend(Op, 2, TRUE); }

/* XADD r/m, r (0F C0/C1): store the sum in the destination and the old
 * destination value in the register. */
static bool FASTCALL Xadd(x86opcode *Op, int Size)
{
    x86operand Rm;
    DWORD a, b, sum;

    FetchModRM(Op);
    GetRmOperand(Op, &Rm);
    a = LoadOperand(Op, &Rm, Size);            /* old r/m value */
    b = ReadRegSized(Op->Cpu, Op->Reg, Size);
    sum = a + b;
    FlagsAdd(Op->Cpu, Size, sum, a, b);
    StoreOperand(Op, &Rm, Size, sum);
    WriteRegSized(Op->Cpu, Op->Reg, Size, a);
    return false;
}

static bool FASTCALL CmdC0(x86opcode *Op) { return Xadd(Op, 1); }

static bool FASTCALL CmdC1(x86opcode *Op) { return Xadd(Op, OpSize32(Op)); }

static bool FASTCALL CmdC2(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdC3(x86opcode *Op)
{
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

static bool FASTCALL CmdC6(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdC7(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdC8(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 0, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 0, 4, v);
    return false;
}

static bool FASTCALL CmdC9(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 1, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 1, 4, v);
    return false;
}

static bool FASTCALL CmdCA(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 2, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 2, 4, v);
    return false;
}

static bool FASTCALL CmdCB(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 3, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 3, 4, v);
    return false;
}

static bool FASTCALL CmdCC(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 4, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 4, 4, v);
    return false;
}

static bool FASTCALL CmdCD(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 5, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 5, 4, v);
    return false;
}

static bool FASTCALL CmdCE(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 6, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 6, 4, v);
    return false;
}

static bool FASTCALL CmdCF(x86opcode *Op) // BSWAP r32
{
    DWORD v = ReadRegSized(Op->Cpu, 7, 4);
    v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    WriteRegSized(Op->Cpu, 7, 4, v);
    return false;
}

static bool FASTCALL CmdD0(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD1(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD2(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD3(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD4(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD5(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD6(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdD7(x86opcode *Op)
{
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

static bool FASTCALL CmdE0(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE1(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE2(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE3(x86opcode *Op)
{
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

static bool FASTCALL CmdE8(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdE9(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdEA(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdEB(x86opcode *Op)
{
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

static bool FASTCALL CmdF0(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF1(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF2(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF3(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF4(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF5(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF6(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF7(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF8(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdF9(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdFA(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdFB(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdFC(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdFD(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdFE(x86opcode *Op)
{
    return false;
}

static bool FASTCALL CmdFF(x86opcode *Op)
{
    return false;
}

/* Dispatch table for the 0F (two-byte) opcode space.
 *
 * The handlers listed below are still placeholders (they return false without
 * changing any CPU state); they are NOT implemented instructions:
 *
 *   00-07      system (SLDT/STR/LLDT/LTR/VERR/VERW, SGDT/SIDT/LGDT/LIDT/
 *              SMSW/LMSW/INVLPG, LAR/LSL, SYSCALL/CLTS/SYSRET)
 *   08 09      INVD / WBINVD            0A-0F      reserved / UD2 / prefetch
 *   10-17      SSE moves                 18-1F      hints / NOP
 *   20-27      MOV to/from CR/DR/TR      28-2F      SSE
 *   30 32-37   MSR / SENTER / GETSEC     38-3F      three-byte opcode escapes
 *   50-5F      SSE                       60-6F      MMX
 *   70-7F      MMX / SSE shuffle         A0 A1      PUSH/POP FS
 *   A6 A7      undocumented 386          A8 A9      PUSH/POP GS
 *   AA         RSM                       AE         SSE control / fences
 *   B2 B4 B5   LSS / LFS / LGS           B8 B9      JMPE / UD1
 *   C2-C7      SSE / MOVNTI / GRP9        D0-D7      MMX / SSE2
 *   D8-DF      MMX / SSE2                E0-E7      MMX / SSE2
 *   E8-EF      MMX                       F0-FF      MMX / SSE / UD0
 *
 * They are intentionally left as placeholders: dosbox_emu.dll implements the
 * integer, MMX and some system instructions (but no SSE), bochs_emu.dll is a
 * full CPU core built with BX_CPU_LEVEL 5 (no MMX/SSE), and this engine is not
 * loaded at runtime.  Executing one of them here is a silent no-op, not
 * "support".
 */
COMMAND *TwoByteCmds[256] =
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
