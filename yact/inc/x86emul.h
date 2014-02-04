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

// Emulate code starting from Addr, passing NParams on stack (like stdcall/cdecl)
EMU_EXPORT DWORD EmuExecute(DWORD Addr, int NParams,...);

// Get and set FPU control word for the current emulator thread (needed to keep the same value between bochs and dosbox cores)
EMU_EXPORT int GetFPUCW();
EMU_EXPORT void SetFPUCW(int cw);


#define L_Lock()		// Lock dosbox CPU single-threaded emulator
#define L_Unlock()		// Unlock

#ifdef __cplusplus
}
#endif

#endif // __EMUL_H
