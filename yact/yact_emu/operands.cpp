#include "stdafx.h"
#include "yact_emu.h"
#include "../inc/x86emul.h"

DWORD ReadReg(PCPU_STATE cpu, int reg_idx) {
    switch (reg_idx) {
        case 0: return cpu->eax; case 1: return cpu->ecx;
        case 2: return cpu->edx; case 3: return cpu->ebx;
        case 4: return cpu->esp; case 5: return cpu->ebp;
        case 6: return cpu->esi; case 7: return cpu->edi;
        default: return 0;
    }
}
void WriteReg(PCPU_STATE cpu, int reg_idx, DWORD value) {
    switch (reg_idx) {
        case 0: cpu->eax = value; break; case 1: cpu->ecx = value; break;
        case 2: cpu->edx = value; break; case 3: cpu->ebx = value; break;
        case 4: cpu->esp = value; break; case 5: cpu->ebp = value; break;
        case 6: cpu->esi = value; break; case 7: cpu->edi = value; break;
    }
}
/* ------------------------------------------------------------------
 * Guest memory access.
 *
 * The accesses are assembled from byte reads/writes instead of using a
 * direct *(DWORD*)addr / *(WORD*)addr cast.  The ARM32 build compiles with
 * -mno-unaligned-access (see Makefile.arm), and x86 guest code is allowed to
 * perform unaligned accesses, so a direct wide load/store would fault on the
 * Windows RT target.  The observable behaviour on x86 hosts is unchanged.
 * ------------------------------------------------------------------ */
BYTE ReadMem8(DWORD addr) { return *(BYTE*)addr; }
void WriteMem8(DWORD addr, BYTE value) { *(BYTE*)addr = value; }
WORD ReadMem16(DWORD addr) { return (WORD)(ReadMem8(addr) | (ReadMem8(addr + 1) << 8)); }
void WriteMem16(DWORD addr, WORD value)
{
    WriteMem8(addr, (BYTE)value);
    WriteMem8(addr + 1, (BYTE)(value >> 8));
}
DWORD ReadMem32(DWORD addr)
{
    return (DWORD)ReadMem8(addr)
         | ((DWORD)ReadMem8(addr + 1) << 8)
         | ((DWORD)ReadMem8(addr + 2) << 16)
         | ((DWORD)ReadMem8(addr + 3) << 24);
}
void WriteMem32(DWORD addr, DWORD value)
{
    WriteMem8(addr, (BYTE)value);
    WriteMem8(addr + 1, (BYTE)(value >> 8));
    WriteMem8(addr + 2, (BYTE)(value >> 16));
    WriteMem8(addr + 3, (BYTE)(value >> 24));
}

/* Read a 32-bit displacement without touching Op->Imm32 (the displacement
 * and the instruction immediate are separate fields in the encoding). */
static DWORD FetchDisp32(x86opcode *Op)
{
    DWORD v = (DWORD)Op->CurrentPtr[0]
            | ((DWORD)Op->CurrentPtr[1] << 8)
            | ((DWORD)Op->CurrentPtr[2] << 16)
            | ((DWORD)Op->CurrentPtr[3] << 24);
    Op->CurrentPtr += 4;
    return v;
}

int ResolveOperand(x86opcode *Op, int operand_index, DWORD *out_addr, BOOL *is_memory) {
    BYTE modrm = Op->ModRM;
    BYTE mod = (modrm >> 6) & 0x03;
    BYTE reg = (modrm >> 3) & 0x07;
    BYTE rm = modrm & 0x07;

    if (operand_index == 1) {
        /* Operand 1 is always the register named by the ModR/M reg field.
         * The out parameters are deliberately left alone: the read-modify-
         * write handlers resolve operand 0 first and then operand 1, and they
         * still need the memory address / is_memory flag of operand 0 when
         * they store the result.  (Clobbering them here made every RMW form
         * such as ADD [mem],reg write to a register instead of to memory.)
         * No bytes are consumed for this operand either. */
        return reg;
    }

    if (mod == 3) {
        *is_memory = FALSE;
        *out_addr = 0;
        return rm;
    }

    DWORD base = 0, index = 0, scale = 1, disp = 0;

    if (rm == 4) {
        /* A SIB byte follows the ModR/M byte. */
        FetchSIB(Op);
        BYTE base_val = (BYTE)Op->Base;
        BYTE index_val = (BYTE)Op->Index;
        scale = 1u << Op->Scale;
        if (index_val != 4)                 /* index==4 encodes "no index" */
            index = ReadReg(Op->Cpu, index_val);
        if (base_val == 5 && mod == 0)      /* no base register: disp32 follows */
            disp = FetchDisp32(Op);
        else
            base = ReadReg(Op->Cpu, base_val);
    }
    else if (mod == 0 && rm == 5) {
        /* [disp32] with no base register. */
        disp = FetchDisp32(Op);
    }
    else {
        base = ReadReg(Op->Cpu, rm);
    }

    if (mod == 1)
        disp += (DWORD)(int)(signed char)FetchB(Op);   /* sign-extended disp8 */
    else if (mod == 2)
        disp += FetchDisp32(Op);

    *out_addr = base + index * scale + disp;
    *is_memory = TRUE;
    return 0xFFFFFFFF;
}

