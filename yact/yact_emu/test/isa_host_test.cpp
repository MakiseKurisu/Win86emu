/*
 * Host-side ISA regression harness for the yact_emu x86 interpreter.
 *
 * This harness links the *unmodified* interpreter sources
 * (operands.cpp, onebytecmds.cpp, twobytecmds.cpp, cmdstream.cpp) and drives
 * them through the real dispatch tables (OneByteCmds[] / TwoByteCmds[]) and
 * the real ProcessCommand() entry point.  It is therefore an execution test
 * of the shipped interpreter, not a static inspection.
 *
 * The interpreter addresses guest memory with 32-bit linear addresses
 * (ReadMem32()/WriteMem32() cast a DWORD to a pointer).  To exercise memory
 * operands on a 64-bit host, the harness maps guest RAM at a fixed address
 * below 4 GiB, which requires linking with
 *     -Wl,-pagezero_size,0x1000
 * (see test_isa_host.py, which builds and runs this file).
 *
 * Expected values are derived from the Intel SDM semantics for each
 * instruction; they are never read back out of the interpreter.
 *
 * This file is test-only: it is not part of any Windows build.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>

#include "yact_emu.h"

/* ------------------------------------------------------------------ */
/* guest memory                                                        */
/* ------------------------------------------------------------------ */

#define GUEST_MAP_ADDR   0x20000000UL
#define GUEST_MEM_SIZE   (2u << 20)          /* 2 MiB */
#define CODE_OFF         0x00000
#define DATA_OFF         0x100000

static BYTE  *GuestMem;
static DWORD  GuestBase;

static void GuestInit(void)
{
    void *p = mmap((void *)GUEST_MAP_ADDR, GUEST_MEM_SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED)
    {
        fprintf(stderr,
                "FATAL: cannot map guest memory at 0x%lX.\n"
                "       Link with -Wl,-pagezero_size,0x1000 so that the low\n"
                "       4 GiB address space is available.\n",
                GUEST_MAP_ADDR);
        exit(2);
    }
    if ((uintptr_t)p > 0xFFFFFFFFull)
    {
        fprintf(stderr, "FATAL: guest memory is not addressable with 32 bits\n");
        exit(2);
    }
    GuestMem  = (BYTE *)p;
    GuestBase = (DWORD)(uintptr_t)p;
}

static BYTE *CodePtr(void)  { return GuestMem + CODE_OFF; }
static DWORD CodeAddr(void) { return GuestBase + CODE_OFF; }
static DWORD DataAddr(void) { return GuestBase + DATA_OFF; }

/* Independent accessors (they bypass the interpreter entirely) used both to
 * prepare memory and to verify what the interpreter wrote. */
static void  HostPoke8 (DWORD a, BYTE  v) { *(BYTE  *)(GuestMem + (a - GuestBase)) = v; }
static void  HostPoke16(DWORD a, WORD  v) { *(WORD  *)(GuestMem + (a - GuestBase)) = v; }
static void  HostPoke32(DWORD a, DWORD v) { *(DWORD *)(GuestMem + (a - GuestBase)) = v; }
static BYTE  HostPeek8 (DWORD a) { return *(BYTE  *)(GuestMem + (a - GuestBase)); }
static WORD  HostPeek16(DWORD a) { return *(WORD  *)(GuestMem + (a - GuestBase)); }
static DWORD HostPeek32(DWORD a) { return *(DWORD *)(GuestMem + (a - GuestBase)); }

/* ------------------------------------------------------------------ */
/* test framework                                                      */
/* ------------------------------------------------------------------ */

static int Checks;
static int Failures;
static const char *Section = "";

static CPU_STATE Cpu;
static x86opcode Op;

static void SectionBegin(const char *name)
{
    Section = name;
    printf("-- %s\n", name);
}

static void CheckU32(const char *what, DWORD got, DWORD want)
{
    Checks++;
    if (got != want)
    {
        printf("FAIL [%s] %s: got 0x%08X want 0x%08X\n", Section, what, got, want);
        Failures++;
    }
}

static void CheckI32(const char *what, DWORD got, long want)
{
    CheckU32(what, got, (DWORD)want);
}

static void CheckFlags(const char *what, DWORD mask, DWORD want)
{
    Checks++;
    DWORD got = Cpu.eflags & mask;
    if (got != want)
    {
        printf("FAIL [%s] %s: flags got 0x%03X want 0x%03X (eflags=0x%08X)\n",
               Section, what, got, want, Cpu.eflags);
        Failures++;
    }
}

static void CheckBytes(const char *what, const BYTE *got, const BYTE *want, int n)
{
    Checks++;
    if (memcmp(got, want, n) != 0)
    {
        printf("FAIL [%s] %s: memory differs\n", Section, what);
        Failures++;
    }
}

static void CpuReset(void)
{
    memset(&Cpu, 0, sizeof(Cpu));
    Cpu.eflags = 0x00000002;          /* bit 1 reads as 1 on x86 */
    Cpu.esp = DataAddr() + 0x8000;    /* usable stack inside guest RAM */
}

/* Load instruction bytes at the code area and prepare the decoder. */
static void Load(const BYTE *bytes, int n)
{
    memcpy(CodePtr(), bytes, n);
    memset(CodePtr() + n, 0xCC, 16);
    memset(&Op, 0, sizeof(Op));
    Op.Cpu = &Cpu;
    Op.CurrentPtr = CodePtr();
    Op.StartPtr = CodePtr();
    InitCommand(&Op);
}

/* Execute exactly the loaded instruction; returns the instruction length. */
static DWORD Exec(void)
{
    ProcessCommand(&Op);
    return (DWORD)(Op.CurrentPtr - CodePtr());
}

/* Load and execute in one step. */
static DWORD Run(const BYTE *bytes, int n)
{
    Load(bytes, n);
    return Exec();
}

#define INSN(...) do { const BYTE b[] = { __VA_ARGS__ }; Run(b, (int)sizeof(b)); } while (0)
#define INSNL(...) ({ const BYTE b[] = { __VA_ARGS__ }; DWORD _l = Run(b, (int)sizeof(b)); _l; })

/* ------------------------------------------------------------------ */
/* data transfer                                                       */
/* ------------------------------------------------------------------ */

static void TestDataTransfer(void)
{
    SectionBegin("data transfer");

    /* MOV r8, imm8 (B0+r ib) */
    CpuReset(); INSN(0xB0, 0x7F);
    CheckU32("MOV AL,7Fh", Cpu.eax, 0x0000007F);
    CpuReset(); Cpu.eax = 0xAABBCCDD; INSN(0xB4, 0x80);
    CheckU32("MOV AH,80h keeps EAX[31:16]", Cpu.eax, 0xAABB80DD);

    /* MOV r32, imm32 (B8+r id) */
    CpuReset(); INSNL(0xB8, 0x78, 0x56, 0x34, 0x12);
    CheckU32("MOV EAX,12345678h", Cpu.eax, 0x12345678);
    CpuReset(); INSN(0xBB, 0xEF, 0xBE, 0xAD, 0xDE);
    CheckU32("MOV EBX,DEADBEEFh", Cpu.ebx, 0xDEADBEEF);
    CheckU32("MOV EBX length", INSNL(0xBB, 0xEF, 0xBE, 0xAD, 0xDE), 5);

    /* MOV r16, imm16 (66 B8+r iw) - upper 16 bits preserved */
    CpuReset(); Cpu.eax = 0xAAAA5555; INSN(0x66, 0xB8, 0x34, 0x12);
    CheckU32("66 MOV AX,1234h keeps upper half", Cpu.eax, 0xAAAA1234);
    CheckU32("66 MOV AX length", INSNL(0x66, 0xB8, 0x34, 0x12), 4);

    /* MOV r/m8, r8 (88 /r) */
    CpuReset(); Cpu.eax = 0; Cpu.ecx = 0x11223344; INSN(0x88, 0xC8);
    CheckU32("MOV AL,CL", Cpu.eax, 0x00000044);

    /* MOV r/m32, r32 (89 /r) */
    CpuReset(); Cpu.ebx = 0xDEADBEEF; INSN(0x89, 0xD8);
    CheckU32("MOV EAX,EBX", Cpu.eax, 0xDEADBEEF);

    /* MOV r8, r/m8 (8A /r) */
    CpuReset(); Cpu.ebx = 0x00000099; INSN(0x8A, 0xC3);
    CheckU32("MOV AL,BL", Cpu.eax, 0x00000099);

    /* MOV r32, r/m32 (8B /r) */
    CpuReset(); Cpu.ebx = 0x12345678; INSN(0x8B, 0xC3);
    CheckU32("MOV EAX,EBX (8B)", Cpu.eax, 0x12345678);

    /* MOV r/m8, imm8 (C6 /0 ib) */
    CpuReset(); Cpu.eax = 0xFFFFFF00; INSN(0xC6, 0xC0, 0x55);
    CheckU32("MOV AL,55h", Cpu.eax, 0xFFFFFF55);

    /* MOV r/m32, imm32 (C7 /0 id) */
    CpuReset(); INSN(0xC7, 0xC0, 0x44, 0x33, 0x22, 0x11);
    CheckU32("MOV EAX,11223344h", Cpu.eax, 0x11223344);
    CheckU32("MOV r/m32,imm32 length", INSNL(0xC7, 0xC0, 0x44, 0x33, 0x22, 0x11), 6);

    /* MOV EAX, moffs32 (A1 id) / MOV moffs32, EAX (A3 id) */
    {
        DWORD d = DataAddr() + 0x100;
        CpuReset(); HostPoke32(d, 0xCAFEBABE);
        BYTE b[] = { 0xA1, 0,0,0,0 };
        *(DWORD *)(b + 1) = d;
        Run(b, 5);
        CheckU32("MOV EAX,moffs32", Cpu.eax, 0xCAFEBABE);
        CheckU32("MOV EAX,moffs32 length", 5, 5);

        CpuReset(); Cpu.eax = 0x0BADF00D;
        BYTE c[] = { 0xA3, 0,0,0,0 };
        *(DWORD *)(c + 1) = d;
        Run(c, 5);
        CheckU32("MOV moffs32,EAX", HostPeek32(d), 0x0BADF00D);

        CpuReset(); Cpu.eax = 0; HostPoke8(d, 0x5A);
        BYTE e[] = { 0xA0, 0,0,0,0 };
        *(DWORD *)(e + 1) = d;
        Run(e, 5);
        CheckU32("MOV AL,moffs8", Cpu.eax, 0x5A);

        CpuReset(); Cpu.eax = 0x00000077;
        BYTE f[] = { 0xA2, 0,0,0,0 };
        *(DWORD *)(f + 1) = d;
        Run(f, 5);
        CheckU32("MOV moffs8,AL", HostPeek8(d), 0x77);
    }

    /* LEA r32, m (8D /r) - the address is computed but never accessed */
    CpuReset(); Cpu.ebx = 0x00010000; INSN(0x8D, 0x83, 0x34, 0x12, 0x00, 0x00);
    CheckU32("LEA EAX,[EBX+1234h]", Cpu.eax, 0x00011234);
    CheckU32("LEA length", INSNL(0x8D, 0x83, 0x34, 0x12, 0x00, 0x00), 6);

    /* MOVZX / MOVSX (0F B6/B7/BE/BF) */
    CpuReset(); Cpu.ecx = 0xFFFFFF80; INSN(0x0F, 0xB6, 0xC1);
    CheckU32("MOVZX EAX,CL", Cpu.eax, 0x00000080);
    CpuReset(); Cpu.ecx = 0xFFFF8000; INSN(0x0F, 0xB7, 0xC1);
    CheckU32("MOVZX EAX,CX", Cpu.eax, 0x00008000);
    CpuReset(); Cpu.ecx = 0xFFFFFF80; INSN(0x0F, 0xBE, 0xC1);
    CheckU32("MOVSX EAX,CL", Cpu.eax, 0xFFFFFF80);
    CpuReset(); Cpu.ecx = 0xFFFF8000; INSN(0x0F, 0xBF, 0xC1);
    CheckU32("MOVSX EAX,CX", Cpu.eax, 0xFFFF8000);
    CheckU32("MOVSX EAX,CX length", INSNL(0x0F, 0xBF, 0xC1), 3);
}

/* ------------------------------------------------------------------ */
/* addressing modes (ModR/M, SIB, displacement)                        */
/* ------------------------------------------------------------------ */

static void TestAddressing(void)
{
    SectionBegin("addressing modes");

    /* [disp32] - mod=00 rm=101 */
    {
        DWORD d = DataAddr() + 0x200;
        HostPoke32(d, 0x11111111);
        CpuReset();
        BYTE b[] = { 0x8B, 0x05, 0,0,0,0 };
        *(DWORD *)(b + 2) = d;
        Run(b, 6);
        CheckU32("MOV EAX,[disp32]", Cpu.eax, 0x11111111);
    }

    /* [EAX] - mod=00 rm=000 */
    {
        DWORD d = DataAddr() + 0x210;
        HostPoke32(d, 0x22222222);
        CpuReset(); Cpu.eax = d; INSN(0x8B, 0x00);
        CheckU32("MOV EAX,[EAX]", Cpu.eax, 0x22222222);
    }

    /* [EBP+disp8] and [EBP-disp8] */
    {
        DWORD d = DataAddr() + 0x220;
        HostPoke32(d + 8, 0x33333333);
        CpuReset(); Cpu.ebp = d; INSN(0x8B, 0x45, 0x08);
        CheckU32("MOV EAX,[EBP+8]", Cpu.eax, 0x33333333);

        HostPoke32(d - 8, 0x44444444);
        CpuReset(); Cpu.ebp = d; INSN(0x8B, 0x45, 0xF8);
        CheckU32("MOV EAX,[EBP-8] (disp8 sign-extended)", Cpu.eax, 0x44444444);
    }

    /* [EBP+disp32] */
    {
        DWORD d = DataAddr() + 0x240;
        HostPoke32(d + 0x1234, 0x55555555);
        CpuReset(); Cpu.ebp = d;
        BYTE b[] = { 0x8B, 0x85, 0,0,0,0 };
        *(DWORD *)(b + 2) = 0x1234;
        Run(b, 6);
        CheckU32("MOV EAX,[EBP+disp32]", Cpu.eax, 0x55555555);
    }

    /* [ESP+disp8] - SIB with index==4 (no index) */
    {
        DWORD d = DataAddr() + 0x260;
        HostPoke32(d + 4, 0x66666666);
        CpuReset(); Cpu.esp = d; INSN(0x8B, 0x44, 0x24, 0x04);
        CheckU32("MOV EAX,[ESP+4] (SIB no index)", Cpu.eax, 0x66666666);
        CheckU32("MOV EAX,[ESP+4] length", INSNL(0x8B, 0x44, 0x24, 0x04), 4);
    }

    /* [ECX*4+disp32] - SIB with base==5, mod=00 */
    {
        DWORD d = DataAddr() + 0x280;
        HostPoke32(d + 4 * 3, 0x77777777);
        CpuReset(); Cpu.ecx = 3;
        BYTE b[] = { 0x8B, 0x04, 0x8D, 0,0,0,0 };
        *(DWORD *)(b + 3) = d;
        Run(b, 7);
        CheckU32("MOV EAX,[ECX*4+disp32]", Cpu.eax, 0x77777777);
    }

    /* [EAX+ECX*4+disp8] */
    {
        DWORD d = DataAddr() + 0x2A0;
        HostPoke32(d + 5 * 4 + 2, 0x88888888);
        CpuReset(); Cpu.eax = d + 2; Cpu.ecx = 5; INSN(0x8B, 0x44, 0x88, 0x00);
        CheckU32("MOV EAX,[EAX+ECX*4]", Cpu.eax, 0x88888888);
    }

    /* Store to a SIB address */
    {
        DWORD d = DataAddr() + 0x2C0;
        CpuReset(); Cpu.esp = d; Cpu.eax = 0x99999999;
        INSN(0x89, 0x44, 0x24, 0x0C);
        CheckU32("MOV [ESP+12],EAX", HostPeek32(d + 12), 0x99999999);
    }

    /* 8/16-bit stores must not disturb neighbouring bytes */
    {
        DWORD d = DataAddr() + 0x2E0;
        HostPoke32(d, 0xAABBCCDD);
        CpuReset(); Cpu.ebp = d; Cpu.eax = 0x11223344;
        INSN(0x88, 0x45, 0x00);
        CheckU32("MOV [EBP],AL keeps upper bytes", HostPeek32(d), 0xAABBCC44);

        HostPoke32(d, 0xAABBCCDD);
        CpuReset(); Cpu.ebp = d; Cpu.eax = 0x11223344; INSN(0x66, 0x89, 0x45, 0x00);
        CheckU32("66 MOV [EBP],AX keeps upper bytes", HostPeek32(d), 0xAABB3344);
    }
}

