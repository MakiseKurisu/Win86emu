/*
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * PE file loader internal functions
 *
 */

#ifndef __I_PELDR_H
#define __I_PELDR_H

#include <peldr.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct PeFile
    {
        DWORD Signature;		// 'FILE'
        HANDLE hFile; 			// File handle
        LPCWSTR FileName;
        BOOL IsNative;			// TRUE if native DLL, in this case hFile, AllocatedMem, etc are not used
        BOOL IsExe;				// This file is EXE, not a DLL
        BOOL IsStub;			// This DLL is a stub DLL (filename ends with .86.dll)
        BOOL IsGUI;				// True == file is GUI, false == console. Only valid if IsExe==true
        BOOL AllocatedMem;  	// Memory for image (Base) was allocated by us
        void *Base;				// Loaded image base (0 if not loaded yet)
        DWORD Size;				// Module size
        TLS_CALLBACK *Cb;		// This callback is called to specify functions that should be called during process/thread creation
        LPVOID CbParam;			// Callback parameter	
        void *TlsTemplateStart;	// used for TLS
        DWORD TlsTemplateLen;
        DWORD TlsZeroFillLen;
        LPDWORD TlsIndex;
        int ModuleTlsIndex;		// TLS index for this module
        int PeLdrTlsIndex;		// TLS index for pe loader internal data about thread and this module
    } *PE_HANDLE;

    PE_EXPORT DWORD PeLdrCalcModuleRamSize(PE_HANDLE);
    PE_EXPORT BOOL PeLdrFixupModule(PE_HANDLE Pe);
    PE_EXPORT BOOL PeLdrProcessModuleImports(PE_HANDLE Pe);
    PE_EXPORT int PeLdrStartProgram(LPWSTR ExePath);

#ifdef __cplusplus
}
#endif

#endif // __PELDR_H
