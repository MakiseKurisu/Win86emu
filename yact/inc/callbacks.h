/* 
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * Emulator support callbacks (x86 to native, etc)
 *
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CB_EXPORT
#define CB_EXPORT
#endif

typedef unsigned int DW;

// Return address of callback "stop emulation and return to host"
CB_EXPORT DW CbReturnToHost(void);

// Check for address inside "return to host" callback
CB_EXPORT bool CbIsReturnToHost(DW Addr);

// Call the given func when current thread exits (by ExitThread for example), returns Param or NULL on error
CB_EXPORT void* CbCallAtThreadExit(void* Func, void*Param);	

// Remove the callback
CB_EXPORT bool CbRemoveCallAtThreadExit(void* Func, void*Param);

// Called by TerminateThread 
CB_EXPORT void CbNotifyThreadExit(DW ThrId);

// Allocate memory and initialize native callback to native stdcall func taking NumParams (used to emulate DlgProc and alike)
// Return 0 if error
// The callbacks are not freed and already allocated callbacks are returned automatically
CB_EXPORT DW CbCreateNativeCdeclCallbackEx(void *NativeFunc, DW Param);
CB_EXPORT DW CbCreateNativeStdcallCallback(DW Param, int NumParams);
CB_EXPORT DW CbCreateNativeStdcallCallbackEx(void *NativeFunc, DW FuncParam, int NumParams);
CB_EXPORT DW CbCreateNativeCdeclCallback(DW Param);

// Allocate memory and initialize callback from x86 to native stdcall func taking NumParams
// Return 0 if error
CB_EXPORT DW CbCreateX86StdcallCallback(void *NativeFunc, int NumParams);
CB_EXPORT DW CbCreateX86CdeclCallback(void *NativeFunc);

#ifdef __cplusplus
}
#endif

#endif // __EMUL_H