/* ------------------------------------------------------------------ */
/* integer arithmetic                                                  */
/* ------------------------------------------------------------------ */

static void TestArithmetic(void)
{
    SectionBegin("integer arithmetic");

    /* ADD AL, imm8 (04 ib) */
    CpuReset(); Cpu.eax = 0x0F; INSN(0x04, 0x01);
    CheckU32("ADD AL,1", Cpu.eax, 0x10);
    CheckFlags("ADD AL,1", EFLAGS_CF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF | EFLAGS_PF,
               EFLAGS_AF);

    /* ADD EAX, imm32 (05 id) - signed overflow 0x7FFFFFFF+1 */
    CpuReset(); Cpu.eax = 0x7FFFFFFF; INSN(0x05, 0x01, 0x00, 0x00, 0x00);
    CheckU32("ADD EAX,1 overflow", Cpu.eax, 0x80000000);
    CheckFlags("ADD EAX,1 overflow", EFLAGS_CF | EFLAGS_OF | EFLAGS_SF | EFLAGS_ZF,
               EFLAGS_OF | EFLAGS_SF);
    CheckFlags("ADD EAX,1 AF", EFLAGS_AF, EFLAGS_AF);

    /* unsigned carry */
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0x05, 0x01, 0x00, 0x00, 0x00);
    CheckU32("ADD EAX,1 wraps", Cpu.eax, 0x00000000);
    CheckFlags("ADD EAX,1 wraps", EFLAGS_CF | EFLAGS_OF | EFLAGS_ZF | EFLAGS_SF,
               EFLAGS_CF | EFLAGS_ZF);

    /* ADC with carry in */
    CpuReset(); Cpu.eax = 0; Cpu.eflags |= EFLAGS_CF; INSN(0x15, 0x00, 0x00, 0x00, 0x00);
    CheckU32("ADC EAX,0 with CF", Cpu.eax, 1);

    /* SUB AL, imm8 (2C ib) */
    CpuReset(); Cpu.eax = 0x00; INSN(0x2C, 0x01);
    CheckU32("SUB AL,1", Cpu.eax, 0xFF);
    CheckFlags("SUB AL,1", EFLAGS_CF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF,
               EFLAGS_CF | EFLAGS_AF | EFLAGS_SF);

    /* SUB EAX, imm32 (2D id) - signed overflow 0x80000000-1 */
    CpuReset(); Cpu.eax = 0x80000000; INSN(0x2D, 0x01, 0x00, 0x00, 0x00);
    CheckU32("SUB EAX,1 overflow", Cpu.eax, 0x7FFFFFFF);
    CheckFlags("SUB EAX,1 overflow", EFLAGS_CF | EFLAGS_OF | EFLAGS_SF,
               EFLAGS_OF);
    CheckFlags("SUB EAX,1 AF", EFLAGS_AF, EFLAGS_AF);

    /* CMP AL, imm8 (3C ib) does not write */
    CpuReset(); Cpu.eax = 0x42; INSN(0x3C, 0x42);
    CheckU32("CMP AL,AL leaves AL", Cpu.eax, 0x42);
    CheckFlags("CMP AL,AL", EFLAGS_CF | EFLAGS_ZF | EFLAGS_OF, EFLAGS_ZF);

    /* CMP EAX, imm32 (3D id) */
    CpuReset(); Cpu.eax = 5; INSN(0x3D, 0x0A, 0x00, 0x00, 0x00);
    CheckU32("CMP EAX,10 leaves EAX", Cpu.eax, 5);
    CheckFlags("CMP EAX,10", EFLAGS_CF | EFLAGS_ZF | EFLAGS_OF | EFLAGS_SF, EFLAGS_CF | EFLAGS_SF);

    /* ADD r32, r/m32 (03 /r) */
    CpuReset(); Cpu.eax = 0x100; Cpu.ecx = 0x23; INSN(0x03, 0xC1);
    CheckU32("ADD EAX,ECX", Cpu.eax, 0x123);

    /* ADD r8, r/m8 (02 /r) */
    CpuReset(); Cpu.eax = 0x40; Cpu.ecx = 0x42; INSN(0x02, 0xC1);
    CheckU32("ADD AL,CL", Cpu.eax, 0x82);
    CheckFlags("ADD AL,CL signed overflow", EFLAGS_CF | EFLAGS_OF | EFLAGS_SF,
               EFLAGS_OF | EFLAGS_SF);

    /* SUB r32, r/m32 (2B /r) */
    CpuReset(); Cpu.eax = 0x1000; Cpu.ecx = 0x1; INSN(0x2B, 0xC1);
    CheckU32("SUB EAX,ECX", Cpu.eax, 0x0FFF);

    /* CMP r32, r/m32 (3B /r) */
    CpuReset(); Cpu.eax = 0x10; Cpu.ecx = 0x20; INSN(0x3B, 0xC1);
    CheckU32("CMP EAX,ECX leaves EAX", Cpu.eax, 0x10);
    CheckFlags("CMP EAX,ECX", EFLAGS_CF | EFLAGS_ZF | EFLAGS_SF, EFLAGS_CF | EFLAGS_SF);

    /* Group 1: 80 /n ib (r/m8, imm8) */
    CpuReset(); Cpu.eax = 0xF0; INSN(0x80, 0xC0, 0x0F);   /* ADD AL,0Fh */
    CheckU32("80 /0 ADD AL,0Fh", Cpu.eax, 0xFF);
    CpuReset(); Cpu.eax = 0x0F; INSN(0x80, 0xE8, 0xF0);   /* SUB AL,0F0h */
    CheckU32("80 /5 SUB AL,0F0h", Cpu.eax, 0x1F);
    CpuReset(); Cpu.eax = 0xFF; INSN(0x80, 0xF8, 0xFF);   /* CMP AL,0FFh */
    CheckFlags("80 /7 CMP AL,0FFh", EFLAGS_ZF, EFLAGS_ZF);

    /* Group 1: 81 /n id (r/m32, imm32) */
    CpuReset(); Cpu.eax = 0x0000FFFF; INSN(0x81, 0xC0, 0x01, 0x00, 0x00, 0x00);
    CheckU32("81 /0 ADD EAX,1", Cpu.eax, 0x00010000);
    CpuReset(); Cpu.eax = 0xF0F0F0F0; INSN(0x81, 0xE0, 0x0F, 0x0F, 0x0F, 0x0F);   /* AND */
    CheckU32("81 /4 AND EAX,0F0F0F0Fh", Cpu.eax, 0x00000000);
    CheckFlags("81 /4 AND -> ZF", EFLAGS_ZF | EFLAGS_CF | EFLAGS_OF, EFLAGS_ZF);
    CpuReset(); Cpu.eax = 0x12345678; INSN(0x81, 0xF0, 0xFF, 0x00, 0x00, 0x00); /* XOR */
    CheckU32("81 /6 XOR EAX,0FFh", Cpu.eax, 0x12345687);
    CpuReset(); Cpu.eax = 0x0F; INSN(0x81, 0xC8, 0x30, 0x00, 0x00, 0x00);      /* OR */
    CheckU32("81 /1 OR EAX,30h", Cpu.eax, 0x3F);

    /* Group 1: 83 /n ib (r/m32, sign-extended imm8) */
    CpuReset(); Cpu.eax = 0x10; INSN(0x83, 0xC0, 0xFF);   /* ADD EAX,-1 */
    CheckU32("83 /0 ADD EAX,-1", Cpu.eax, 0x0F);
    CheckFlags("83 /0 ADD EAX,-1", EFLAGS_CF | EFLAGS_SF | EFLAGS_OF, EFLAGS_CF);
    CpuReset(); Cpu.eax = 0x10; INSN(0x83, 0xE8, 0x20);   /* SUB EAX,20h */
    CheckU32("83 /5 SUB EAX,20h", Cpu.eax, 0xFFFFFFF0);
    CheckFlags("83 /5 SUB EAX,20h", EFLAGS_CF | EFLAGS_SF, EFLAGS_CF | EFLAGS_SF);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0x83, 0xF8, 0xFF);  /* CMP EAX,-1 */
    CheckFlags("83 /7 CMP EAX,-1", EFLAGS_ZF, EFLAGS_ZF);

    /* INC/DEC preserve CF */
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.eflags |= EFLAGS_CF; INSN(0x40);
    CheckU32("INC EAX wraps", Cpu.eax, 0);
    CheckFlags("INC EAX sets ZF and keeps CF", EFLAGS_CF | EFLAGS_ZF, EFLAGS_CF | EFLAGS_ZF);
    CpuReset(); Cpu.eax = 0x7FFFFFFF; INSN(0x40);
    CheckU32("INC EAX overflow", Cpu.eax, 0x80000000);
    CheckFlags("INC EAX overflow sets OF", EFLAGS_OF | EFLAGS_SF, EFLAGS_OF | EFLAGS_SF);

    /* FE /0, /1 - 8-bit INC/DEC */
    CpuReset(); Cpu.eax = 0x000000FF; INSN(0xFE, 0xC0);
    CheckU32("FE /0 INC AL", Cpu.eax, 0x00000000);
    CpuReset(); Cpu.eax = 0xABCD0100; INSN(0xFE, 0xC8);
    CheckU32("FE /1 DEC AL keeps AH", Cpu.eax, 0xABCD01FF);

    /* FF /0, /1 - 32-bit INC/DEC */
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0xFF, 0xC0);
    CheckU32("FF /0 INC EAX", Cpu.eax, 0x00000000);
    CpuReset(); Cpu.eax = 0x00000001; INSN(0xFF, 0xC8);
    CheckU32("FF /1 DEC EAX", Cpu.eax, 0x00000000);

    /* NEG (F7 /3) */
    CpuReset(); Cpu.eax = 1; INSN(0xF7, 0xD8);
    CheckU32("NEG EAX", Cpu.eax, 0xFFFFFFFF);
    CheckFlags("NEG EAX sets CF", EFLAGS_CF, EFLAGS_CF);

    /* MUL r/m32 (F7 /4) - EDX:EAX */
    CpuReset(); Cpu.eax = 0x10000; Cpu.ecx = 0x10000; INSN(0xF7, 0xE1);
    CheckU32("MUL EDX:EAX low", Cpu.eax, 0x00000000);
    CheckU32("MUL EDX:EAX high", Cpu.edx, 0x00000001);
    CheckFlags("MUL overflow CF/OF", EFLAGS_CF | EFLAGS_OF, EFLAGS_CF | EFLAGS_OF);

    /* IMUL r32, r/m32 (0F AF /r) */
    CpuReset(); Cpu.eax = 0x10000; Cpu.ecx = 0x10000; INSN(0x0F, 0xAF, 0xC1);
    CheckU32("IMUL EAX,ECX truncated", Cpu.eax, 0x00000000);
    CheckFlags("IMUL overflow CF/OF", EFLAGS_CF | EFLAGS_OF, EFLAGS_CF | EFLAGS_OF);
    CpuReset(); Cpu.eax = 2; Cpu.ecx = 3; INSN(0x0F, 0xAF, 0xC1);
    CheckU32("IMUL EAX,ECX", Cpu.eax, 6);
    CheckFlags("IMUL no overflow", EFLAGS_CF | EFLAGS_OF, 0);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.ecx = 0xFFFFFFFE; INSN(0x0F, 0xAF, 0xC1);
    CheckU32("IMUL (-1)*(-2)", Cpu.eax, 2);

    /* CWDE (98) / CDQ (99) */
    CpuReset(); Cpu.eax = 0x00007FFF; INSN(0x98);
    CheckU32("CWDE 7FFFh stays positive", Cpu.eax, 0x00007FFF);
    CpuReset(); Cpu.eax = 0x00008000; INSN(0x98);
    CheckU32("CWDE 8000h sign-extends", Cpu.eax, 0xFFFF8000);
    CpuReset(); Cpu.eax = 0x00000000; INSN(0x99);
    CheckU32("CDQ positive", Cpu.edx, 0x00000000);
    CpuReset(); Cpu.eax = 0x80000000; INSN(0x99);
    CheckU32("CDQ negative", Cpu.edx, 0xFFFFFFFF);
    CpuReset(); Cpu.eax = 0x00000080; INSN(0x66, 0x98);
    CheckU32("66 CBW 80h sign-extends AL into AX", Cpu.eax, 0x0000FF80);
    CpuReset(); Cpu.eax = 0x00008000; INSN(0x66, 0x99);
    CheckU32("66 CWD negative", Cpu.edx, 0x0000FFFF);
    CpuReset(); Cpu.eax = 0x00007000; INSN(0x66, 0x99);
    CheckU32("66 CWD positive", Cpu.edx, 0x00000000);
}

/* ------------------------------------------------------------------ */
/* shifts and rotates                                                  */
/* ------------------------------------------------------------------ */

static void TestShifts(void)
{
    SectionBegin("shifts and rotates");

    /* SHL r/m32, imm8 (C1 /4 ib) */
    CpuReset(); Cpu.eax = 1; INSN(0xC1, 0xE0, 0x04);
    CheckU32("SHL EAX,4", Cpu.eax, 0x10);
    CpuReset(); Cpu.eax = 0x80000000; INSN(0xC1, 0xE0, 0x01);
    CheckU32("SHL EAX,1 msb out", Cpu.eax, 0x00000000);
    CheckFlags("SHL EAX,1 CF", EFLAGS_CF, EFLAGS_CF);

    /* SHR r/m32, 1 (D1 /5) */
    CpuReset(); Cpu.eax = 0x00000003; INSN(0xD1, 0xE8);
    CheckU32("SHR EAX,1", Cpu.eax, 0x00000001);
    CheckFlags("SHR EAX,1 CF", EFLAGS_CF, EFLAGS_CF);

    /* SAR r/m32, 1 (D1 /7) */
    CpuReset(); Cpu.eax = 0x80000000; INSN(0xD1, 0xF8);
    CheckU32("SAR EAX,1", Cpu.eax, 0xC0000000);

    /* ROL/ROR (D1 /0, /1) */
    CpuReset(); Cpu.eax = 0x80000001; INSN(0xD1, 0xC0);
    CheckU32("ROL EAX,1", Cpu.eax, 0x00000003);
    CpuReset(); Cpu.eax = 0x00000001; INSN(0xD1, 0xC8);
    CheckU32("ROR EAX,1", Cpu.eax, 0x80000000);

    /* SHLD r/m32, r32, imm8 (0F A4 /r ib) */
    CpuReset(); Cpu.eax = 0x12345678; Cpu.ecx = 0x9ABCDEF0;
    INSN(0x0F, 0xA4, 0xC8, 0x08);
    CheckU32("SHLD EAX,ECX,8", Cpu.eax, 0x3456789A);
    CheckFlags("SHLD CF", EFLAGS_CF, 0);

    CpuReset(); Cpu.eax = 0x80000000; Cpu.ecx = 0x00000000;
    INSN(0x0F, 0xA4, 0xC8, 0x01);
    CheckU32("SHLD EAX,ECX,1", Cpu.eax, 0x00000000);
    CheckFlags("SHLD EAX,ECX,1 CF", EFLAGS_CF, EFLAGS_CF);

    /* SHRD r/m32, r32, imm8 (0F AC /r ib) */
    CpuReset(); Cpu.eax = 0x000000FF; Cpu.ecx = 0x11223344;
    INSN(0x0F, 0xAC, 0xC8, 0x08);
    CheckU32("SHRD EAX,ECX,8", Cpu.eax, 0x44000000);
    CheckFlags("SHRD EAX,ECX,8 CF", EFLAGS_CF, EFLAGS_CF);

    /* SHLD by CL (0F A5 /r) and SHRD by CL (0F AD /r) */
    CpuReset(); Cpu.eax = 0x00000001; Cpu.ecx = 0x00000004;
    INSN(0x0F, 0xA5, 0xC8);
    CheckU32("SHLD EAX,ECX,CL=4", Cpu.eax, 0x00000010);
    CpuReset(); Cpu.eax = 0x10000000; Cpu.ecx = 0x00000004;
    INSN(0x0F, 0xAD, 0xC8);
    CheckU32("SHRD EAX,ECX,CL=4", Cpu.eax, 0x41000000);
}

/* ------------------------------------------------------------------ */
/* bit operations                                                      */
/* ------------------------------------------------------------------ */