void GetRmOperand(x86opcode *Op, x86operand *Out)
{
    Out->Reg = ResolveOperand(Op, 0, &Out->Addr, &Out->IsMem);
}

void GetRegOperand(x86opcode *Op, x86operand *Out)
{
    /* Unused helper (kept for completeness): operand 1 is a register, so it
     * must never be treated as a memory reference. */
    Out->Addr = 0;
    Out->IsMem = FALSE;
    Out->Reg = ResolveOperand(Op, 1, &Out->Addr, &Out->IsMem);
}

DWORD ReadRegSized(PCPU_STATE Cpu, int Reg, int Size)
{
    Reg &= 7;
    if (Size == 1) {
        if (Reg < 4) return ReadReg(Cpu, Reg) & 0xFF;
        return (ReadReg(Cpu, Reg - 4) >> 8) & 0xFF;   /* AH, CH, DH, BH */
    }
    DWORD v = ReadReg(Cpu, Reg);
    return Size == 2 ? (v & 0xFFFF) : v;
}

void WriteRegSized(PCPU_STATE Cpu, int Reg, int Size, DWORD Value)
{
    Reg &= 7;
    if (Size == 1) {
        if (Reg < 4) {
            DWORD v = ReadReg(Cpu, Reg);
            WriteReg(Cpu, Reg, (v & 0xFFFFFF00u) | (Value & 0xFF));
        } else {
            DWORD v = ReadReg(Cpu, Reg - 4);
            WriteReg(Cpu, Reg - 4, (v & 0xFFFF00FFu) | ((Value & 0xFF) << 8));
        }
        return;
    }
    if (Size == 2) {
        DWORD v = ReadReg(Cpu, Reg);
        WriteReg(Cpu, Reg, (v & 0xFFFF0000u) | (Value & 0xFFFF));
        return;
    }
    WriteReg(Cpu, Reg, Value);
}

DWORD LoadOperand(x86opcode *Op, const x86operand *O, int Size)
{
    if (!O->IsMem) return ReadRegSized(Op->Cpu, O->Reg, Size);
    switch (Size) {
        case 1:  return ReadMem8(O->Addr);
        case 2:  return ReadMem16(O->Addr);
        default: return ReadMem32(O->Addr);
    }
}

void StoreOperand(x86opcode *Op, const x86operand *O, int Size, DWORD Value)
{
    if (!O->IsMem) { WriteRegSized(Op->Cpu, O->Reg, Size, Value); return; }
    switch (Size) {
        case 1:  WriteMem8(O->Addr, (BYTE)Value);  break;
        case 2:  WriteMem16(O->Addr, (WORD)Value); break;
        default: WriteMem32(O->Addr, Value);       break;
    }
}

void PushStack(x86opcode *Op, int Size, DWORD Value)
{
    DWORD esp = Op->Cpu->esp - Size;
    Op->Cpu->esp = esp;
    switch (Size) {
        case 1:  WriteMem8(esp, (BYTE)Value);  break;
        case 2:  WriteMem16(esp, (WORD)Value); break;
        default: WriteMem32(esp, Value);       break;
    }
}

DWORD PopStack(x86opcode *Op, int Size)
{
    DWORD esp = Op->Cpu->esp;
    DWORD v;
    switch (Size) {
        case 1:  v = ReadMem8(esp);  break;
        case 2:  v = ReadMem16(esp); break;
        default: v = ReadMem32(esp); break;
    }
    Op->Cpu->esp = esp + Size;
    return v;
}

