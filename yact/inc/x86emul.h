/*
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * x86 32bit emulator public functions
 *
 */

#ifndef __EMUL_H
#define __EMUL_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EMU_EXPORT
#define EMU_EXPORT
#endif

#define EMU_STACK_SIZE (1024*1024*2)		// reserve this amount for emulated code


    // Initialize engine. Return true == success.
    EMU_EXPORT BOOL EmuInitialize(void);

    // Emulate code starting from Addr, passing NParams on stack (like stdcall/cdecl).
    // NParams is the number of 32-bit arguments; the contract is 0..8. EmuExecute()
    // in x86_emu.cpp (which defines MAXARGS = 8) rejects anything outside that
    // range before reading an argument, so the dosbox/bochs engine exports of the
    // same name are only ever called with 0..8.
    EMU_EXPORT DWORD EmuExecute(DWORD Addr, int NParams, ...);

    // Get and set FPU control word for the current emulator thread (needed to keep the same value between bochs and dosbox cores)
    EMU_EXPORT int GetFPUCW();
    EMU_EXPORT void SetFPUCW(int cw);


#define L_Lock()		// Lock dosbox CPU single-threaded emulator
#define L_Unlock()		// Unlock

#ifdef __cplusplus
}
#endif

/*
 * CPU State & EFLAGS Definitions (Phase 4)
 */
#ifndef _CPU_STATE_DEFINED
#define _CPU_STATE_DEFINED

#define EFLAGS_CF  (1 << 0)
#define EFLAGS_PF  (1 << 2)
#define EFLAGS_AF  (1 << 4)
#define EFLAGS_ZF  (1 << 6)
#define EFLAGS_SF  (1 << 7)
#define EFLAGS_TF  (1 << 8)
#define EFLAGS_IF  (1 << 9)
#define EFLAGS_DF  (1 << 10)
#define EFLAGS_OF  (1 << 11)

typedef struct _CPU_STATE {
    DWORD eax, ecx, edx, ebx, esp, ebp, esi, edi;
    DWORD eip;
    DWORD eflags;
    WORD cs, ds, es, ss, fs, gs;
} CPU_STATE, *PCPU_STATE;

/*
 * CalcParity() returns non-zero when the low byte of v has an EVEN number of
 * set bits, i.e. exactly when the x86 PF flag must be set.
 *
 * The 0x6996 nibble lookup gives the ODD-parity result, so it is inverted.
 * (Before this fix PF was set for odd parity, which is the opposite of the
 * architectural definition, and every flag-setting instruction inherited
 * the error.)
 */
static inline DWORD CalcParity(DWORD v) {
    DWORD b = v & 0xFF;
    b ^= (b >> 4);
    b &= 0x0F;
    return ((0x6996 >> b) & 1) ^ 1;
}

static inline void UpdateFlags_Add(PCPU_STATE cpu, DWORD res, DWORD op1, DWORD op2) {
    cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
    if (res == 0) cpu->eflags |= EFLAGS_ZF;
    if (res & 0x80000000) cpu->eflags |= EFLAGS_SF;
    if (CalcParity(res)) cpu->eflags |= EFLAGS_PF;
    if (res < op1) cpu->eflags |= EFLAGS_CF;
    if (((op1 & 0xF) + (op2 & 0xF)) > 0xF) cpu->eflags |= EFLAGS_AF;
    if (!((op1 ^ op2) & 0x80000000) && ((op1 ^ res) & 0x80000000)) cpu->eflags |= EFLAGS_OF;
}

static inline void UpdateFlags_Sub(PCPU_STATE cpu, DWORD res, DWORD op1, DWORD op2) {
    cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
    if ((op1 & 0xF) < (op2 & 0xF)) cpu->eflags |= EFLAGS_AF;
    if (res == 0) cpu->eflags |= EFLAGS_ZF;
    if (res & 0x80000000) cpu->eflags |= EFLAGS_SF;
    if (CalcParity(res)) cpu->eflags |= EFLAGS_PF;
    if (op1 < op2) cpu->eflags |= EFLAGS_CF;
    if (((op1 ^ op2) & 0x80000000) && ((op1 ^ res) & 0x80000000)) cpu->eflags |= EFLAGS_OF;
}

static inline void UpdateFlags_Logical(PCPU_STATE cpu, DWORD res) {
    cpu->eflags &= ~(EFLAGS_CF | EFLAGS_OF | EFLAGS_AF | EFLAGS_PF | EFLAGS_ZF | EFLAGS_SF);
    if (res == 0) cpu->eflags |= EFLAGS_ZF;
    if (res & 0x80000000) cpu->eflags |= EFLAGS_SF;
    if (CalcParity(res)) cpu->eflags |= EFLAGS_PF;
}

static inline void UpdateFlags_Adc(PCPU_STATE cpu, DWORD res, DWORD op1, DWORD op2, BOOL carry_in) {
    cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
    if (res == 0) cpu->eflags |= EFLAGS_ZF;
    if (res & 0x80000000) cpu->eflags |= EFLAGS_SF;
    if (CalcParity(res)) cpu->eflags |= EFLAGS_PF;
    // CF: unsigned overflow including carry
    if (carry_in && res < op1) cpu->eflags |= EFLAGS_CF;
    else if (!carry_in && res < op1) cpu->eflags |= EFLAGS_CF;
    else if (carry_in && (res == op1)) cpu->eflags |= EFLAGS_CF; // carry caused overflow
    // More accurate CF calculation
    if ((op1 + op2 + (carry_in ? 1 : 0)) < op1) cpu->eflags |= EFLAGS_CF;
    // AF: auxiliary carry
    if (((op1 & 0xF) + (op2 & 0xF) + (carry_in ? 1 : 0)) > 0xF) cpu->eflags |= EFLAGS_AF;
    // OF: signed overflow
    if (!((op1 ^ op2) & 0x80000000) && ((op1 ^ res) & 0x80000000)) cpu->eflags |= EFLAGS_OF;
}

static inline void UpdateFlags_Sbb(PCPU_STATE cpu, DWORD res, DWORD op1, DWORD op2, BOOL borrow_in) {
    cpu->eflags &= ~(EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF);
    if ((op1 & 0xF) < (op2 & 0xF) + (borrow_in ? 1 : 0)) cpu->eflags |= EFLAGS_AF;
    if (res == 0) cpu->eflags |= EFLAGS_ZF;
    if (res & 0x80000000) cpu->eflags |= EFLAGS_SF;
    if (CalcParity(res)) cpu->eflags |= EFLAGS_PF;
    // CF: unsigned borrow including borrow-in
    if (op1 < op2 + (borrow_in ? 1 : 0)) cpu->eflags |= EFLAGS_CF;
    // OF: signed overflow
    if (((op1 ^ op2) & 0x80000000) && ((op1 ^ res) & 0x80000000)) cpu->eflags |= EFLAGS_OF;
}

#endif // _CPU_STATE_DEFINED

#endif // __EMUL_H