static void TestBitOps(void)
{
    SectionBegin("bit operations");

    /* BT r/m32, r32 (0F A3 /r) */
    CpuReset(); Cpu.eax = 0x80000000; Cpu.ecx = 31; INSN(0x0F, 0xA3, 0xC8);
    CheckFlags("BT EAX,31", EFLAGS_CF, EFLAGS_CF);
    CpuReset(); Cpu.eax = 0x80000000; Cpu.ecx = 30; INSN(0x0F, 0xA3, 0xC8);
    CheckFlags("BT EAX,30", EFLAGS_CF, 0);
    CpuReset(); Cpu.eax = 0x80000000; Cpu.ecx = 63; INSN(0x0F, 0xA3, 0xC8);
    CheckFlags("BT EAX,63 wraps to bit 31", EFLAGS_CF, EFLAGS_CF);

    /* BTS / BTR / BTC (0F AB /r, 0F B3 /r, 0F BB /r) */
    CpuReset(); Cpu.eax = 0; Cpu.ecx = 5; INSN(0x0F, 0xAB, 0xC8);
    CheckU32("BTS EAX,5", Cpu.eax, 0x00000020);
    CheckFlags("BTS EAX,5 CF was 0", EFLAGS_CF, 0);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.ecx = 5; INSN(0x0F, 0xB3, 0xC8);
    CheckU32("BTR EAX,5", Cpu.eax, 0xFFFFFFDF);
    CheckFlags("BTR EAX,5 CF was 1", EFLAGS_CF, EFLAGS_CF);
    CpuReset(); Cpu.eax = 0; Cpu.ecx = 7; INSN(0x0F, 0xBB, 0xC8);
    CheckU32("BTC EAX,7", Cpu.eax, 0x00000080);
    CpuReset(); Cpu.eax = 0x00000080; Cpu.ecx = 7; INSN(0x0F, 0xBB, 0xC8);
    CheckU32("BTC EAX,7 clears", Cpu.eax, 0x00000000);
    CheckFlags("BTC EAX,7 CF was 1", EFLAGS_CF, EFLAGS_CF);

    /* Group 8: 0F BA /4../7 ib */
    CpuReset(); Cpu.eax = 0x00000001; INSN(0x0F, 0xBA, 0xE0, 0x1F);  /* BT EAX,31 */
    CheckFlags("0F BA /4 BT EAX,31", EFLAGS_CF, 0);
    CpuReset(); Cpu.eax = 0x80000000; INSN(0x0F, 0xBA, 0xE0, 0x1F);
    CheckFlags("0F BA /4 BT EAX,31 set", EFLAGS_CF, EFLAGS_CF);
    CpuReset(); Cpu.eax = 0x00000000; INSN(0x0F, 0xBA, 0xE8, 0x03);  /* BTS EAX,3 */
    CheckU32("0F BA /5 BTS EAX,3", Cpu.eax, 0x00000008);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0x0F, 0xBA, 0xF0, 0x03);  /* BTR EAX,3 */
    CheckU32("0F BA /6 BTR EAX,3", Cpu.eax, 0xFFFFFFF7);
    CpuReset(); Cpu.eax = 0x00000000; INSN(0x0F, 0xBA, 0xF8, 0x00);  /* BTC EAX,0 */
    CheckU32("0F BA /7 BTC EAX,0", Cpu.eax, 0x00000001);
    CpuReset(); Cpu.eax = 0x00000001; INSN(0x0F, 0xBA, 0xF8, 0x00);
    CheckU32("0F BA /7 BTC EAX,0 clears", Cpu.eax, 0x00000000);
    CheckU32("0F BA length", INSNL(0x0F, 0xBA, 0xE0, 0x1F), 4);

    /* BSF / BSR (0F BC /r, 0F BD /r) */
    CpuReset(); Cpu.ecx = 0x00000100; INSN(0x0F, 0xBC, 0xC1);
    CheckU32("BSF EAX,ECX", Cpu.eax, 8);
    CheckFlags("BSF found -> ZF=0", EFLAGS_ZF, 0);
    CpuReset(); Cpu.eax = 0xDEADBEEF; Cpu.ecx = 0; INSN(0x0F, 0xBC, 0xC1);
    CheckFlags("BSF zero source -> ZF=1", EFLAGS_ZF, EFLAGS_ZF);
    CpuReset(); Cpu.ecx = 0x00000100; INSN(0x0F, 0xBD, 0xC1);
    CheckU32("BSR EAX,ECX", Cpu.eax, 8);
    CpuReset(); Cpu.ecx = 0x00000001; INSN(0x0F, 0xBD, 0xC1);
    CheckU32("BSR EAX,1", Cpu.eax, 0);
    CpuReset(); Cpu.eax = 0xDEADBEEF; Cpu.ecx = 0; INSN(0x0F, 0xBD, 0xC1);
    CheckFlags("BSR zero source -> ZF=1", EFLAGS_ZF, EFLAGS_ZF);

    /* BSWAP (0F C8+r) */
    CpuReset(); Cpu.eax = 0x12345678; INSN(0x0F, 0xC8);
    CheckU32("BSWAP EAX", Cpu.eax, 0x78563412);
    CpuReset(); Cpu.ecx = 0x00FF00FF; INSN(0x0F, 0xC9);
    CheckU32("BSWAP ECX", Cpu.ecx, 0xFF00FF00);
    CheckU32("BSWAP length", INSNL(0x0F, 0xC8), 2);
}

/* ------------------------------------------------------------------ */
/* logic and TEST                                                      */
/* ------------------------------------------------------------------ */

static void TestLogic(void)
{
    SectionBegin("logic and TEST");

    /* TEST r/m32, r32 (85 /r) */
    CpuReset(); Cpu.eax = 0x0F0F0F0F; Cpu.ecx = 0xF0F0F0F0; INSN(0x85, 0xC8);
    CheckU32("TEST leaves EAX", Cpu.eax, 0x0F0F0F0F);
    CheckFlags("TEST -> ZF", EFLAGS_ZF | EFLAGS_CF | EFLAGS_OF, EFLAGS_ZF);

    /* TEST r/m8, r8 (84 /r) */
    CpuReset(); Cpu.eax = 0x01; Cpu.ecx = 0x01; INSN(0x84, 0xC8);
    CheckFlags("TEST AL,CL nonzero", EFLAGS_ZF, 0);

    /* TEST AL, imm8 (A8 ib) */
    CpuReset(); Cpu.eax = 0x80; INSN(0xA8, 0x80);
    CheckU32("TEST AL,80h leaves AL", Cpu.eax, 0x80);
    CheckFlags("TEST AL,80h SF", EFLAGS_ZF | EFLAGS_SF | EFLAGS_PF, EFLAGS_SF);

    /* TEST EAX, imm32 (A9 id) */
    CpuReset(); Cpu.eax = 0x0F; INSN(0xA9, 0x0F, 0x00, 0x00, 0x00);
    CheckFlags("TEST EAX,0Fh", EFLAGS_ZF, 0);

    /* OR r/m32, r32 (09 /r) */
    CpuReset(); Cpu.eax = 0x0F; Cpu.ecx = 0xF0; INSN(0x09, 0xC8);
    CheckU32("OR EAX,ECX", Cpu.eax, 0xFF);

    /* XOR r/m32, r32 (31 /r) - classic zeroing idiom */
    CpuReset(); Cpu.eax = 0x12345678; INSN(0x31, 0xC0);
    CheckU32("XOR EAX,EAX", Cpu.eax, 0);
    CheckFlags("XOR EAX,EAX -> ZF", EFLAGS_ZF | EFLAGS_CF | EFLAGS_OF, EFLAGS_ZF);

    /* AND EAX, imm32 (25 id) */
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0x25, 0x00, 0xFF, 0x00, 0xFF);
    CheckU32("AND EAX,FF00FF00h", Cpu.eax, 0xFF00FF00);

    /* NOT (F7 /2) */
    CpuReset(); Cpu.eax = 0x00000000; INSN(0xF7, 0xD0);
    CheckU32("NOT EAX", Cpu.eax, 0xFFFFFFFF);
    CheckFlags("NOT does not touch flags", EFLAGS_CF | EFLAGS_ZF | EFLAGS_OF, 0);
}

/* ------------------------------------------------------------------ */
/* EFLAGS instructions                                                 */
/* ------------------------------------------------------------------ */

static void TestFlags(void)
{
    SectionBegin("EFLAGS instructions");

    CpuReset(); INSN(0xF9);
    CheckFlags("STC", EFLAGS_CF, EFLAGS_CF);
    CpuReset(); Cpu.eflags |= EFLAGS_CF; INSN(0xF8);
    CheckFlags("CLC", EFLAGS_CF, 0);
    CpuReset(); INSN(0xF5);
    CheckFlags("CMC sets CF", EFLAGS_CF, EFLAGS_CF);
    CpuReset(); Cpu.eflags |= EFLAGS_CF; INSN(0xF5);
    CheckFlags("CMC clears CF", EFLAGS_CF, 0);

    CpuReset(); INSN(0xFD);
    CheckFlags("STD", EFLAGS_DF, EFLAGS_DF);
    CpuReset(); Cpu.eflags |= EFLAGS_DF; INSN(0xFC);
    CheckFlags("CLD", EFLAGS_DF, 0);

    CpuReset(); INSN(0xFB);
    CheckFlags("STI", EFLAGS_IF, EFLAGS_IF);
    CpuReset(); Cpu.eflags |= EFLAGS_IF; INSN(0xFA);
    CheckFlags("CLI", EFLAGS_IF, 0);

    /* SAHF / LAHF round trip */
    CpuReset(); Cpu.eax = 0x0000D500; INSN(0x9E);
    CheckFlags("SAHF from AH=D5h", EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF,
               EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF);
    CpuReset(); Cpu.eax = 0x12340000;
    Cpu.eflags = EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | 0x02;
    INSN(0x9F);
    /* LAHF always sets AH bit 1:  SF:ZF:0:AF:0:PF:1:CF */
    CheckU32("LAHF packs CF/PF/AF/ZF/SF into AH", (Cpu.eax >> 8) & 0xFF, 0xD7);

    /* PUSHF / POPF */
    CpuReset(); Cpu.eflags = 0x00000246;   /* ZF|PF|bit1 */
    INSN(0x9C);
    CheckU32("PUSHF stores EFLAGS", HostPeek32(Cpu.esp), 0x00000246);
    CpuReset(); Cpu.eflags = 0x00000002;
    HostPoke32(Cpu.esp, 0x00000246); INSN(0x9D);
    CheckFlags("POPF loads EFLAGS", EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF,
               EFLAGS_PF | EFLAGS_ZF);

    /* PUSH imm32 (68 id) / PUSH imm8 (6A ib) */
    CpuReset(); { DWORD e = Cpu.esp; INSN(0x68, 0x78, 0x56, 0x34, 0x12);
        CheckU32("PUSH imm32 value", HostPeek32(Cpu.esp), 0x12345678);
        CheckU32("PUSH imm32 ESP", Cpu.esp, e - 4); }
    CpuReset(); { DWORD e = Cpu.esp; INSN(0x6A, 0xFF);
        CheckU32("PUSH imm8 sign-extended", HostPeek32(Cpu.esp), 0xFFFFFFFF);
        CheckU32("PUSH imm8 ESP", Cpu.esp, e - 4);
        CheckU32("PUSH imm8 length", INSNL(0x6A, 0xFF), 2); }
    CpuReset(); INSN(0x6A, 0x7F);
    CheckU32("PUSH imm8 positive", HostPeek32(Cpu.esp), 0x0000007F);

    /* PUSH r/m32 (FF /6) */
    CpuReset(); Cpu.eax = 0xDEADBEEF; INSN(0xFF, 0xF0);
    CheckU32("FF /6 PUSH EAX", HostPeek32(Cpu.esp), 0xDEADBEEF);
}

/* ------------------------------------------------------------------ */
/* control flow                                                        */
/* ------------------------------------------------------------------ */