BOOL TestCondition(PCPU_STATE Cpu, int Cond)
{
    DWORD f = Cpu->eflags;
    BOOL CF = (f & EFLAGS_CF) ? TRUE : FALSE;
    BOOL PF = (f & EFLAGS_PF) ? TRUE : FALSE;
    BOOL ZF = (f & EFLAGS_ZF) ? TRUE : FALSE;
    BOOL SF = (f & EFLAGS_SF) ? TRUE : FALSE;
    BOOL OF = (f & EFLAGS_OF) ? TRUE : FALSE;

    switch (Cond & 0x0F)
    {
        case CC_O:  return OF;
        case CC_NO: return !OF;
        case CC_B:  return CF;
        case CC_AE: return !CF;
        case CC_E:  return ZF;
        case CC_NE: return !ZF;
        case CC_BE: return CF || ZF;
        case CC_A:  return !CF && !ZF;
        case CC_S:  return SF;
        case CC_NS: return !SF;
        case CC_P:  return PF;
        case CC_NP: return !PF;
        case CC_L:  return SF != OF;
        case CC_GE: return SF == OF;
        case CC_LE: return ZF || (SF != OF);
        case CC_G:  return !ZF && (SF == OF);
    }
    return FALSE;
}

DWORD AluOp(PCPU_STATE Cpu, int Op, int Size, DWORD A, DWORD B)
{
    DWORD m = SizeMask(Size);
    DWORD CarryIn = (Cpu->eflags & EFLAGS_CF) ? 1u : 0u;
    DWORD res = 0;

    switch (Op)
    {
        case ALU_ADD: res = A + B;            FlagsAdd(Cpu, Size, res, A, B); break;
        case ALU_OR:  res = (A | B) & m;      FlagsLogical(Cpu, Size, res);   break;
        case ALU_ADC: res = A + B + CarryIn;  FlagsAdc(Cpu, Size, res, A, B, CarryIn); break;
        case ALU_SBB: res = A - B - CarryIn;  FlagsSbb(Cpu, Size, res, A, B, CarryIn); break;
        case ALU_AND: res = (A & B) & m;      FlagsLogical(Cpu, Size, res);   break;
        case ALU_SUB: res = A - B;            FlagsSub(Cpu, Size, res, A, B); break;
        case ALU_XOR: res = (A ^ B) & m;      FlagsLogical(Cpu, Size, res);   break;
        case ALU_CMP: res = A - B;            FlagsSub(Cpu, Size, res, A, B); break;
    }
    return res & m;
}

bool FASTCALL DoJcc8(x86opcode *Op, int Cond)
{
    signed char rel = (signed char)FetchImm8(Op);
    if (TestCondition(Op->Cpu, Cond))
        Op->CurrentPtr += rel;
    return false;
}

bool FASTCALL DoJcc32(x86opcode *Op, int Cond)
{
    int size = OpSize32(Op);
    DWORD rel = FetchRel(Op, size);
    if (TestCondition(Op->Cpu, Cond))
        Op->CurrentPtr = DwordToPtr(PtrToDword(Op->CurrentPtr) + rel);
    return false;
}

bool FASTCALL DoSetcc(x86opcode *Op, int Cond)
{
    x86operand Dst;
    FetchModRM(Op);
    GetRmOperand(Op, &Dst);
    StoreOperand(Op, &Dst, 1, TestCondition(Op->Cpu, Cond) ? 1u : 0u);
    return false;
}

bool FASTCALL DoCmovcc(x86opcode *Op, int Cond)
{
    x86operand Src;
    int size;
    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    size = OpSize32(Op);
    if (TestCondition(Op->Cpu, Cond))
        WriteRegSized(Op->Cpu, Op->Reg, size, LoadOperand(Op, &Src, size));
    return false;
}

bool FASTCALL MovExtend(x86opcode *Op, int SrcSize, BOOL SignExtend)
{
    x86operand Src;
    int size;
    DWORD v;

    FetchModRM(Op);
    GetRmOperand(Op, &Src);
    size = OpSize32(Op);
    v = LoadOperand(Op, &Src, SrcSize);
    if (SignExtend)
    {
        if (SrcSize == 1) v = (DWORD)(int)(signed char)v;
        else v = (DWORD)(int)(signed short)v;
        if (size == 2) v &= 0xFFFF;
    }
    else
    {
        v &= SizeMask(SrcSize);
    }
    WriteRegSized(Op->Cpu, Op->Reg, size, v);
    return false;
}
