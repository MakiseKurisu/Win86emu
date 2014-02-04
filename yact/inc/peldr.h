/* 
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * PE file loader public functions
 *
 * All functions are unicode. In case of error they call SetLastError()
 *
 */

#ifndef __PELDR_H
#define __PELDR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PE_EXPORT
#define PE_EXPORT
#endif

struct PeFile;
typedef struct PeFile *PE_HANDLE;

typedef void TLS_CALLBACK(PE_HANDLE Pe, PIMAGE_TLS_CALLBACK Ptr, DWORD Reason, LPVOID Param);

// Callback returns address of x86 function implementing the given import, HMODULE or NULL
// if Pe==NULL, checks for DLL presence (return HMODULE==module base), if (DWORD)FuncName<65536 -> ordinal
typedef void* IMP_CALLBACK(PE_HANDLE Pe, LPCSTR DllName, LPCSTR FuncName, LPVOID Param);

PE_EXPORT PE_HANDLE PeLdrLoadModule(LPCWSTR FileName);
PE_EXPORT PE_HANDLE PeLdrLoadModuleA(LPCSTR FileName);
PE_EXPORT PE_HANDLE PeLdrFindModule(LPCWSTR FileName);
PE_EXPORT PE_HANDLE PeLdrFindModuleA(LPCSTR FileName);
PE_EXPORT PE_HANDLE PeLdrFindModuleByBase(DWORD Base);
PE_EXPORT void PeLdrCloseAllModules();
PE_EXPORT DWORD PeLdrGetPageSize();
PE_EXPORT TLS_CALLBACK *PeLdrSetExecutionCallback(TLS_CALLBACK *Cb, LPVOID Param);
PE_EXPORT IMP_CALLBACK *PeLdrSetImportCallback(IMP_CALLBACK *Cb, LPVOID Param);
PE_EXPORT BOOL PeLdrNotifyNewThread(PE_HANDLE Pe, DWORD Reason);	// If PE==0, notify all modules
PE_EXPORT DWORD PeLdrGetEntryPoint(PE_HANDLE Pe);
PE_EXPORT DWORD PeLdrGetModuleBase(PE_HANDLE Pe);
PE_EXPORT DWORD PeLdrGetModuleFileNameA(PE_HANDLE Pe, LPSTR FileName, DWORD Size);
PE_EXPORT DWORD PeLdrGetModuleFileName(PE_HANDLE Pe, LPWSTR FileName, DWORD Size);
PE_EXPORT DWORD PeLdrGetFixedLoadAddress(PE_HANDLE Pe);
PE_EXPORT FARPROC PeLdrGetProcAddressA(PE_HANDLE Dll, LPCSTR Name);
PE_EXPORT FARPROC PeLdrGetProcAddress(PE_HANDLE Dll, LPCWSTR Name);
PE_EXPORT LPSTR PeLdrGetSystemDirectoryA();
PE_EXPORT LPWSTR PeLdrGetSystemDirectoryW();
struct _TEB;
PE_EXPORT struct _TEB *PeLdrGetCurrentTeb(void);
PE_EXPORT int PeLdrIsValidX86(LPCWSTR ExePath);	// return 0 = not valid, 1 = GUI, -1 = console

#ifdef __cplusplus
}
#endif

#endif // __PELDR_H