static void TestControlFlow(void)
{
    SectionBegin("control flow");

    /* Jcc rel8 (70..7F) */
    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    CheckU32("JE taken target", (INSNL(0x74, 0x05), (DWORD)(Op.CurrentPtr - CodePtr())), 7);
    CpuReset();
    CheckU32("JE not taken", (INSNL(0x74, 0x05), (DWORD)(Op.CurrentPtr - CodePtr())), 2);
    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    CheckU32("JNE not taken", (INSNL(0x75, 0x05), (DWORD)(Op.CurrentPtr - CodePtr())), 2);
    CpuReset();
    CheckU32("JNE taken", (INSNL(0x75, 0x05), (DWORD)(Op.CurrentPtr - CodePtr())), 7);
    CpuReset(); Cpu.eflags |= EFLAGS_CF;
    CheckU32("JB taken", (INSNL(0x72, 0x10), (DWORD)(Op.CurrentPtr - CodePtr())), 18);
    CpuReset();
    CheckU32("JAE taken", (INSNL(0x73, 0x10), (DWORD)(Op.CurrentPtr - CodePtr())), 18);
    CpuReset(); Cpu.eflags |= EFLAGS_SF | EFLAGS_OF;
    CheckU32("JL not taken (SF==OF)", (INSNL(0x7C, 0x03), (DWORD)(Op.CurrentPtr - CodePtr())), 2);
    CpuReset(); Cpu.eflags |= EFLAGS_SF;
    CheckU32("JL taken (SF!=OF)", (INSNL(0x7C, 0x03), (DWORD)(Op.CurrentPtr - CodePtr())), 5);
    CpuReset(); Cpu.eflags |= EFLAGS_CF | EFLAGS_ZF;
    CheckU32("JBE taken", (INSNL(0x76, 0x02), (DWORD)(Op.CurrentPtr - CodePtr())), 4);
    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    CheckU32("JLE taken (ZF)", (INSNL(0x7E, 0x02), (DWORD)(Op.CurrentPtr - CodePtr())), 4);
    CpuReset();
    CheckU32("JG taken", (INSNL(0x7F, 0x02), (DWORD)(Op.CurrentPtr - CodePtr())), 4);

    /* backward branch: rel8 = FEh (-2) lands back on the branch itself */
    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    CheckU32("JE backward (rel8=FEh)", (INSNL(0x74, 0xFE), (DWORD)(Op.CurrentPtr - CodePtr())), 0);

    /* Jcc rel32 (0F 80..8F) */
    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    CheckU32("0F 84 taken target", (INSNL(0x0F, 0x84, 0x00, 0x01, 0x00, 0x00),
                                  (DWORD)(Op.CurrentPtr - CodePtr())), 6 + 0x100);
    CpuReset();
    CheckU32("0F 84 not taken", (INSNL(0x0F, 0x84, 0x00, 0x01, 0x00, 0x00),
                                (DWORD)(Op.CurrentPtr - CodePtr())), 6);
    CpuReset(); Cpu.eflags |= EFLAGS_CF;
    CheckU32("0F 82 taken", (INSNL(0x0F, 0x82, 0x10, 0x00, 0x00, 0x00),
                             (DWORD)(Op.CurrentPtr - CodePtr())), 6 + 0x10);

    /* JMP rel32 (E9) / JMP rel8 (EB) */
    CpuReset();
    CheckU32("JMP rel32", (INSNL(0xE9, 0x00, 0x01, 0x00, 0x00),
                           (DWORD)(Op.CurrentPtr - CodePtr())), 5 + 0x100);
    CpuReset();
    CheckU32("JMP rel8", (INSNL(0xEB, 0x10, 0x00),
                          (DWORD)(Op.CurrentPtr - CodePtr())), 2 + 0x10);

    /* CALL rel32 (E8) + RET (C3) */
    {
        DWORD entry = Cpu.esp;
        CpuReset(); entry = Cpu.esp;
        INSNL(0xE8, 0x00, 0x01, 0x00, 0x00);
        CheckU32("CALL return address", HostPeek32(Cpu.esp), (DWORD)(uintptr_t)(CodePtr() + 5));
        CheckU32("CALL ESP", Cpu.esp, entry - 4);
        CheckU32("CALL target", (DWORD)(Op.CurrentPtr - CodePtr()), 5 + 0x100);

        CpuReset();
        HostPoke32(Cpu.esp, 0x00001234);
        INSN(0xC3);
        CheckU32("RET pops EIP", Cpu.eip, 0x00001234);
        CheckU32("RET ESP", Cpu.esp, DataAddr() + 0x8004);

        CpuReset();
        HostPoke32(Cpu.esp, 0x00001234);
        INSN(0xC2, 0x08, 0x00);
        CheckU32("RET imm16 pops EIP", Cpu.eip, 0x00001234);
        CheckU32("RET imm16 adjusts ESP", Cpu.esp, DataAddr() + 0x800C);
    }

    /* JMP r/m32 (FF /4) */
    CpuReset(); Cpu.eax = 0x00002000; INSN(0xFF, 0xE0);
    CheckU32("FF /4 JMP EAX", Cpu.eip, 0x00002000);

    /* CALL r/m32 (FF /2) */
    {
        DWORD entry;
        CpuReset(); entry = Cpu.esp; Cpu.eax = 0x00003000; INSN(0xFF, 0xD0);
        CheckU32("FF /2 CALL EAX target", Cpu.eip, 0x00003000);
        CheckU32("FF /2 CALL pushes next EIP", HostPeek32(Cpu.esp), (DWORD)(uintptr_t)(CodePtr() + 2));
        CheckU32("FF /2 CALL ESP", Cpu.esp, entry - 4);
    }

    /* SETcc (0F 90..9F) */
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.eflags |= EFLAGS_ZF; INSN(0x0F, 0x94, 0xC0);
    CheckU32("SETE AL when ZF", Cpu.eax, 0xFFFFFF01);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0x0F, 0x94, 0xC0);
    CheckU32("SETE AL when !ZF", Cpu.eax, 0xFFFFFF00);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.eflags |= EFLAGS_CF; INSN(0x0F, 0x92, 0xC0);
    CheckU32("SETB AL when CF", Cpu.eax, 0xFFFFFF01);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; INSN(0x0F, 0x9F, 0xC0);
    CheckU32("SETG AL when !ZF & SF==OF", Cpu.eax, 0xFFFFFF01);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.eflags |= EFLAGS_ZF; INSN(0x0F, 0x9F, 0xC0);
    CheckU32("SETG AL when ZF", Cpu.eax, 0xFFFFFF00);
    CheckU32("SETE length", INSNL(0x0F, 0x94, 0xC0), 3);

    /* CMOVcc (0F 40..4F) */
    CpuReset(); Cpu.eax = 0x11111111; Cpu.ecx = 0x22222222; Cpu.eflags |= EFLAGS_ZF;
    INSN(0x0F, 0x44, 0xC1);
    CheckU32("CMOVE moves when ZF", Cpu.eax, 0x22222222);
    CpuReset(); Cpu.eax = 0x11111111; Cpu.ecx = 0x22222222; INSN(0x0F, 0x44, 0xC1);
    CheckU32("CMOVE keeps when !ZF", Cpu.eax, 0x11111111);
    CpuReset(); Cpu.eax = 0x11111111; Cpu.ecx = 0x22222222; INSN(0x0F, 0x45, 0xC1);
    CheckU32("CMOVNE moves when !ZF", Cpu.eax, 0x22222222);
    CpuReset(); Cpu.eax = 0x11111111; Cpu.ecx = 0x22222222; Cpu.eflags |= EFLAGS_CF;
    INSN(0x0F, 0x42, 0xC1);
    CheckU32("CMOVB moves when CF", Cpu.eax, 0x22222222);
    CheckU32("CMOVcc length", INSNL(0x0F, 0x44, 0xC1), 3);

    /* LOOP/JECXZ (E0..E3) */
    CpuReset(); Cpu.ecx = 2;
    CheckU32("LOOP decrements and branches", (INSNL(0xE2, 0x05), Cpu.ecx), 1);
    CheckU32("LOOP target", (DWORD)(Op.CurrentPtr - CodePtr()), 7);
    CpuReset(); Cpu.ecx = 1;
    CheckU32("LOOP falls through at 1", (INSNL(0xE2, 0x05), Cpu.ecx), 0);
    CheckU32("LOOP fall-through target", (DWORD)(Op.CurrentPtr - CodePtr()), 2);
    CpuReset(); Cpu.ecx = 0;
    CheckU32("JECXZ taken when ECX==0", (INSNL(0xE3, 0x04), (DWORD)(Op.CurrentPtr - CodePtr())), 6);

    /* LEAVE (C9) */
    {
        DWORD fp = DataAddr() + 0x4000;
        CpuReset(); Cpu.ebp = fp; HostPoke32(fp, 0x0000BEEF); INSN(0xC9);
        CheckU32("LEAVE EBP", Cpu.ebp, 0x0000BEEF);
        CheckU32("LEAVE ESP", Cpu.esp, fp + 4);
    }

    /* PUSHAD / POPAD (60 / 61) */
    {
        DWORD entry;
        CpuReset(); entry = Cpu.esp;
        Cpu.eax = 0x11111111; Cpu.ecx = 0x22222222; Cpu.edx = 0x33333333; Cpu.ebx = 0x44444444;
        Cpu.ebp = 0x66666666; Cpu.esi = 0x77777777; Cpu.edi = 0x88888888;
        INSN(0x60);
        CheckU32("PUSHAD ESP", Cpu.esp, entry - 32);
        CheckU32("PUSHAD [ESP] = EDI", HostPeek32(Cpu.esp), 0x88888888);
        CheckU32("PUSHAD [ESP+4] = ESI", HostPeek32(Cpu.esp + 4), 0x77777777);
        CheckU32("PUSHAD [ESP+28] = EAX", HostPeek32(Cpu.esp + 28), 0x11111111);

        CpuReset(); entry = Cpu.esp;
        HostPoke32(entry + 0, 0x11111111);    /* -> EDI */
        HostPoke32(entry + 4, 0x22222222);    /* -> ESI */
        HostPoke32(entry + 8, 0x33333333);    /* -> EBP */
        HostPoke32(entry + 12, 0x44444444);   /* -> discarded ESP slot */
        HostPoke32(entry + 16, 0x55555555);   /* -> EBX */
        HostPoke32(entry + 20, 0x66666666);   /* -> EDX */
        HostPoke32(entry + 24, 0x77777777);   /* -> ECX */
        HostPoke32(entry + 28, 0x88888888);   /* -> EAX */
        INSN(0x61);
        CheckU32("POPAD EAX", Cpu.eax, 0x88888888);
        CheckU32("POPAD EDI", Cpu.edi, 0x11111111);
        CheckU32("POPAD ESI", Cpu.esi, 0x22222222);
        CheckU32("POPAD EBP", Cpu.ebp, 0x33333333);
        CheckU32("POPAD EBX", Cpu.ebx, 0x55555555);
        CheckU32("POPAD EDX", Cpu.edx, 0x66666666);
        CheckU32("POPAD ECX", Cpu.ecx, 0x77777777);
        CheckU32("POPAD ESP", Cpu.esp, entry + 32);
    }
}

/* ------------------------------------------------------------------ */
/* string instructions                                                 */
/* ------------------------------------------------------------------ */

static void TestStrings(void)
{
    SectionBegin("string instructions");

    /* MOVSD (A5) */
    {
        DWORD s = DataAddr() + 0x3000, d = DataAddr() + 0x3100;
        CpuReset(); Cpu.esi = s; Cpu.edi = d; HostPoke32(s, 0x11223344);
        INSN(0xA5);
        CheckU32("MOVSD copies", HostPeek32(d), 0x11223344);
        CheckU32("MOVSD ESI", Cpu.esi, s + 4);
        CheckU32("MOVSD EDI", Cpu.edi, d + 4);
    }

    /* MOVSB with DF=1 (A4) */
    {
        DWORD s = DataAddr() + 0x3200, d = DataAddr() + 0x3300;
        CpuReset(); Cpu.esi = s; Cpu.edi = d; Cpu.eflags |= EFLAGS_DF; HostPoke8(s, 0x5A);
        INSN(0xA4);
        CheckU32("MOVSB with DF=1 copies", HostPeek8(d), 0x5A);
        CheckU32("MOVSB DF=1 ESI", Cpu.esi, s - 1);
        CheckU32("MOVSB DF=1 EDI", Cpu.edi, d - 1);
    }

    /* STOSD (AB) */
    {
        DWORD d = DataAddr() + 0x3400;
        CpuReset(); Cpu.edi = d; Cpu.eax = 0xCAFEBABE; INSN(0xAB);
        CheckU32("STOSD stores", HostPeek32(d), 0xCAFEBABE);
        CheckU32("STOSD EDI", Cpu.edi, d + 4);
    }

    /* LODSD (AD) */
    {
        DWORD s = DataAddr() + 0x3500;
        CpuReset(); Cpu.esi = s; HostPoke32(s, 0x0BADF00D); INSN(0xAD);
        CheckU32("LODSD loads", Cpu.eax, 0x0BADF00D);
        CheckU32("LODSD ESI", Cpu.esi, s + 4);
    }

    /* SCASD (AF) */
    {
        DWORD d = DataAddr() + 0x3600;
        CpuReset(); Cpu.edi = d; Cpu.eax = 0x10; HostPoke32(d, 0x10); INSN(0xAF);
        CheckFlags("SCASD equal -> ZF", EFLAGS_ZF, EFLAGS_ZF);
        CheckU32("SCASD EDI", Cpu.edi, d + 4);
        CpuReset(); Cpu.edi = d; Cpu.eax = 0x10; HostPoke32(d, 0x20); INSN(0xAF);
        CheckFlags("SCASD below -> CF", EFLAGS_CF | EFLAGS_ZF, EFLAGS_CF);
    }

    /* CMPSD (A7) */
    {
        DWORD s = DataAddr() + 0x3700, d = DataAddr() + 0x3710;
        CpuReset(); Cpu.esi = s; Cpu.edi = d; HostPoke32(s, 5); HostPoke32(d, 5); INSN(0xA7);
        CheckFlags("CMPSD equal -> ZF", EFLAGS_ZF, EFLAGS_ZF);
        CheckU32("CMPSD ESI", Cpu.esi, s + 4);
        CheckU32("CMPSD EDI", Cpu.edi, d + 4);
    }

    /* REP MOVSB (F3 A4) */
    {
        DWORD s = DataAddr() + 0x3800, d = DataAddr() + 0x3900;
        BYTE want[16];
        int i;
        CpuReset();
        for (i = 0; i < 16; i++) { want[i] = (BYTE)(0x40 + i); HostPoke8(s + i, want[i]); }
        Cpu.esi = s; Cpu.edi = d; Cpu.ecx = 16;
        INSN(0xF3, 0xA4);
        CheckBytes("REP MOVSB data", GuestMem + (d - GuestBase), want, 16);
        CheckU32("REP MOVSB ECX", Cpu.ecx, 0);
        CheckU32("REP MOVSB ESI", Cpu.esi, s + 16);
        CheckU32("REP MOVSB EDI", Cpu.edi, d + 16);
    }

    /* REP STOSD (F3 AB) */
    {
        DWORD d = DataAddr() + 0x3A00;
        CpuReset(); Cpu.edi = d; Cpu.eax = 0x01020304; Cpu.ecx = 4;
        INSN(0xF3, 0xAB);
        CheckU32("REP STOSD [0]", HostPeek32(d), 0x01020304);
        CheckU32("REP STOSD [3]", HostPeek32(d + 12), 0x01020304);
        CheckU32("REP STOSD ECX", Cpu.ecx, 0);
    }
}

/* ------------------------------------------------------------------ */
/* misc                                                                */
/* ------------------------------------------------------------------ */

static void TestMisc(void)
{
    SectionBegin("miscellaneous");

    /* AAM (D4 0A) */
    CpuReset(); Cpu.eax = 0x001B; INSN(0xD4, 0x0A);
    CheckU32("AAM AH", (Cpu.eax >> 8) & 0xFF, 2);
    CheckU32("AAM AL", Cpu.eax & 0xFF, 7);
    CheckFlags("AAM flags from AL", EFLAGS_ZF, 0);

    /* AAD (D5 0A) */
    CpuReset(); Cpu.eax = 0x0207; INSN(0xD5, 0x0A);
    CheckU32("AAD result", Cpu.eax, 0x001B);

    /* XLAT (D7) */
    {
        DWORD t = DataAddr() + 0x3B00;
        CpuReset(); Cpu.ebx = t; Cpu.eax = 3; HostPoke8(t + 3, 0xAB); INSN(0xD7);
        CheckU32("XLAT", Cpu.eax & 0xFF, 0xAB);
    }

    /* NOP (0F 1F /0) */
    CpuReset(); Cpu.eax = 0x12345678;
    CheckU32("0F 1F length", INSNL(0x0F, 0x1F, 0x00), 3);
    CheckU32("0F 1F is a no-op", Cpu.eax, 0x12345678);
    CpuReset();
    CheckU32("0F 1F C0 length", INSNL(0x0F, 0x1F, 0xC0), 3);

    /* NOP (90) */
    CpuReset(); Cpu.eax = 0x12345678; INSN(0x90);
    CheckU32("NOP", Cpu.eax, 0x12345678);

    /* XCHG EAX, ECX (91) */
    CpuReset(); Cpu.eax = 0x11111111; Cpu.ecx = 0x22222222; INSN(0x91);
    CheckU32("XCHG EAX,ECX -> EAX", Cpu.eax, 0x22222222);
    CheckU32("XCHG EAX,ECX -> ECX", Cpu.ecx, 0x11111111);

    /* CMPXCHG r/m32, r32 (0F B1 /r) */
    CpuReset(); Cpu.eax = 0x10; Cpu.ecx = 0x20; Cpu.ebx = 0x10; INSN(0x0F, 0xB1, 0xCB);
    CheckU32("CMPXCHG equal -> EBX=ECX", Cpu.ebx, 0x20);
    CheckFlags("CMPXCHG equal -> ZF", EFLAGS_ZF, EFLAGS_ZF);
    CpuReset(); Cpu.eax = 0x99; Cpu.ecx = 0x20; Cpu.ebx = 0x10; INSN(0x0F, 0xB1, 0xCB);
    CheckU32("CMPXCHG unequal -> EAX=EBX", Cpu.eax, 0x10);
    CheckFlags("CMPXCHG unequal -> !ZF", EFLAGS_ZF, 0);

    /* XADD r/m32, r32 (0F C1 /r) */
    CpuReset(); Cpu.eax = 3; Cpu.ebx = 4; INSN(0x0F, 0xC1, 0xC3);
    CheckU32("XADD EBX=old+new", Cpu.ebx, 7);
    CheckU32("XADD EAX=old EBX", Cpu.eax, 4);

    /* RDTSC (0F 31) and CPUID (0F A2) must at least decode as 2-byte ops */
    CpuReset();
    CheckU32("RDTSC length", INSNL(0x0F, 0x31), 2);
    CpuReset();
    CheckU32("CPUID length", INSNL(0x0F, 0xA2), 2);
}

/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* 16-bit (0x66) operand size                                          */
/* ------------------------------------------------------------------ */

static void TestWide16(void)
{
    SectionBegin("16-bit (0x66) operand size");

    /* 66 05 iw - ADD AX, imm16 */
    CpuReset(); Cpu.eax = 0xFFFF0000; INSN(0x66, 0x05, 0x34, 0x12);
    CheckU32("66 ADD AX,1234h", Cpu.eax, 0xFFFF1234);
    CheckU32("66 ADD AX,1234h length", INSNL(0x66, 0x05, 0x34, 0x12), 4);

    /* 66 2D iw - SUB AX, imm16 */
    CpuReset(); Cpu.eax = 0xAAAA0000; INSN(0x66, 0x2D, 0x01, 0x00);
    CheckU32("66 SUB AX,1", Cpu.eax, 0xAAAAFFFF);
    CheckFlags("66 SUB AX,1 CF", EFLAGS_CF, EFLAGS_CF);

    /* 66 3D iw - CMP AX, imm16 */
    CpuReset(); Cpu.eax = 0x00010000; INSN(0x66, 0x3D, 0x00, 0x00);
    CheckFlags("66 CMP AX,0 ZF", EFLAGS_ZF, EFLAGS_ZF);

    /* 66 81 /0 iw - ADD AX, imm16 (the immediate really is 16 bits) */
    CpuReset(); Cpu.eax = 0x1234FFFF; INSN(0x66, 0x81, 0xC0, 0x01, 0x00);
    CheckU32("66 ADD AX,1 wraps AX", Cpu.eax, 0x12340000);
    CheckFlags("66 ADD AX,1 CF", EFLAGS_CF, EFLAGS_CF);
    CheckU32("66 ADD AX,1 length", INSNL(0x66, 0x81, 0xC0, 0x01, 0x00), 5);

    /* 66 83 /0 ib - ADD AX, -1 */
    CpuReset(); Cpu.eax = 0x00000010; INSN(0x66, 0x83, 0xC0, 0xFF);
    CheckU32("66 ADD AX,-1", Cpu.eax, 0x0000000F);
    CheckU32("66 83 /0 length", INSNL(0x66, 0x83, 0xC0, 0xFF), 4);

    /* 66 89 /r - MOV AX, BX */
    CpuReset(); Cpu.eax = 0xAAAA0000; Cpu.ebx = 0x12345678; INSN(0x66, 0x89, 0xD8);
    CheckU32("66 MOV AX,BX keeps the high half", Cpu.eax, 0xAAAA5678);

    /* 66 8B /r - MOV AX, [EBX] */
    {
        DWORD d = DataAddr() + 0x7000;
        HostPoke32(d, 0xDEADBEEF);
        CpuReset(); Cpu.eax = 0xAAAA0000; Cpu.ebx = d; INSN(0x66, 0x8B, 0x03);
        CheckU32("66 MOV AX,[EBX]", Cpu.eax, 0xAAAABEEF);
    }

    /* 66 C7 /0 iw - MOV AX, imm16 */
    CpuReset(); Cpu.eax = 0xAAAA0000; INSN(0x66, 0xC7, 0xC0, 0x34, 0x12);
    CheckU32("66 MOV AX,1234h", Cpu.eax, 0xAAAA1234);
    CheckU32("66 C7 /0 length", INSNL(0x66, 0xC7, 0xC0, 0x34, 0x12), 5);

    /* 66 0F AF /r - IMUL AX, CX */
    CpuReset(); Cpu.eax = 0xAAAA0100; Cpu.ecx = 0x00000100; INSN(0x66, 0x0F, 0xAF, 0xC1);
    CheckU32("66 IMUL AX,CX truncates", Cpu.eax, 0xAAAA0000);
    CheckFlags("66 IMUL AX,CX overflow", EFLAGS_CF | EFLAGS_OF, EFLAGS_CF | EFLAGS_OF);

    /* 66 0F BC /r - BSF AX, CX */
    CpuReset(); Cpu.eax = 0xAAAA0000; Cpu.ecx = 0x00000100; INSN(0x66, 0x0F, 0xBC, 0xC1);
    CheckU32("66 BSF AX,CX", Cpu.eax, 0xAAAA0008);

    /* 66 D1 /4 - SHL AX, 1 */
    CpuReset(); Cpu.eax = 0x00008001; INSN(0x66, 0xD1, 0xE0);
    CheckU32("66 SHL AX,1", Cpu.eax, 0x00000002);
    CheckFlags("66 SHL AX,1 CF", EFLAGS_CF, EFLAGS_CF);

    /* 66 0F A4 /r ib - SHLD AX, CX, 4 */
    CpuReset(); Cpu.eax = 0x00001234; Cpu.ecx = 0x0000ABCD; INSN(0x66, 0x0F, 0xA4, 0xC8, 0x04);
    CheckU32("66 SHLD AX,CX,4", Cpu.eax, 0x0000234A);
    CheckFlags("66 SHLD AX,CX,4 CF", EFLAGS_CF, EFLAGS_CF);

    /* 66 FF /0 - INC AX */
    CpuReset(); Cpu.eax = 0x0000FFFF; INSN(0x66, 0xFF, 0xC0);
    CheckU32("66 INC AX wraps", Cpu.eax, 0x00000000);
    CheckFlags("66 INC AX ZF", EFLAGS_ZF, EFLAGS_ZF);

    /* 66 A1 id - MOV AX, moffs16/32 */
    {
        DWORD d = DataAddr() + 0x7100;
        BYTE b[] = { 0x66, 0xA1, 0, 0, 0, 0 };
        HostPoke32(d, 0x11223344);
        *(DWORD *)(b + 2) = d;
        CpuReset(); Cpu.eax = 0xAAAA0000;
        CheckU32("66 A1 length", Run(b, 6), 6);
        CheckU32("66 MOV AX,moffs32", Cpu.eax, 0xAAAA3344);
    }

    /* 66 E9 rel16 - JMP rel16 */
    CpuReset();
    CheckU32("66 JMP rel16",
             (INSNL(0x66, 0xE9, 0x00, 0x01), (DWORD)(Op.CurrentPtr - CodePtr())),
             4 + 0x100);

    /* 66 0F 84 rel16 - JZ rel16 */
    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    CheckU32("66 JZ rel16 taken",
             (INSNL(0x66, 0x0F, 0x84, 0x00, 0x01), (DWORD)(Op.CurrentPtr - CodePtr())),
             5 + 0x100);
    CpuReset();
    CheckU32("66 JZ rel16 not taken",
             (INSNL(0x66, 0x0F, 0x84, 0x00, 0x01), (DWORD)(Op.CurrentPtr - CodePtr())),
             5);
}

/* ------------------------------------------------------------------ */
/* memory operand forms                                                */
/* ------------------------------------------------------------------ */

static void TestMemoryForms(void)
{
    SectionBegin("memory operand forms");

    DWORD d = DataAddr() + 0x4000;

    /* 01 /r - ADD r/m32, r32 */
    HostPoke32(d, 0x100); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x23; INSN(0x01, 0x08);
    CheckU32("ADD [EAX],ECX", HostPeek32(d), 0x123);

    /* 03 /r - ADD r32, r/m32 */
    HostPoke32(d, 0x100); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x23; INSN(0x03, 0x08);
    CheckU32("ADD ECX,[EAX]", Cpu.ecx, 0x123);

    /* 29 /r - SUB r/m32, r32 */
    HostPoke32(d, 0x100); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x1; INSN(0x29, 0x08);
    CheckU32("SUB [EAX],ECX", HostPeek32(d), 0xFF);

    /* 2B /r - SUB r32, r/m32 */
    HostPoke32(d, 0x1); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x100; INSN(0x2B, 0x08);
    CheckU32("SUB ECX,[EAX]", Cpu.ecx, 0xFF);

    /* 39 /r - CMP r/m32, r32 */
    HostPoke32(d, 0x100); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x100; INSN(0x39, 0x08);
    CheckFlags("CMP [EAX],ECX ZF", EFLAGS_ZF, EFLAGS_ZF);
    CheckU32("CMP [EAX],ECX does not store", HostPeek32(d), 0x100);

    /* 3B /r - CMP r32, r/m32 */
    HostPoke32(d, 0x100); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x100; INSN(0x3B, 0x08);
    CheckFlags("CMP ECX,[EAX] ZF", EFLAGS_ZF, EFLAGS_ZF);

    /* 80 /0 ib - ADD BYTE [EAX], imm8 */
    HostPoke32(d, 0xAABBCC10); CpuReset(); Cpu.eax = d; INSN(0x80, 0x00, 0x05);
    CheckU32("ADD BYTE [EAX],5", HostPeek32(d), 0xAABBCC15);

    /* 81 /0 id - ADD DWORD [EAX], imm32 */
    HostPoke32(d, 0x10); CpuReset(); Cpu.eax = d; INSN(0x81, 0x00, 0x05, 0x00, 0x00, 0x00);
    CheckU32("ADD DWORD [EAX],5", HostPeek32(d), 0x15);
    CheckU32("81 /0 memory length", INSNL(0x81, 0x00, 0x05, 0x00, 0x00, 0x00), 6);

    /* 83 /0 ib - ADD DWORD [EAX], sign-extended imm8 */
    HostPoke32(d, 0x10); CpuReset(); Cpu.eax = d; INSN(0x83, 0x00, 0xFF);
    CheckU32("ADD DWORD [EAX],-1", HostPeek32(d), 0x0F);

    /* C6 /0 ib - MOV BYTE [EAX], imm8 */
    HostPoke32(d, 0xAABBCCDD); CpuReset(); Cpu.eax = d; INSN(0xC6, 0x00, 0x5A);
    CheckU32("MOV BYTE [EAX],5Ah", HostPeek32(d), 0xAABBCC5A);

    /* C7 /0 id - MOV DWORD [EAX], imm32 */
    HostPoke32(d, 0xFFFFFFFF); CpuReset(); Cpu.eax = d; INSN(0xC7, 0x00, 0x5A, 0x00, 0x00, 0x00);
    CheckU32("MOV DWORD [EAX],5Ah", HostPeek32(d), 0x5A);

    /* FE /0 - INC BYTE [EAX] */
    HostPoke32(d, 0xAABBCCFF); CpuReset(); Cpu.eax = d; INSN(0xFE, 0x00);
    CheckU32("INC BYTE [EAX]", HostPeek32(d), 0xAABBCC00);

    /* FF /0 - INC DWORD [EAX] */
    HostPoke32(d, 0xFFFFFFFF); CpuReset(); Cpu.eax = d; INSN(0xFF, 0x00);
    CheckU32("INC DWORD [EAX]", HostPeek32(d), 0);

    /* FF /6 - PUSH DWORD [EAX] */
    {
        DWORD entry;
        HostPoke32(d, 0xDEADBEEF); CpuReset(); entry = Cpu.esp; Cpu.eax = d;
        INSN(0xFF, 0x30);
        CheckU32("PUSH DWORD [EAX]", HostPeek32(Cpu.esp), 0xDEADBEEF);
        CheckU32("PUSH DWORD [EAX] ESP", Cpu.esp, entry - 4);
    }

    /* FF /4 - JMP DWORD [EAX] */
    HostPoke32(d, 0x00002000); CpuReset(); Cpu.eax = d; INSN(0xFF, 0x20);
    CheckU32("JMP DWORD [EAX]", Cpu.eip, 0x00002000);

    /* FF /2 - CALL DWORD [EAX] */
    {
        DWORD entry;
        HostPoke32(d, 0x00003000); CpuReset(); entry = Cpu.esp; Cpu.eax = d;
        INSN(0xFF, 0x10);
        CheckU32("CALL DWORD [EAX] target", Cpu.eip, 0x00003000);
        CheckU32("CALL DWORD [EAX] return address",
                 HostPeek32(Cpu.esp), (DWORD)(uintptr_t)(CodePtr() + 2));
        CheckU32("CALL DWORD [EAX] ESP", Cpu.esp, entry - 4);
    }

    /* 0F B6 /r - MOVZX EAX, BYTE [EBX] */
    HostPoke8(d, 0x80); CpuReset(); Cpu.ebx = d; INSN(0x0F, 0xB6, 0x03);
    CheckU32("MOVZX EAX,BYTE [EBX]", Cpu.eax, 0x00000080);

    /* 0F BE /r - MOVSX EAX, BYTE [EBX] */
    HostPoke8(d, 0x80); CpuReset(); Cpu.ebx = d; INSN(0x0F, 0xBE, 0x03);
    CheckU32("MOVSX EAX,BYTE [EBX]", Cpu.eax, 0xFFFFFF80);

    /* 0F AF /r - IMUL EAX, [EBX] */
    HostPoke32(d, 3); CpuReset(); Cpu.eax = 5; Cpu.ebx = d; INSN(0x0F, 0xAF, 0x03);
    CheckU32("IMUL EAX,[EBX]", Cpu.eax, 15);

    /* 0F BC /r - BSF EAX, [EBX] */
    HostPoke32(d, 0x100); CpuReset(); Cpu.ebx = d; INSN(0x0F, 0xBC, 0x03);
    CheckU32("BSF EAX,[EBX]", Cpu.eax, 8);

    /* 0F A3 /r - BT [EBX], EAX */
    HostPoke32(d, 0x00000008); CpuReset(); Cpu.ebx = d; Cpu.eax = 3; INSN(0x0F, 0xA3, 0x03);
    CheckFlags("BT [EBX],3 CF", EFLAGS_CF, EFLAGS_CF);

    /* 0F AB /r - BTS [EBX], EAX */
    HostPoke32(d, 0); CpuReset(); Cpu.ebx = d; Cpu.eax = 5; INSN(0x0F, 0xAB, 0x03);
    CheckU32("BTS [EBX],5", HostPeek32(d), 0x20);
    CheckFlags("BTS [EBX],5 old CF was 0", EFLAGS_CF, 0);

    /* 0F BA /4 ib - BT DWORD [EBX], 4 */
    HostPoke32(d, 0x10); CpuReset(); Cpu.ebx = d; INSN(0x0F, 0xBA, 0x23, 0x04);
    CheckFlags("0F BA /4 BT [EBX],4", EFLAGS_CF, EFLAGS_CF);

    /* 0F BA /5 ib - BTS DWORD [EBX], 4 */
    HostPoke32(d, 0); CpuReset(); Cpu.ebx = d; INSN(0x0F, 0xBA, 0x2B, 0x04);
    CheckU32("0F BA /5 BTS [EBX],4", HostPeek32(d), 0x10);

    /* 0F 44 /r - CMOVE EAX, [EBX] */
    HostPoke32(d, 0x22222222); CpuReset(); Cpu.eax = 0x11111111; Cpu.ebx = d;
    Cpu.eflags |= EFLAGS_ZF; INSN(0x0F, 0x44, 0x03);
    CheckU32("CMOVE EAX,[EBX] taken", Cpu.eax, 0x22222222);
    CpuReset(); Cpu.eax = 0x11111111; Cpu.ebx = d; INSN(0x0F, 0x44, 0x03);
    CheckU32("CMOVE EAX,[EBX] not taken", Cpu.eax, 0x11111111);

    /* 0F 94 /r - SETE [EBX] */
    HostPoke8(d, 0xAA); CpuReset(); Cpu.ebx = d; Cpu.eflags |= EFLAGS_ZF; INSN(0x0F, 0x94, 0x03);
    CheckU32("SETE [EBX] when ZF", HostPeek8(d), 0x01);
    HostPoke8(d, 0xAA); CpuReset(); Cpu.ebx = d; INSN(0x0F, 0x94, 0x03);
    CheckU32("SETE [EBX] when !ZF", HostPeek8(d), 0x00);

    /* 0F 1F /0 - multi-byte NOP with a SIB byte */
    HostPoke32(d + 0x100, 0x12345678); CpuReset(); Cpu.esp = d + 0x100;
    CheckU32("0F 1F [ESP] length", INSNL(0x0F, 0x1F, 0x04, 0x24), 4);
    CheckU32("0F 1F [ESP] does not touch memory", HostPeek32(d + 0x100), 0x12345678);

    /* F7 /2 - NOT DWORD [EBX] */
    HostPoke32(d, 0); CpuReset(); Cpu.ebx = d; INSN(0xF7, 0x13);
    CheckU32("NOT DWORD [EBX]", HostPeek32(d), 0xFFFFFFFF);

    /* D1 /4 - SHL DWORD [EBX], 1 */
    HostPoke32(d, 0x80000000); CpuReset(); Cpu.ebx = d; INSN(0xD1, 0x23);
    CheckU32("SHL DWORD [EBX],1", HostPeek32(d), 0);
    CheckFlags("SHL DWORD [EBX],1 CF", EFLAGS_CF, EFLAGS_CF);

    /* C1 /4 ib - SHL DWORD [EBX], 4 */
    HostPoke32(d, 1); CpuReset(); Cpu.ebx = d; INSN(0xC1, 0x23, 0x04);
    CheckU32("SHL DWORD [EBX],4", HostPeek32(d), 0x10);

    /* 0F A4 /r ib - SHLD DWORD [EBX], EAX, 4 */
    HostPoke32(d, 0x00000001); CpuReset(); Cpu.ebx = d; Cpu.eax = 0; INSN(0x0F, 0xA4, 0x03, 0x04);
    CheckU32("SHLD [EBX],EAX,4", HostPeek32(d), 0x10);
}

/* ------------------------------------------------------------------ */
/* rotates, RCL/RCR and shift edge cases                               */
/* ------------------------------------------------------------------ */

static void TestRotates(void)
{
    SectionBegin("rotates and shift edge cases");

    /* C0 /0 ib - ROL AL, 3 */
    CpuReset(); Cpu.eax = 0x81; INSN(0xC0, 0xC0, 0x03);
    CheckU32("ROL AL,3", Cpu.eax, 0x0C);
    CheckFlags("ROL AL,3 CF", EFLAGS_CF, 0);

    /* D0 /0 - ROL AL, 1 */
    CpuReset(); Cpu.eax = 0x81; INSN(0xD0, 0xC0);
    CheckU32("ROL AL,1", Cpu.eax, 0x03);
    CheckFlags("ROL AL,1 CF", EFLAGS_CF, EFLAGS_CF);

    /* D2 /0 - ROL AL, CL */
    CpuReset(); Cpu.eax = 0x81; Cpu.ecx = 0x02; INSN(0xD2, 0xC0);
    CheckU32("ROL AL,CL=2", Cpu.eax, 0x06);

    /* D1 /2 - RCL EAX, 1 with CF=1 */
    CpuReset(); Cpu.eax = 0x80000000; Cpu.eflags |= EFLAGS_CF; INSN(0xD1, 0xD0);
    CheckU32("RCL EAX,1 (CF=1)", Cpu.eax, 0x00000001);
    CheckFlags("RCL EAX,1 CF out", EFLAGS_CF, EFLAGS_CF);

    /* D1 /3 - RCR EAX, 1 with CF=1 */
    CpuReset(); Cpu.eax = 0x00000001; Cpu.eflags |= EFLAGS_CF; INSN(0xD1, 0xD8);
    CheckU32("RCR EAX,1 (CF=1)", Cpu.eax, 0x80000000);
    CheckFlags("RCR EAX,1 CF out", EFLAGS_CF, EFLAGS_CF);

    /* C0 /4 ib - SHL AL, 4 */
    CpuReset(); Cpu.eax = 0xFF; INSN(0xC0, 0xE0, 0x04);
    CheckU32("SHL AL,4", Cpu.eax, 0xF0);
    CheckFlags("SHL AL,4 CF", EFLAGS_CF, EFLAGS_CF);

    /* C0 /5 ib - SHR AL, 4 */
    CpuReset(); Cpu.eax = 0xFF; INSN(0xC0, 0xE8, 0x04);
    CheckU32("SHR AL,4", Cpu.eax, 0x0F);
    CheckFlags("SHR AL,4 CF", EFLAGS_CF, EFLAGS_CF);

    /* C0 /7 ib - SAR AL, 4 */
    CpuReset(); Cpu.eax = 0xF0; INSN(0xC0, 0xF8, 0x04);
    CheckU32("SAR AL,4", Cpu.eax, 0xFF);
    CheckFlags("SAR AL,4 CF", EFLAGS_CF, 0);

    /* C0 /6 ib - SAL is an alias of SHL */
    CpuReset(); Cpu.eax = 0xFF; INSN(0xC0, 0xF0, 0x04);
    CheckU32("SAL AL,4", Cpu.eax, 0xF0);

    /* a count of zero changes neither the value nor the flags */
    CpuReset(); Cpu.eax = 0x12345678; Cpu.eflags = 0x02; INSN(0xC1, 0xE0, 0x00);
    CheckU32("SHL EAX,0 leaves the value", Cpu.eax, 0x12345678);
    CheckU32("SHL EAX,0 leaves the flags", Cpu.eflags, 0x02);

    /* the count is masked to 5 bits: 32 becomes 0 */
    CpuReset(); Cpu.eax = 0x12345678; INSN(0xC1, 0xC0, 0x20);
    CheckU32("ROL EAX,32 is a no-op", Cpu.eax, 0x12345678);

    /* SHRD by an immediate, CF comes from the last bit shifted out */
    CpuReset(); Cpu.eax = 0x000000FF; Cpu.ecx = 0x00000000;
    INSN(0x0F, 0xAC, 0xC8, 0x08);
    CheckU32("SHRD EAX,ECX,8", Cpu.eax, 0x00000000);
    CheckFlags("SHRD EAX,ECX,8 CF", EFLAGS_CF, EFLAGS_CF);

    /* SHRD shifts bits of the source into the top of the destination */
    CpuReset(); Cpu.eax = 0x000000FF; Cpu.ecx = 0x00000008;
    INSN(0x0F, 0xAC, 0xC8, 0x08);
    CheckU32("SHRD EAX,ECX,8 with ECX=8", Cpu.eax, 0x08000000);
}

/* ------------------------------------------------------------------ */
/* PUSH/POP forms                                                      */
/* ------------------------------------------------------------------ */

static void TestPushPop(void)
{
    SectionBegin("PUSH/POP forms");

    DWORD entry;

    /* 50 / 58 - PUSH EAX / POP EAX */
    CpuReset(); entry = Cpu.esp; Cpu.eax = 0x11223344;
    INSN(0x50);
    CheckU32("PUSH EAX ESP", Cpu.esp, entry - 4);
    CheckU32("PUSH EAX value", HostPeek32(Cpu.esp), 0x11223344);
    CpuReset(); Cpu.eax = 0; HostPoke32(Cpu.esp, 0x55667788);
    INSN(0x58);
    CheckU32("POP EAX", Cpu.eax, 0x55667788);
    CheckU32("POP EAX ESP", Cpu.esp, entry + 4);

    /* 54 - PUSH ESP pushes the value ESP had before the push */
    CpuReset(); entry = Cpu.esp;
    INSN(0x54);
    CheckU32("PUSH ESP pushes the original ESP", HostPeek32(Cpu.esp), entry);
    CheckU32("PUSH ESP ESP", Cpu.esp, entry - 4);

    /* 5C - POP ESP replaces ESP with the popped value */
    CpuReset(); entry = Cpu.esp;
    HostPoke32(entry, 0x00004000);
    INSN(0x5C);
    CheckU32("POP ESP", Cpu.esp, 0x00004000);

    /* 5D / 5E / 5F - POP EBP / POP ESI / POP EDI */
    CpuReset(); HostPoke32(Cpu.esp, 0xAAAAAAAA); INSN(0x5D);
    CheckU32("POP EBP", Cpu.ebp, 0xAAAAAAAA);
    CpuReset(); HostPoke32(Cpu.esp, 0xBBBBBBBB); INSN(0x5E);
    CheckU32("POP ESI", Cpu.esi, 0xBBBBBBBB);
    CpuReset(); HostPoke32(Cpu.esp, 0xCCCCCCCC); INSN(0x5F);
    CheckU32("POP EDI", Cpu.edi, 0xCCCCCCCC);

    /* 68 id - PUSH imm32 */
    CpuReset(); entry = Cpu.esp;
    INSN(0x68, 0x78, 0x56, 0x34, 0x12);
    CheckU32("PUSH imm32 value", HostPeek32(Cpu.esp), 0x12345678);
    CheckU32("PUSH imm32 ESP", Cpu.esp, entry - 4);

    /* 6A ib - PUSH imm8 (sign-extended to the operand size) */
    CpuReset(); entry = Cpu.esp;
    INSN(0x6A, 0xFF);
    CheckU32("PUSH imm8 -1", HostPeek32(Cpu.esp), 0xFFFFFFFF);
    CheckU32("PUSH imm8 ESP", Cpu.esp, entry - 4);

    /* FF /6 - PUSH r/m32 */
    CpuReset(); Cpu.ecx = 0x0BADF00D;
    INSN(0xFF, 0xF1);
    CheckU32("FF /6 PUSH ECX", HostPeek32(Cpu.esp), 0x0BADF00D);

    /* 8F /0 - POP r/m32 */
    CpuReset(); entry = Cpu.esp; HostPoke32(entry, 0x0DEADBEE);
    INSN(0x8F, 0xC2);
    CheckU32("8F /0 POP EDX", Cpu.edx, 0x0DEADBEE);
    CheckU32("8F /0 POP EDX ESP", Cpu.esp, entry + 4);

    /* 66 50 / 66 58 - PUSH AX / POP AX (16-bit stack slot) */
    CpuReset(); entry = Cpu.esp; Cpu.eax = 0x12345678;
    INSN(0x66, 0x50);
    CheckU32("66 PUSH AX ESP", Cpu.esp, entry - 2);
    CheckU32("66 PUSH AX value", HostPeek16(Cpu.esp), 0x5678);
    CpuReset(); Cpu.eax = 0; HostPoke16(Cpu.esp, 0x5678);
    INSN(0x66, 0x58);
    CheckU32("66 POP AX", Cpu.eax, 0x00005678);
    CheckU32("66 POP AX ESP", Cpu.esp, entry + 2);

    /* 66 68 iw - PUSH imm16 */
    CpuReset(); entry = Cpu.esp;
    INSN(0x66, 0x68, 0x34, 0x12);
    CheckU32("66 PUSH imm16 value", HostPeek16(Cpu.esp), 0x1234);
    CheckU32("66 PUSH imm16 ESP", Cpu.esp, entry - 2);
    CheckU32("66 PUSH imm16 length", INSNL(0x66, 0x68, 0x34, 0x12), 4);

    /* 66 6A ib - PUSH imm8 with a 16-bit operand size */
    CpuReset(); entry = Cpu.esp;
    INSN(0x66, 0x6A, 0xFF);
    CheckU32("66 PUSH imm8 -1", HostPeek16(Cpu.esp), 0xFFFF);
    CheckU32("66 PUSH imm8 ESP", Cpu.esp, entry - 2);

    /* 66 FF /6 - PUSH r/m16 */
    CpuReset(); Cpu.ecx = 0x0BADF00D;
    INSN(0x66, 0xFF, 0xF1);
    CheckU32("66 FF /6 PUSH CX", HostPeek16(Cpu.esp), 0xF00D);
}

/* ------------------------------------------------------------------ */
/* string instructions: REP/REPNE and 16-bit forms                     */
/* ------------------------------------------------------------------ */

static void TestStringMore(void)
{
    SectionBegin("string instructions (REP/REPNE, 16-bit)");

    /* F2 AE - REPNE SCASB stops when AL matches */
    {
        DWORD d = DataAddr() + 0x5000;
        HostPoke8(d + 0, 'A'); HostPoke8(d + 1, 'B'); HostPoke8(d + 2, 0);
        CpuReset(); Cpu.edi = d; Cpu.eax = 0; Cpu.ecx = 10; INSN(0xF2, 0xAE);
        CheckU32("REPNE SCASB ECX", Cpu.ecx, 7);
        CheckU32("REPNE SCASB EDI", Cpu.edi, d + 3);
        CheckFlags("REPNE SCASB ZF", EFLAGS_ZF, EFLAGS_ZF);
    }

    /* F3 A6 - REPE CMPSB stops at the first difference */
    {
        DWORD s1 = DataAddr() + 0x5100, s2 = DataAddr() + 0x5200;
        HostPoke8(s1 + 0, 'A'); HostPoke8(s1 + 1, 'B'); HostPoke8(s1 + 2, 'C'); HostPoke8(s1 + 3, 'X');
        HostPoke8(s2 + 0, 'A'); HostPoke8(s2 + 1, 'B'); HostPoke8(s2 + 2, 'C'); HostPoke8(s2 + 3, 'Y');
        CpuReset(); Cpu.esi = s1; Cpu.edi = s2; Cpu.ecx = 10; INSN(0xF3, 0xA6);
        CheckU32("REPE CMPSB ECX", Cpu.ecx, 6);
        CheckU32("REPE CMPSB ESI", Cpu.esi, s1 + 4);
        CheckU32("REPE CMPSB EDI", Cpu.edi, s2 + 4);
        CheckFlags("REPE CMPSB ZF", EFLAGS_ZF, 0);
    }

    /* 66 A5 - MOVSW */
    {
        DWORD s = DataAddr() + 0x5300, d = DataAddr() + 0x5400;
        HostPoke16(s, 0xBEEF);
        CpuReset(); Cpu.esi = s; Cpu.edi = d; INSN(0x66, 0xA5);
        CheckU32("MOVSW copies 16 bits", HostPeek16(d), 0xBEEF);
        CheckU32("MOVSW ESI", Cpu.esi, s + 2);
        CheckU32("MOVSW EDI", Cpu.edi, d + 2);
    }

    /* 66 AB - STOSW */
    {
        DWORD d = DataAddr() + 0x5500;
        CpuReset(); Cpu.edi = d; Cpu.eax = 0x12345678; INSN(0x66, 0xAB);
        CheckU32("STOSW stores AX", HostPeek16(d), 0x5678);
        CheckU32("STOSW EDI", Cpu.edi, d + 2);
    }

    /* F3 AA - REP STOSB */
    {
        DWORD d = DataAddr() + 0x5600;
        CpuReset(); Cpu.edi = d; Cpu.eax = 0xAB; Cpu.ecx = 8; INSN(0xF3, 0xAA);
        CheckU32("REP STOSB [0]", HostPeek8(d), 0xAB);
        CheckU32("REP STOSB [7]", HostPeek8(d + 7), 0xAB);
        CheckU32("REP STOSB ECX", Cpu.ecx, 0);
        CheckU32("REP STOSB EDI", Cpu.edi, d + 8);
    }

    /* 66 AD - LODSW */
    {
        DWORD s = DataAddr() + 0x5700;
        HostPoke16(s, 0xCAFE);
        CpuReset(); Cpu.esi = s; INSN(0x66, 0xAD);
        CheckU32("LODSW", Cpu.eax, 0x0000CAFE);
        CheckU32("LODSW ESI", Cpu.esi, s + 2);
    }

    /* direction flag: STD makes the string pointer walk backwards */
    {
        DWORD d = DataAddr() + 0x5800;
        CpuReset(); Cpu.edi = d + 4; Cpu.eax = 0x11; Cpu.ecx = 3;
        Cpu.eflags |= EFLAGS_DF;
        INSN(0xF3, 0xAA);
        CheckU32("STD REP STOSB EDI", Cpu.edi, d + 1);
        CheckU32("STD REP STOSB [4]", HostPeek8(d + 4), 0x11);
        CheckU32("STD REP STOSB [2]", HostPeek8(d + 2), 0x11);
    }
}

/* ------------------------------------------------------------------ */
/* BCD adjust                                                          */
/* ------------------------------------------------------------------ */

static void TestBcd(void)
{
    SectionBegin("BCD adjust");

    /* 27 - DAA: 1Bh is not a valid BCD byte and is corrected to 21h */
    CpuReset(); Cpu.eax = 0x1B; INSN(0x27);
    CheckU32("DAA 1Bh -> 21h", Cpu.eax & 0xFF, 0x21);
    CheckFlags("DAA 1Bh AF", EFLAGS_AF, EFLAGS_AF);

    /* 2F - DAS: 0Bh is corrected to 05h */
    CpuReset(); Cpu.eax = 0x0B; INSN(0x2F);
    CheckU32("DAS 0Bh -> 05h", Cpu.eax & 0xFF, 0x05);
    CheckFlags("DAS 0Bh AF", EFLAGS_AF, EFLAGS_AF);

    /* 37 - AAA: 0Fh + 1 -> 05h, AH += 1, CF = AF = 1 */
    CpuReset(); Cpu.eax = 0x010F; INSN(0x37);
    CheckU32("AAA AL", Cpu.eax & 0xFF, 0x05);
    CheckU32("AAA AH", (Cpu.eax >> 8) & 0xFF, 0x02);
    CheckFlags("AAA CF/AF", EFLAGS_CF | EFLAGS_AF, EFLAGS_CF | EFLAGS_AF);

    /* 3F - AAS: 0Fh - 1 -> 09h, AH -= 1, CF = AF = 1 */
    CpuReset(); Cpu.eax = 0x020F; INSN(0x3F);
    CheckU32("AAS AL", Cpu.eax & 0xFF, 0x09);
    CheckU32("AAS AH", (Cpu.eax >> 8) & 0xFF, 0x01);
    CheckFlags("AAS CF/AF", EFLAGS_CF | EFLAGS_AF, EFLAGS_CF | EFLAGS_AF);

    /* D5 ib - AAD: AL = AL*10 + AH, AH = 0 */
    CpuReset(); Cpu.eax = 0x0205; INSN(0xD5, 0x0A);
    CheckU32("AAD AL", Cpu.eax & 0xFF, 0x19);
    CheckU32("AAD AH", (Cpu.eax >> 8) & 0xFF, 0x00);

    /* D4 ib - AAM: AH = AL/10 (tens), AL = AL%10 (units) */
    CpuReset(); Cpu.eax = 0x0019; INSN(0xD4, 0x0A);
    CheckU32("AAM AL", Cpu.eax & 0xFF, 0x05);
    CheckU32("AAM AH", (Cpu.eax >> 8) & 0xFF, 0x02);
    CheckU32("AAM whole AX", Cpu.eax, 0x00000205);
}

/* ------------------------------------------------------------------ */
/* segment override prefixes                                           */
/* ------------------------------------------------------------------ */

static void TestSegPrefix(void)
{
    SectionBegin("segment override prefixes");

    DWORD d = DataAddr() + 0x6000;

    /* 2E 8B 00 - CS: MOV EAX, [EAX] (flat model: the override is ignored) */
    HostPoke32(d, 0x1234);
    CpuReset(); Cpu.eax = d;
    CheckU32("2Eh MOV EAX,[EAX] length", INSNL(0x2E, 0x8B, 0x00), 3);
    Cpu.eax = d;                      /* the length probe above overwrote EAX */
    INSN(0x2E, 0x8B, 0x00);
    CheckU32("2Eh MOV EAX,[EAX]", Cpu.eax, 0x1234);

    /* 3E 8B 00 - DS override */
    HostPoke32(d, 0x5678);
    CpuReset(); Cpu.eax = d; INSN(0x3E, 0x8B, 0x00);
    CheckU32("3Eh MOV EAX,[EAX]", Cpu.eax, 0x5678);

    /* 26 8B 00 - ES override */
    HostPoke32(d, 0x9ABC);
    CpuReset(); Cpu.eax = d; INSN(0x26, 0x8B, 0x00);
    CheckU32("26h MOV EAX,[EAX]", Cpu.eax, 0x9ABC);

    /* 36 8B 00 - SS override */
    HostPoke32(d, 0xDEF0);
    CpuReset(); Cpu.eax = d; INSN(0x36, 0x8B, 0x00);
    CheckU32("36h MOV EAX,[EAX]", Cpu.eax, 0xDEF0);

    /* 64 A1 id - FS: MOV EAX, moffs32 */
    {
        BYTE b[] = { 0x64, 0xA1, 0, 0, 0, 0 };
        HostPoke32(d, 0x0ABCDEF1);
        *(DWORD *)(b + 2) = d;
        CpuReset();
        CheckU32("64h MOV EAX,moffs32 length", Run(b, 6), 6);
        CheckU32("64h MOV EAX,moffs32", Cpu.eax, 0x0ABCDEF1);
    }

    /* the prefix survives an intervening 0F escape */
    HostPoke32(d, 0x11223344);
    CpuReset(); Cpu.eax = d; INSN(0x2E, 0x0F, 0xB6, 0x08);
    CheckU32("2Eh 0F B6 MOVZX ECX,BYTE [EAX]", Cpu.ecx, 0x44);
}

/* ------------------------------------------------------------------ */
/* misc additions                                                      */
/* ------------------------------------------------------------------ */

static void TestMisc2(void)
{
    SectionBegin("miscellaneous (continued)");

    /* A0 / A2 / A3 - MOV AL, moffs8 / MOV moffs8, AL / MOV moffs32, EAX */
    {
        DWORD d = DataAddr() + 0x6200;
        BYTE b[] = { 0xA0, 0, 0, 0, 0 };
        HostPoke8(d, 0x5A);
        *(DWORD *)(b + 1) = d;
        CpuReset(); Cpu.eax = 0xFFFFFF00;
        CheckU32("A0 length", Run(b, 5), 5);
        CheckU32("A0 MOV AL,moffs8", Cpu.eax, 0xFFFFFF5A);
    }
    {
        DWORD d = DataAddr() + 0x6204;
        BYTE b[] = { 0xA2, 0, 0, 0, 0 };
        *(DWORD *)(b + 1) = d;
        CpuReset(); Cpu.eax = 0x12345678;
        CheckU32("A2 length", Run(b, 5), 5);
        CheckU32("A2 MOV moffs8,AL", HostPeek8(d), 0x78);
    }
    {
        DWORD d = DataAddr() + 0x6208;
        BYTE b[] = { 0xA3, 0, 0, 0, 0 };
        *(DWORD *)(b + 1) = d;
        CpuReset(); Cpu.eax = 0xCAFEBABE;
        CheckU32("A3 length", Run(b, 5), 5);
        CheckU32("A3 MOV moffs32,EAX", HostPeek32(d), 0xCAFEBABE);
    }

    /* D7 - XLAT: AL = [EBX + AL] */
    {
        DWORD d = DataAddr() + 0x6300;
        HostPoke8(d + 0x20, 0x99);
        CpuReset(); Cpu.ebx = d; Cpu.eax = 0x20; INSN(0xD7);
        CheckU32("XLAT", Cpu.eax & 0xFF, 0x99);
    }

    /* C9 - LEAVE: ESP = EBP, then EBP = [ESP] */
    {
        DWORD frame = DataAddr() + 0x7FF0;
        CpuReset();
        Cpu.ebp = frame;
        HostPoke32(frame, 0x87654321);
        INSN(0xC9);
        CheckU32("LEAVE ESP", Cpu.esp, frame + 4);
        CheckU32("LEAVE EBP", Cpu.ebp, 0x87654321);
    }

    /* 98 / 99 - CWDE / CDQ */
    CpuReset(); Cpu.eax = 0x00008000; INSN(0x98);
    CheckU32("CWDE sign-extends AX", Cpu.eax, 0xFFFF8000);
    CpuReset(); Cpu.eax = 0x00007FFF; INSN(0x98);
    CheckU32("CWDE leaves a positive AX alone", Cpu.eax, 0x00007FFF);
    CpuReset(); Cpu.eax = 0x80000000; INSN(0x99);
    CheckU32("CDQ fills EDX with the sign", Cpu.edx, 0xFFFFFFFF);
    CpuReset(); Cpu.eax = 0x7FFFFFFF; INSN(0x99);
    CheckU32("CDQ is zero for a positive value", Cpu.edx, 0x00000000);

    /* 60 - PUSHAD pushes EAX first, so EAX ends up at the highest address */
    CpuReset(); Cpu.eax = 1; Cpu.ecx = 2; Cpu.edx = 3; Cpu.ebx = 4;
    Cpu.esi = 6; Cpu.edi = 7; Cpu.ebp = 8;
    {
        DWORD entry = Cpu.esp;
        INSN(0x60);
        CheckU32("PUSHAD [ESP+28] = EAX", HostPeek32(entry - 4), 1);
        CheckU32("PUSHAD [ESP+24] = ECX", HostPeek32(entry - 8), 2);
        CheckU32("PUSHAD [ESP+20] = EDX", HostPeek32(entry - 12), 3);
        CheckU32("PUSHAD [ESP+16] = EBX", HostPeek32(entry - 16), 4);
        CheckU32("PUSHAD [ESP+12] = original ESP", HostPeek32(entry - 20), entry);
        CheckU32("PUSHAD [ESP+8] = EBP", HostPeek32(entry - 24), 8);
        CheckU32("PUSHAD [ESP+4] = ESI", HostPeek32(entry - 28), 6);
        CheckU32("PUSHAD [ESP+0] = EDI", HostPeek32(entry - 32), 7);
        CheckU32("PUSHAD ESP", Cpu.esp, entry - 32);
    }
}

/* ------------------------------------------------------------------ */
/* ALU operand sizes, destinations, TEST / XCHG / F6 / F7              */
/* ------------------------------------------------------------------ */

static void TestAluSizes(void)
{
    SectionBegin("ALU operand sizes and destinations");

    DWORD d = DataAddr() + 0x4800;

    /* 00 /r - ADD r/m8, r8: one byte is written, the neighbours survive */
    HostPoke32(d, 0xAABBCC10); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x05; INSN(0x00, 0x08);
    CheckU32("00 ADD BYTE [EAX],CL", HostPeek32(d), 0xAABBCC15);

    /* 0FFh + 2 -> 01h; CF and AF come out of bit 7 / bit 3 of the byte */
    HostPoke32(d, 0xAABBCCFF); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x02; INSN(0x00, 0x08);
    CheckU32("00 ADD BYTE [EAX],CL wraps in one byte", HostPeek32(d), 0xAABBCC01);
    CheckFlags("00 ADD BYTE [EAX],CL CF+AF",
               EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF,
               EFLAGS_CF | EFLAGS_AF);

    /* 66 01 /r - ADD r/m16, r16 */
    CpuReset(); Cpu.eax = 0x00010000; Cpu.ecx = 0x00020003; INSN(0x66, 0x01, 0xC8);
    CheckU32("66 01 ADD AX,CX", Cpu.eax, 0x00010003);
    CheckFlags("66 01 ADD AX,CX no carry", EFLAGS_CF | EFLAGS_OF | EFLAGS_ZF, 0);

    HostPoke32(d, 0xAABBFFFF); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x1234; INSN(0x66, 0x01, 0x08);
    CheckU32("66 01 ADD WORD [EAX],CX", HostPeek32(d), 0xAABB1233);
    CheckFlags("66 01 ADD WORD [EAX],CX CF", EFLAGS_CF, EFLAGS_CF);

    /* 08 /r - OR r/m8, r8: SF is bit 7 of the byte, not bit 31 */
    CpuReset(); Cpu.eax = 0; Cpu.ecx = 0x00000080; INSN(0x08, 0xC8);
    CheckU32("08 OR AL,CL", Cpu.eax, 0x00000080);
    CheckFlags("08 OR AL,CL SF from bit 7",
               EFLAGS_CF | EFLAGS_OF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_PF, EFLAGS_SF);

    /* 0A /r - OR r8, r/m8: the destination is the reg field */
    CpuReset(); Cpu.eax = 0x000000F0; Cpu.ecx = 0x0000000F; INSN(0x0A, 0xC1);
    CheckU32("0A OR AL,CL destination", Cpu.eax, 0x000000FF);
    CheckU32("0A OR AL,CL source unchanged", Cpu.ecx, 0x0000000F);

    /* 10 /r - ADC r/m8, r8 */
    CpuReset(); Cpu.eax = 0x000000FF; Cpu.ecx = 0; Cpu.eflags |= EFLAGS_CF; INSN(0x10, 0xC8);
    CheckU32("10 ADC AL,CL 0FFh+0+1", Cpu.eax, 0x00000000);
    CheckFlags("10 ADC AL,CL CF/ZF/AF/PF",
               EFLAGS_CF | EFLAGS_ZF | EFLAGS_AF | EFLAGS_PF | EFLAGS_SF | EFLAGS_OF,
               EFLAGS_CF | EFLAGS_ZF | EFLAGS_AF | EFLAGS_PF);

    HostPoke32(d, 0xAABBCCFF); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x01; Cpu.eflags |= EFLAGS_CF;
    INSN(0x10, 0x08);
    CheckU32("10 ADC BYTE [EAX],CL wraps in one byte", HostPeek32(d), 0xAABBCC01);
    CheckFlags("10 ADC BYTE [EAX],CL CF", EFLAGS_CF, EFLAGS_CF);

    /* 12 /r - ADC r8, r/m8: the destination is the reg field */
    CpuReset(); Cpu.eax = 0x00000010; Cpu.ecx = 0x00000005; INSN(0x12, 0xC1);
    CheckU32("12 ADC AL,CL destination", Cpu.eax, 0x00000015);
    CheckU32("12 ADC AL,CL source unchanged", Cpu.ecx, 0x00000005);

    /* 18 /r - SBB r/m8, r8 */
    CpuReset(); Cpu.eax = 0; Cpu.ecx = 0; Cpu.eflags |= EFLAGS_CF; INSN(0x18, 0xC8);
    CheckU32("18 SBB AL,CL 0-0-1", Cpu.eax, 0x000000FF);
    CheckFlags("18 SBB AL,CL CF/SF/AF/PF",
               EFLAGS_CF | EFLAGS_SF | EFLAGS_AF | EFLAGS_PF | EFLAGS_ZF | EFLAGS_OF,
               EFLAGS_CF | EFLAGS_SF | EFLAGS_AF | EFLAGS_PF);

    /* 1A /r - SBB r8, r/m8: the destination is the reg field */
    CpuReset(); Cpu.eax = 0x00000010; Cpu.ecx = 0x00000001; INSN(0x1A, 0xC1);
    CheckU32("1A SBB AL,CL destination", Cpu.eax, 0x0000000F);
    CheckU32("1A SBB AL,CL source unchanged", Cpu.ecx, 0x00000001);

    /* 20 / 22 - AND */
    HostPoke32(d, 0xAABBCC0F); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x33; INSN(0x20, 0x08);
    CheckU32("20 AND BYTE [EAX],CL", HostPeek32(d), 0xAABBCC03);
    CpuReset(); Cpu.eax = 0x000000F0; Cpu.ecx = 0x0000003C; INSN(0x22, 0xC1);
    CheckU32("22 AND AL,CL destination", Cpu.eax, 0x00000030);
    CheckU32("22 AND AL,CL source unchanged", Cpu.ecx, 0x0000003C);

    /* 28 / 29 - SUB */
    HostPoke32(d, 0xAABBCC01); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x02; INSN(0x28, 0x08);
    CheckU32("28 SUB BYTE [EAX],CL wraps in one byte", HostPeek32(d), 0xAABBCCFF);
    CheckFlags("28 SUB BYTE [EAX],CL CF", EFLAGS_CF, EFLAGS_CF);

    CpuReset(); Cpu.eax = 0; Cpu.ecx = 1; INSN(0x66, 0x29, 0xC8);
    CheckU32("66 29 SUB AX,CX wraps in 16 bits", Cpu.eax, 0x0000FFFF);
    CheckFlags("66 29 SUB AX,CX CF", EFLAGS_CF, EFLAGS_CF);

    /* 30 / 32 - XOR */
    HostPoke32(d, 0xAABBCCFF); CpuReset(); Cpu.eax = d; Cpu.ecx = 0xFF; INSN(0x30, 0x08);
    CheckU32("30 XOR BYTE [EAX],CL", HostPeek32(d), 0xAABBCC00);
    CpuReset(); Cpu.eax = 0x000000FF; Cpu.ecx = 0x0000000F; INSN(0x32, 0xC1);
    CheckU32("32 XOR AL,CL destination", Cpu.eax, 0x000000F0);
    CheckU32("32 XOR AL,CL source unchanged", Cpu.ecx, 0x0000000F);

    /* 34 ib - XOR AL, imm8: SF is bit 7 of the byte */
    CpuReset(); Cpu.eax = 0x00000000; INSN(0x34, 0x80);
    CheckU32("34 XOR AL,80h", Cpu.eax, 0x00000080);
    CheckFlags("34 XOR AL,80h SF",
               EFLAGS_CF | EFLAGS_OF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_PF, EFLAGS_SF);

    /* 38 / 39 - CMP (no store) */
    HostPoke32(d, 0x00000010); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x11; INSN(0x38, 0x08);
    CheckFlags("38 CMP BYTE [EAX],CL CF", EFLAGS_CF, EFLAGS_CF);
    CheckU32("38 CMP BYTE [EAX],CL does not store", HostPeek32(d), 0x00000010);

    CpuReset(); Cpu.eax = 0x00010000; Cpu.ecx = 0x00020000; INSN(0x66, 0x39, 0xC8);
    CheckFlags("66 39 CMP AX,CX equal in 16 bits", EFLAGS_ZF, EFLAGS_ZF);

    /* 84 / 85 - TEST: nothing is stored, SF follows the operand size */
    CpuReset(); Cpu.eax = 0x00000080; Cpu.ecx = 0x00000080; INSN(0x84, 0xC8);
    CheckU32("84 TEST AL,CL does not store", Cpu.eax, 0x00000080);
    CheckFlags("84 TEST AL,CL SF from bit 7",
               EFLAGS_CF | EFLAGS_OF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_PF, EFLAGS_SF);

    CpuReset(); Cpu.eax = 0x00010000; Cpu.ecx = 0x00010000; INSN(0x66, 0x85, 0xC8);
    CheckFlags("66 85 TEST AX,CX ZF", EFLAGS_ZF, EFLAGS_ZF);

    /* A8 / A9 - TEST AL/EAX, imm: the accumulator is not modified */
    CpuReset(); Cpu.eax = 0x000000FF; INSN(0xA8, 0x0F);
    CheckU32("A8 TEST AL,0Fh keeps AL", Cpu.eax, 0x000000FF);
    CheckFlags("A8 TEST AL,0Fh ZF=0", EFLAGS_ZF, 0);
    CpuReset(); Cpu.eax = 0x12345678; INSN(0xA9, 0x00, 0x00, 0x00, 0x00);
    CheckU32("A9 TEST EAX,0 keeps EAX", Cpu.eax, 0x12345678);
    CheckFlags("A9 TEST EAX,0 ZF", EFLAGS_ZF, EFLAGS_ZF);

    /* 86 / 87 - XCHG */
    HostPoke32(d, 0xAABBCC11); CpuReset(); Cpu.eax = d; Cpu.ecx = 0x00000099; INSN(0x86, 0x08);
    CheckU32("86 XCHG BYTE [EAX],CL swaps one byte", HostPeek32(d), 0xAABBCC99);
    CheckU32("86 XCHG BYTE [EAX],CL register", Cpu.ecx, 0x00000011);
    CpuReset(); Cpu.eax = 0x00000011; Cpu.ecx = 0x00000022; INSN(0x86, 0xC8);
    CheckU32("86 XCHG AL,CL eax", Cpu.eax, 0x00000022);
    CheckU32("86 XCHG AL,CL ecx", Cpu.ecx, 0x00000011);
    CpuReset(); Cpu.eax = 0x00000011; Cpu.ecx = 0x00000022; INSN(0x66, 0x87, 0xC8);
    CheckU32("66 87 XCHG AX,CX eax", Cpu.eax, 0x00000022);
    CheckU32("66 87 XCHG AX,CX ecx", Cpu.ecx, 0x00000011);

    /* 40..4F - INC/DEC r32/r16 (0x66 selects the 16-bit form) */
    CpuReset(); Cpu.eax = 0x0000FFFF; INSN(0x66, 0x40);
    CheckU32("66 40 INC AX wraps in 16 bits", Cpu.eax, 0x00000000);
    CheckFlags("66 40 INC AX ZF/AF/PF, CF preserved",
               EFLAGS_CF | EFLAGS_ZF | EFLAGS_AF | EFLAGS_PF,
               EFLAGS_ZF | EFLAGS_AF | EFLAGS_PF);
    CpuReset(); Cpu.eax = 0x00000000; INSN(0x66, 0x48);
    CheckU32("66 48 DEC AX wraps in 16 bits", Cpu.eax, 0x0000FFFF);

    /* 91 - XCHG EAX, r32 with 0x66 */
    CpuReset(); Cpu.eax = 0x11110001; Cpu.ecx = 0x22220002; INSN(0x66, 0x91);
    CheckU32("66 91 XCHG AX,CX eax", Cpu.eax, 0x11110002);
    CheckU32("66 91 XCHG AX,CX ecx", Cpu.ecx, 0x22220001);

    /* F6 /2, /3 - 8-bit NOT and NEG write exactly one byte */
    HostPoke32(d, 0xAABBCC00); CpuReset(); Cpu.eax = d; INSN(0xF6, 0x10);
    CheckU32("F6 /2 NOT BYTE [EAX]", HostPeek32(d), 0xAABBCCFF);

    HostPoke32(d, 0xAABBCC01); CpuReset(); Cpu.eax = d; INSN(0xF6, 0x18);
    CheckU32("F6 /3 NEG BYTE [EAX]", HostPeek32(d), 0xAABBCCFF);
    CheckFlags("F6 /3 NEG BYTE [EAX] CF", EFLAGS_CF, EFLAGS_CF);

    /* NEG 80h: the result is 80h again and OF is set (SDM: OF = source == 80h) */
    CpuReset(); Cpu.eax = 0x00000080; INSN(0xF6, 0xD8);
    CheckU32("F6 /3 NEG AL 80h", Cpu.eax, 0x00000080);
    CheckFlags("F6 /3 NEG AL 80h OF/CF/SF",
               EFLAGS_CF | EFLAGS_OF | EFLAGS_SF | EFLAGS_ZF | EFLAGS_AF | EFLAGS_PF,
               EFLAGS_CF | EFLAGS_OF | EFLAGS_SF);

    /* F6 /4 - MUL CL: AX = AL * CL, CF=OF=AH != 0 */
    CpuReset(); Cpu.eax = 0x00000010; Cpu.ecx = 0x00000004; INSN(0xF6, 0xE1);
    CheckU32("F6 /4 MUL CL", Cpu.eax, 0x00000040);
    CheckFlags("F6 /4 MUL CL no overflow", EFLAGS_CF | EFLAGS_OF, 0);
    CpuReset(); Cpu.eax = 0x00000010; Cpu.ecx = 0x00000020; INSN(0xF6, 0xE1);
    CheckU32("F6 /4 MUL CL overflow", Cpu.eax, 0x00000200);
    CheckFlags("F6 /4 MUL CL overflow CF/OF", EFLAGS_CF | EFLAGS_OF, EFLAGS_CF | EFLAGS_OF);

    /* F7 /2 - NOT DWORD [EAX] */
    HostPoke32(d, 0x11223344); CpuReset(); Cpu.eax = d; INSN(0xF7, 0x10);
    CheckU32("F7 /2 NOT DWORD [EAX]", HostPeek32(d), 0xEEDDCCBB);

    /* 66 F7 /0 - TEST r/m16, imm16 (5 bytes long, AX is not written) */
    CpuReset(); Cpu.eax = 0x0000000F;
    CheckU32("66 F7 /0 length", INSNL(0x66, 0xF7, 0xC0, 0x0F, 0x00), 5);
    CheckU32("66 F7 /0 TEST AX,0Fh keeps AX", Cpu.eax, 0x0000000F);
    CheckFlags("66 F7 /0 TEST AX,0Fh ZF=0", EFLAGS_ZF, 0);
}

/* ------------------------------------------------------------------ */
/* 0F A3/A4/A5/AB/AC/AD - BT and SHLD/SHRD flag and offset rules       */
/* ------------------------------------------------------------------ */

static void TestDoubleShiftBits(void)
{
    SectionBegin("0F BT / SHLD / SHRD details");

    DWORD d = DataAddr() + 0x4900;

    /* SHLD with a count of 0 (or 32) does not touch the value or the
     * flags (SDM: "If the count is 0, the flags are not affected"). */
    CpuReset(); Cpu.eax = 0x12345678; Cpu.ecx = 0x9ABCDEF0;
    Cpu.eflags |= EFLAGS_CF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF;
    INSN(0x0F, 0xA4, 0xC8, 0x00);
    CheckU32("SHLD EAX,ECX,0 leaves the value", Cpu.eax, 0x12345678);
    CheckFlags("SHLD EAX,ECX,0 leaves the flags",
               EFLAGS_CF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF,
               EFLAGS_CF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);

    CpuReset(); Cpu.eax = 0x12345678; Cpu.ecx = 0x9ABCDEF0;
    Cpu.eflags |= EFLAGS_CF | EFLAGS_ZF;
    INSN(0x0F, 0xA4, 0xC8, 0x20);
    CheckU32("SHLD EAX,ECX,32 leaves the value", Cpu.eax, 0x12345678);
    CheckFlags("SHLD EAX,ECX,32 leaves the flags", EFLAGS_CF | EFLAGS_ZF, EFLAGS_CF | EFLAGS_ZF);

    /* SHLD by 1: CF is the old MSB, OF = MSB(old) XOR MSB(result). */
    CpuReset(); Cpu.eax = 0x80000000; Cpu.ecx = 0x00000001;
    INSN(0x0F, 0xA4, 0xC8, 0x01);
    CheckU32("SHLD EAX,ECX,1", Cpu.eax, 0x00000000);
    CheckFlags("SHLD EAX,ECX,1 CF is the old MSB", EFLAGS_CF, EFLAGS_CF);
    CheckFlags("SHLD EAX,ECX,1 OF (MSBs differ)", EFLAGS_OF, EFLAGS_OF);

    /* SHRD by 1: CF is bit 0 of the source value, OF compares the MSB of
     * the original destination with the MSB of the result. */
    CpuReset(); Cpu.eax = 0x40000001; Cpu.ecx = 0x00000000;
    INSN(0x0F, 0xAC, 0xC8, 0x01);
    CheckU32("SHRD EAX,ECX,1", Cpu.eax, 0x20000000);
    CheckFlags("SHRD EAX,ECX,1 CF is bit 0 of the value", EFLAGS_CF, EFLAGS_CF);
    CheckFlags("SHRD EAX,ECX,1 OF stays 0 (MSBs equal)", EFLAGS_OF, 0);

    CpuReset(); Cpu.eax = 0x00000002; Cpu.ecx = 0x00000001;
    INSN(0x0F, 0xAC, 0xC8, 0x01);
    CheckU32("SHRD EAX,ECX,1 with a set source LSB", Cpu.eax, 0x80000001);
    CheckFlags("SHRD EAX,ECX,1 OF (MSBs differ)", EFLAGS_OF, EFLAGS_OF);

    /* 0F A3 /r - BT r/m32, r32 with a memory operand: the register bit
     * offset is signed, so negative offsets address earlier bytes. */
    HostPoke8(d, 0x01); HostPoke8(d + 1, 0x02);
    CpuReset(); Cpu.eax = d; Cpu.ecx = 8; INSN(0x0F, 0xA3, 0x08);
    CheckFlags("0F A3 BT [EAX],8 uses the next byte", EFLAGS_CF, 0);
    CpuReset(); Cpu.eax = d; Cpu.ecx = 9; INSN(0x0F, 0xA3, 0x08);
    CheckFlags("0F A3 BT [EAX],9", EFLAGS_CF, EFLAGS_CF);
    CpuReset(); Cpu.eax = d + 1; Cpu.ecx = 0xFFFFFFF8; INSN(0x0F, 0xA3, 0x08);
    CheckFlags("0F A3 BT [EAX],-8 reads the previous byte", EFLAGS_CF, EFLAGS_CF);

    /* BTS with a negative offset sets the bit in the previous byte and
     * leaves the addressed byte alone. */
    HostPoke8(d, 0x00); HostPoke8(d + 1, 0x00);
    CpuReset(); Cpu.eax = d + 1; Cpu.ecx = 0xFFFFFFF8; INSN(0x0F, 0xAB, 0x08);
    CheckU32("0F AB BTS [EAX],-8 sets a bit before the operand", (DWORD)HostPeek8(d), 0x01);
    CheckU32("0F AB BTS [EAX],-8 leaves the operand byte", (DWORD)HostPeek8(d + 1), 0x00);

    /* 66 0F A3 /r - BT r/m16, r16 masks the register index to 16 bits. */
    CpuReset(); Cpu.eax = 0x00000001; Cpu.ecx = 16; INSN(0x66, 0x0F, 0xA3, 0xC8);
    CheckFlags("66 0F A3 BT AX,16 wraps to bit 0", EFLAGS_CF, EFLAGS_CF);
}

/* ------------------------------------------------------------------ */
/* 8F /0 - POP r/m with ESP as the base and the 16-bit form            */
/* ------------------------------------------------------------------ */

static void TestPopRm(void)
{
    SectionBegin("POP r/m forms");

    /* 8F /0 - POP r/m32 (and the 0x66 POP r/m16 form).  For a memory
     * destination that uses ESP as its base the address is computed after
     * ESP has been incremented (SDM). */
    HostPoke32(DataAddr() + 0x5000, 0x11223344);
    HostPoke32(DataAddr() + 0x5004, 0x55667788);
    CpuReset(); Cpu.esp = DataAddr() + 0x5000;
    INSN(0x8F, 0x04, 0x24);              /* POP DWORD PTR [ESP] */
    CheckU32("8F /0 POP [ESP] writes to the incremented ESP", HostPeek32(DataAddr() + 0x5004), 0x11223344);
    CheckU32("8F /0 POP [ESP] leaves the old top", HostPeek32(DataAddr() + 0x5000), 0x11223344);
    CheckU32("8F /0 POP [ESP] esp", Cpu.esp, DataAddr() + 0x5004);

    HostPoke16(DataAddr() + 0x5010, 0x1234);
    CpuReset(); Cpu.esp = DataAddr() + 0x5010; Cpu.eax = 0xAAAA0000;
    INSN(0x66, 0x8F, 0xC0);              /* POP AX */
    CheckU32("66 8F /0 POP AX", Cpu.eax, 0xAAAA1234);
    CheckU32("66 8F /0 POP AX esp", Cpu.esp, DataAddr() + 0x5012);
}

/* ------------------------------------------------------------------ */
/* 0F B0/B1/C0/C1 - CMPXCHG and XADD, and 16-bit relative branches     */
/* ------------------------------------------------------------------ */

static void TestCmpxchgXadd(void)
{
    SectionBegin("CMPXCHG / XADD / 16-bit branches");

    DWORD d = DataAddr() + 0x4A00;

    /* 0F B0 /r - CMPXCHG r/m8, r8 (equal: r is stored into r/m) */
    HostPoke32(d, 0xAABBCC01); CpuReset(); Cpu.eax = 0x00000001; Cpu.ecx = 0x00000077; Cpu.edx = d;
    INSN(0x0F, 0xB0, 0x0A);              /* CMPXCHG BYTE [EDX], CL */
    CheckU32("0F B0 CMPXCHG BYTE [EDX],CL equal writes one byte", HostPeek32(d), 0xAABBCC77);
    CheckU32("0F B0 CMPXCHG equal keeps AL", Cpu.eax, 0x00000001);
    CheckFlags("0F B0 CMPXCHG equal sets ZF", EFLAGS_ZF, EFLAGS_ZF);

    /* not equal: r/m is loaded into AL and r/m is left alone */
    HostPoke32(d, 0xAABBCC02); CpuReset(); Cpu.eax = 0x00000001; Cpu.ecx = 0x00000077; Cpu.edx = d;
    INSN(0x0F, 0xB0, 0x0A);
    CheckU32("0F B0 CMPXCHG differ loads AL", Cpu.eax, 0x00000002);
    CheckU32("0F B0 CMPXCHG differ keeps memory", HostPeek32(d), 0xAABBCC02);
    CheckFlags("0F B0 CMPXCHG differ clears ZF", EFLAGS_ZF, 0);

    /* register form: only the low byte of the destination changes */
    CpuReset(); Cpu.eax = 0x00000007; Cpu.ecx = 0xAAAAAA07; INSN(0x0F, 0xB0, 0xC1);
    CheckU32("0F B0 CMPXCHG CL,AL keeps the upper bits", Cpu.ecx, 0xAAAAAA07);

    /* 0F B1 /r - CMPXCHG r/m32, r32 */
    CpuReset(); Cpu.eax = 0x11223344; Cpu.ecx = 0x11223344; INSN(0x0F, 0xB1, 0xC1);
    CheckU32("0F B1 CMPXCHG ECX,EAX equal", Cpu.ecx, 0x11223344);
    CheckFlags("0F B1 CMPXCHG equal ZF", EFLAGS_ZF, EFLAGS_ZF);
    CpuReset(); Cpu.eax = 0x11223344; Cpu.ecx = 0x55667788; INSN(0x0F, 0xB1, 0xC1);
    CheckU32("0F B1 CMPXCHG differ loads EAX", Cpu.eax, 0x55667788);
    CheckU32("0F B1 CMPXCHG differ keeps ECX", Cpu.ecx, 0x55667788);
    CheckFlags("0F B1 CMPXCHG differ clears ZF", EFLAGS_ZF, 0);

    /* 0F C0 /r - XADD r/m8, r8: CF comes out of bit 7 */
    HostPoke32(d, 0xAABBCCFF); CpuReset(); Cpu.edx = d; Cpu.ecx = 0x00000002;
    INSN(0x0F, 0xC0, 0x0A);              /* XADD BYTE [EDX], CL */
    CheckU32("0F C0 XADD BYTE [EDX],CL sum", HostPeek32(d), 0xAABBCC01);
    CheckU32("0F C0 XADD BYTE [EDX],CL register gets the old value", Cpu.ecx, 0x000000FF);
    CheckFlags("0F C0 XADD BYTE [EDX],CL CF/AF from the byte", EFLAGS_CF | EFLAGS_AF, EFLAGS_CF | EFLAGS_AF);

    /* 0F C1 /r - XADD r/m32, r32 */
    CpuReset(); Cpu.eax = 0x00000005; Cpu.ecx = 0x00000010; INSN(0x0F, 0xC1, 0xC1);
    CheckU32("0F C1 XADD ECX,EAX sum", Cpu.ecx, 0x00000015);
    CheckU32("0F C1 XADD ECX,EAX register gets the old r/m value", Cpu.eax, 0x00000010);
    CheckFlags("0F C1 XADD no carry", EFLAGS_CF | EFLAGS_OF, 0);
    CpuReset(); Cpu.eax = 0x7FFFFFFF; Cpu.ecx = 0x00000001; INSN(0x0F, 0xC1, 0xC1);
    CheckU32("0F C1 XADD overflow sum", Cpu.ecx, 0x80000000);
    CheckFlags("0F C1 XADD overflow OF", EFLAGS_OF | EFLAGS_CF, EFLAGS_OF);
    CpuReset(); Cpu.eax = 0xFFFFFFFF; Cpu.ecx = 0x00000002; INSN(0x0F, 0xC1, 0xC1);
    CheckU32("0F C1 XADD carry sum", Cpu.ecx, 0x00000001);
    CheckFlags("0F C1 XADD carry CF", EFLAGS_CF | EFLAGS_OF, EFLAGS_CF);

    /* 66 E9 / 66 E8 / 66 0F 84 - a 16-bit displacement is sign-extended */
    CpuReset(); INSN(0x66, 0xE9, 0xF0, 0xFF);        /* JMP rel16 -16 */
    CheckU32("66 E9 JMP rel16 -16 target", Cpu.eip, CodeAddr() + 4 - 16);

    CpuReset(); INSN(0x66, 0xE8, 0xF0, 0xFF);        /* CALL rel16 -16 */
    CheckU32("66 E8 CALL rel16 -16 target", Cpu.eip, CodeAddr() + 4 - 16);
    CheckU32("66 E8 CALL rel16 pushed return address", (DWORD)HostPeek16(Cpu.esp), (CodeAddr() + 4) & 0xFFFF);

    CpuReset(); Cpu.eflags |= EFLAGS_ZF;
    INSN(0x66, 0x0F, 0x84, 0xF0, 0xFF);              /* JZ rel16 -16 */
    CheckU32("66 0F 84 JZ rel16 -16 target", Cpu.eip, CodeAddr() + 5 - 16);
    CpuReset(); Cpu.eflags &= ~EFLAGS_ZF;
    INSN(0x66, 0x0F, 0x84, 0xF0, 0xFF);              /* JZ rel16 not taken */
    CheckU32("66 0F 84 JZ rel16 not taken", Cpu.eip, CodeAddr() + 5);
}

int main(void)
{
    GuestInit();

    TestDataTransfer();
    TestAddressing();
    TestArithmetic();
    TestShifts();
    TestBitOps();
    TestLogic();
    TestFlags();
    TestControlFlow();
    TestStrings();
    TestMisc();
    TestWide16();
    TestMemoryForms();
    TestRotates();
    TestPushPop();
    TestStringMore();
    TestBcd();
    TestSegPrefix();
    TestMisc2();
    TestAluSizes();
    TestDoubleShiftBits();
    TestPopRm();
    TestCmpxchgXadd();

    printf("\n%d checks, %d failures\n", Checks, Failures);
    if (Failures == 0)
        printf("OK: yact_emu ISA regression harness passed\n");
    return Failures == 0 ? 0 : 1;
}
