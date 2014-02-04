#include "dllyact_def.h"
#include <windows.h>
#include <util.h>
#include "peldr.h"
#include "callbacks.h"
#include "x86emul.h"

struct __EXCEPTION_REGISTRATION_RECORD {
    struct __EXCEPTION_REGISTRATION_RECORD *Next;
    void* Handler;
};

LONG WINAPI YactUnhandledExceptionFilter(
  _In_  struct _EXCEPTION_POINTERS *ExceptionInfo
)
{
#ifdef _ARM_
	LogWarn("Unhandled exception: %08X, PC=%08X, LR=%08X",ExceptionInfo->ExceptionRecord->ExceptionCode,ExceptionInfo->ContextRecord->Pc,ExceptionInfo->ContextRecord->Lr);
#else
	LogWarn("Unhandled exception: %08X",ExceptionInfo->ExceptionRecord->ExceptionCode);
#endif
	PeLdrNotifyNewThread(NULL,DLL_THREAD_DETACH);
	ExitThread(ExceptionInfo->ExceptionRecord->ExceptionCode);
}

static DWORD WINAPI ThreadProc(
  LPVOID lpParameter	// [0] == orig func, [1] == orig param
)
{
	__EXCEPTION_REGISTRATION_RECORD R;
	DWORD *Parm=(DWORD*)lpParameter;
	DWORD *TEB=(DWORD*)PeLdrGetCurrentTeb();
	R.Next=(__EXCEPTION_REGISTRATION_RECORD*)-1;
	R.Handler=(void*)CbReturnToHost();
	TEB[0]=(DWORD)&R;	// in case of unhandled exception - just return 
	PeLdrNotifyNewThread(NULL,DLL_THREAD_ATTACH);

	DWORD Ret=0;
	DWORD parm0=Parm[0];
	DWORD parm1=Parm[1];
	delete Parm;

	int Affinity=CompatGetInt("SetProcessAffinityMask");
	if(Affinity)
		SetThreadAffinityMask(GetCurrentThread(),Affinity);	// we set the affinity of all emulator's threads - so that our own or OS threads may still run on diffirent cores

	SetUnhandledExceptionFilter(YactUnhandledExceptionFilter);
	LogInfo("Thread created: %08X\n",GetCurrentThreadId());
	__try {
		Ret=EmuExecute(parm0,1,parm1);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		Ret=0xDEADC0DE;
	}

	PeLdrNotifyNewThread(NULL,DLL_THREAD_DETACH);
	CbNotifyThreadExit(GetCurrentThreadId());
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_CreateThread(DW *R)
{	
	DWORD* Parm=new DWORD[2];
	Parm[0]=p3;
	Parm[1]=p4;
	DWORD StackSize=0;	
	LogInfo("CreateThread(%08x, %08x, %08x, %08x, %08x, %08x)",p1,p2,p3,p4,p5,p6);
	DWORD t=(DWORD)CreateThread((LPSECURITY_ATTRIBUTES)p1,StackSize,ThreadProc,Parm,p5,(LPDWORD)p6);
	return t;
}

EXTERN_C DW STUB_EXPORT yact_ExitThread(DW *R)
{	
	DWORD Ret=p1;	// keep copy of parameter as emulated stack would be freed
	PeLdrNotifyNewThread(NULL,DLL_THREAD_DETACH);
	CbNotifyThreadExit(GetCurrentThreadId());
	ExitThread(Ret);
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_RtlUnwind(DW *R)	// never called - implemented in x86 part
{	
	PVOID TargetFrame=(PVOID)p1;
	PVOID TargetIp=(PVOID)p2;
	PEXCEPTION_RECORD ExceptionRecord=(PEXCEPTION_RECORD)p3;
	PVOID ReturnValue=(PVOID)p4;
	LogErr("Native RtlUnwind called instead of emulated!!!");

//	RtlUnwind(TargetFrame,TargetIp,ExceptionRecord,ReturnValue);
	return (DWORD)ReturnValue;
}

EXTERN_C DW STUB_EXPORT yact_GetModuleFileNameW(DW *R)
{
	BOOL Ret=FALSE;
	PE_HANDLE Pe=PeLdrFindModuleByBase(p1);
	if(Pe==0)
		goto Exit;

	Ret=PeLdrGetModuleFileName(Pe,(LPWSTR)p2,p3);

Exit:
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_GetModuleFileNameA(DW *R)
{
	BOOL Ret=FALSE;
	PE_HANDLE Pe=PeLdrFindModuleByBase(p1);
	if(Pe==0)
		goto Exit;

	Ret=PeLdrGetModuleFileNameA(Pe,(LPSTR)p2,p3);

Exit:
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_GetModuleHandleA(DW *R)
{
	char *dll=(char*)p1;
	PE_HANDLE Pe=PeLdrFindModuleA(dll);
	return (DW)PeLdrGetModuleBase(Pe);
}

EXTERN_C DW STUB_EXPORT yact_GetModuleHandleExA(DW *R)
{
	char *dll=(char*)p2;
	PE_HANDLE Pe=PeLdrFindModuleA(dll);
	*(DW*)p3=(DW)PeLdrGetModuleBase(Pe);
	return *(DW*)p3;
}

EXTERN_C DW STUB_EXPORT yact_GetModuleHandleExW(DW *R)
{
	wchar_t *dll=(wchar_t*)p2;
	PE_HANDLE Pe=PeLdrFindModule(dll);
	*(DW*)p3=(DW)PeLdrGetModuleBase(Pe);
	return *(DW*)p3;
}

EXTERN_C DW STUB_EXPORT yact_GetModuleHandleW(DW *R)
{
	wchar_t *dll=(wchar_t*)p1;
	PE_HANDLE Pe=PeLdrFindModule(dll);
	return (DW)PeLdrGetModuleBase(Pe);
}

EXTERN_C DW STUB_EXPORT yact_LoadLibraryA(DW *R)
{
	char *dll=(char*)p1;
	PE_HANDLE Pe=PeLdrLoadModuleA(dll);
	return (DW)PeLdrGetModuleBase(Pe);
}

EXTERN_C DW STUB_EXPORT yact_LoadLibraryExA(DW *R)	// implement it as an ordinary LoadLibrary
{
	char *dll=(char*)p1;
	PE_HANDLE Pe=PeLdrLoadModuleA(dll);
	return (DW)PeLdrGetModuleBase(Pe);
}


EXTERN_C DW STUB_EXPORT yact_LoadLibraryW(DW *R)
{
	wchar_t *dll=(wchar_t*)p1;
	PE_HANDLE Pe=PeLdrLoadModule(dll);
	return (DW)PeLdrGetModuleBase(Pe);
}

EXTERN_C DW STUB_EXPORT yact_LoadLibraryExW(DW *R)	// implement it as an ordinary LoadLibrary
{
	wchar_t *dll=(wchar_t*)p1;
	PE_HANDLE Pe=PeLdrLoadModule(dll);
	return (DW)PeLdrGetModuleBase(Pe);
}
//DEFINE_FUNC3(LoadLibraryExW)
//DEFINE_FUNC1(LoadLibraryW)

EXTERN_C DW STUB_EXPORT yact_GetProcAddress(DW *R)		
{	
	char *func=(char*)p2;
	PE_HANDLE Pe=PeLdrFindModuleByBase(p1);
	DW r=(DW)PeLdrGetProcAddressA(Pe,func);	
	return r;
}

EXTERN_C void L_Lock();
EXTERN_C void L_Unlock();

__declspec(thread) int OldPriority1=NORMAL_PRIORITY_CLASS;
__declspec(thread) int OldPriority2=NORMAL_PRIORITY_CLASS;
__declspec(thread) DWORD_PTR OldAffinity1=-1;
__declspec(thread) DWORD_PTR OldAffinity2=-1;

//DEFINE_FUNC1(SuspendThread)
EXTERN_C DW STUB_EXPORT yact_SuspendThread(DW *R)		// hack for the idiotic usage of SuspendThread in MSS32.dll. It can suspend the main thread inside C runtime function that holds some lock
{														// and the same lock may be tried to be obtained during emulation of the calling thread -> deadlock.
	HANDLE Thr=(HANDLE)p1;								// So lower the priority of the suspended thread to min, raise current to max and force them to run on the same CPU. In this case Windows would allow 
#if 1
	DWORD Id=GetThreadId(Thr);							// a "suspended" thread to execute if the current thread waits for the lock hold by it. Otherwise the "suspended" thread should be stalled.
	OldPriority1=GetThreadPriority(Thr);
	OldPriority2=GetThreadPriority(GetCurrentThread());
	OldAffinity1=SetThreadAffinityMask(Thr,1);
	OldAffinity2=SetThreadAffinityMask(GetCurrentThread(),1);
	SetThreadPriority(Thr,THREAD_PRIORITY_IDLE);
	SetThreadPriorityBoost(Thr,FALSE);
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
//	Sleep(0);
//	L_Lock();
	DWORD r=SuspendThread(Thr);
	ResumeThread(Thr);
//	L_Unlock();
#else
//	L_Lock();
	HeapLock(GetProcessHeap());		// avoid deadlock in the default heap (I know about loader lock and alike, lets hope that heap is the only deadlock that is caused by idiotic MSS32 behavior)
	DWORD r=SuspendThread(Thr);
	HeapUnlock(GetProcessHeap());
//	L_Unlock();
#endif
	return r;
}

//DEFINE_FUNC1(ResumeThread)
EXTERN_C DW STUB_EXPORT yact_ResumeThread(DW *R)		
{	
	HANDLE Thr=(HANDLE)p1;
#if 1
	DWORD Id=GetThreadId(Thr);
	SetThreadPriority(Thr,OldPriority1);
	SetThreadPriority(GetCurrentThread(),OldPriority2);
	SetThreadAffinityMask(Thr,OldAffinity1);
	SetThreadAffinityMask(GetCurrentThread(),OldAffinity2);
//	L_Lock();
	DWORD r=SuspendThread(Thr);
	r=ResumeThread(Thr);
//	L_Unlock();
#else
//	L_Lock();
	DWORD r=ResumeThread(Thr);
//	L_Unlock();
#endif
	return r;
}

//DEFINE_FUNC1(FlsAlloc)
EXTERN_C DW STUB_EXPORT yact_FlsAlloc(DW *R)		
{	
	return FlsAlloc(NULL);	// TODO: FLS Callbacks are not implemented!
}

// On ARM Interlocked functions are not exported by kernel32 (they are inlined)
//DEFINE_FUNC5(InterlockedCompareExchange64)
// PROBLEM: high 32 bits are lost by stub code
EXTERN_C LONGLONG STUB_EXPORT yact_InterlockedCompareExchange64(DW *R)		
{	
	LogErr("InterlockedCompareExchange64 called, lost top 32 bits!!!");
	return InterlockedCompareExchange64((LONGLONG*)p1,(p2|((LONGLONG)p3<<32LL)),(p4|((LONGLONG)p5<<32LL)));
}

//DEFINE_FUNC3(InterlockedCompareExchange)
EXTERN_C DW STUB_EXPORT yact_InterlockedCompareExchange(DW *R)		
{	
	return InterlockedCompareExchange((LONG*)p1,p2,p3);
}

//DEFINE_FUNC1(InterlockedDecrement)
EXTERN_C DW STUB_EXPORT yact_InterlockedDecrement(DW *R)		
{	
	return InterlockedDecrement((LONG*)p1);
}

//DEFINE_FUNC2(InterlockedExchange)
EXTERN_C DW STUB_EXPORT yact_InterlockedExchange(DW *R)		
{	
	return InterlockedExchange((LONG*)p1,p2);
}

//DEFINE_FUNC2(InterlockedExchangeAdd)
EXTERN_C DW STUB_EXPORT yact_InterlockedExchangeAdd(DW *R)		
{	
	return InterlockedExchangeAdd((LONG*)p1,p2);
}

//DEFINE_FUNC1(InterlockedIncrement)
EXTERN_C DW STUB_EXPORT yact_InterlockedIncrement(DW *R)		
{	
	return InterlockedIncrement((LONG*)p1);
}

//DEFINE_FUNC4(RaiseException)
EXTERN_C DW STUB_EXPORT yact_RaiseException(DW *R)
{	
	DWORD dwExceptionCode=p1;
	DWORD dwExceptionFlags=p2;
	DWORD nNumberOfArguments=p3;
	const ULONG_PTR* lpArguments=(ULONG_PTR*)p4;
	RaiseException(dwExceptionCode,dwExceptionFlags,nNumberOfArguments,lpArguments);	// RaiseException is implemented in stub DLL, so this is never called!
	return 0;
}

//DEFINE_FUNC1(EnterCriticalSection)
EXTERN_C DW STUB_EXPORT yact_EnterCriticalSection(DW *R)
{
	if(p1>1024)
		EnterCriticalSection((LPCRITICAL_SECTION)p1);
	return 0;
}

EXTERN_C DW STUB_EXPORT yact_LeaveCriticalSection(DW *R)
{
	if(p1>1024)
		LeaveCriticalSection((LPCRITICAL_SECTION)p1);
	return 0;
}

struct IOE_Ctx
{
	DWORD Func;
	DWORD OrgP;
};

BOOL CALLBACK IOE_Func(
  _Inout_      PINIT_ONCE InitOnce,
  _Inout_opt_  PVOID Parameter,
  _Out_opt_    PVOID *Context
)
{
	IOE_Ctx *Ctx=(IOE_Ctx*)Parameter;
	DWORD Ret=EmuExecute(Ctx->Func,3,(DWORD)InitOnce,(DWORD)Ctx->OrgP,(DWORD)Context);
	return Ret;
}

//DEFINE_FUNC4(InitOnceExecuteOnce)
EXTERN_C DW STUB_EXPORT yact_InitOnceExecuteOnce(DW *R)
{
	IOE_Ctx MyCtx;
	MyCtx.Func=p2;
	MyCtx.OrgP=p3;
	DWORD DW=InitOnceExecuteOnce((PINIT_ONCE)p1,IOE_Func,&MyCtx,(void**)p4);
	return DW;
}

EXTERN_C DW STUB_EXPORT yact_IsProcessorFeaturePresent(DW *R)
{
	if(p1>CompatGetInt("MaxProcessorFeaturePresent")||p1==0)
		return 0;
	return 1;
}

EXTERN_C DW STUB_EXPORT yact_LocalReAlloc(DW *R)
{
	HLOCAL hMem=(HLOCAL)p1;
	DWORD NewSize=p2;
	DWORD Flags=p3;
	HLOCAL Ret=LocalReAlloc(hMem,NewSize,Flags);
#if 0
	if(Ret==0 && LocalSize(hMem)>=NewSize)		// bug in WinRT - it sometimes fails reallocs that try to shrink memory blocks
		Ret=hMem;
#endif
	return (DWORD)Ret;
}

EXTERN_C DW STUB_EXPORT yact_HeapCreate(DW *R)
{
	return (DWORD)HeapCreate((p1)&(~HEAP_NO_SERIALIZE),p2,p3);
}

//DEFINE_FUNC3(HeapFree)
EXTERN_C DW STUB_EXPORT yact_HeapFree(DW *R)
{
	return HeapFree(HANDLE (p1),0,(void*)p3);
}

EXTERN_C DW STUB_EXPORT yact_IsDebuggerPresent(DW *R)
{
	return FALSE;
}

// have to manually implement functions that take 64 bit structs as parameters - as these parameters should be a aligned on even registers (i.e. R1 is not used)
EXTERN_C DW STUB_EXPORT yact_SetFilePointerEx(DW *R)
{
	HANDLE hFile=(HANDLE)p1;
	LARGE_INTEGER liDistanceToMove;		
	liDistanceToMove.LowPart=p2;
	liDistanceToMove.HighPart=p3;
	PLARGE_INTEGER lpNewFilePointer=(PLARGE_INTEGER)p4;
	DWORD dwMoveMethod=p5;
	return SetFilePointerEx(hFile,liDistanceToMove,lpNewFilePointer,dwMoveMethod);
}

//DEFINE_FUNC2(TerminateThread)
EXTERN_C DW STUB_EXPORT yact_TerminateThread(DW *R)
{
	HANDLE HThr=(HANDLE)p1;
	DWORD Code=p2;
	LogInfo("TerminateThread(%08x%d)",GetThreadId(HThr),Code);

	if(GetThreadId(HThr)==GetCurrentThreadId())
		CbNotifyThreadExit(GetThreadId(HThr));	// notify current thread trermination before real termination
	
	BOOL r=TerminateThread(HThr,Code);	// terminate then notify
	CbNotifyThreadExit(GetThreadId(HThr));
	return r;
}

EXTERN_C DW STUB_EXPORT yact_GetSystemDirectoryA(DW *R)
{
	char *Ptr=(char*)p1;
	DWORD Ret=GetModuleFileNameA(0,Ptr,p2);
	if(Ret>p2 || Ret==0)
		return Ret;
	char *P=strrchr(Ptr,'\\');
	if(P)
		*P=0;
	strcat_s(Ptr,p2,"\\System32");
	return strlen(Ptr);
}

EXTERN_C DW STUB_EXPORT yact_GetSystemDirectoryW(DW *R)
{
	wchar_t *Ptr=(wchar_t*)p1;
	DWORD Ret=GetModuleFileNameW(0,Ptr,p2);
	if(Ret>p2 || Ret==0)
		return Ret;
	wchar_t *P=wcsrchr(Ptr,'\\');
	if(P)
		*P=0;
	wcscat_s(Ptr,p2,L"\\System32");
	return wcslen(Ptr);
}

EXTERN_C DW STUB_EXPORT yact_GetWindowsDirectoryA(DW *R)
{
	char *Ptr=(char*)p1;
	DWORD Ret=GetModuleFileNameA(0,Ptr,p2);
	if(Ret>p2 || Ret==0)
		return Ret;
	char *P=strrchr(Ptr,'\\');
	if(P)
		*P=0;
	return strlen(Ptr);
}

EXTERN_C DW STUB_EXPORT yact_GetWindowsDirectoryW(DW *R)
{
	wchar_t *Ptr=(wchar_t*)p1;
	DWORD Ret=GetModuleFileNameW(0,Ptr,p2);
	if(Ret>p2 || Ret==0)
		return Ret;
	wchar_t *P=wcsrchr(Ptr,'\\');
	if(P)
		*P=0;
	return wcslen(Ptr);
}

EXTERN_C DW STUB_EXPORT yact_GetSystemWindowsDirectoryA(DW *R)
{
	return yact_GetWindowsDirectoryA(R);
}

EXTERN_C DW STUB_EXPORT yact_GetSystemWindowsDirectoryW(DW *R)
{
	return yact_GetWindowsDirectoryW(R);
}

//DEFINE_FUNC4(VirtualAlloc)
EXTERN_C DW STUB_EXPORT yact_VirtualAlloc(DW *R)
{
	LPVOID Ret=VirtualAlloc((void*)p1,p2,p3,p4);
	if(Ret==0)
	{
		if(p3&MEM_LARGE_PAGES) // "A required privilege is not held by the client" error for large pages
			Ret=VirtualAlloc((void*)p1,p2,p3&(~MEM_LARGE_PAGES),p4);
		if(Ret==0)
			LogErr("VirtualAlloc(%08x,%d,%08x,%08x) failed %d",p1,p2,p3,p4,GetLastError());
	}
	return (DW)Ret;
}

//DEFINE_FUNC0(GetVersion)
EXTERN_C DW STUB_EXPORT yact_GetVersion(DW *R)
{
	DWORD Ret=CompatGetInt("OSVersionHi")|(CompatGetInt("OSVersionLo")<<8)|((CompatGetInt("OSVersionBuild")&0x7fff)<<16);
	if(Ret==0)
		return GetVersion();
	else 
		return Ret;
}
//DEFINE_FUNC1(GetVersionExA)
EXTERN_C DW STUB_EXPORT yact_GetVersionExA(DW *R)
{
	OSVERSIONINFOEXA *vi=(OSVERSIONINFOEXA*)p1;
	BOOL Ret=GetVersionExA((LPOSVERSIONINFOA)vi);
	if(Ret==FALSE)
		return FALSE;
	int Tmp=CompatGetInt("OSVersionHi");
	if(Tmp)
	{
		vi->dwMajorVersion=Tmp;
		vi->dwMinorVersion=CompatGetInt("OSVersionLo");
		Tmp=CompatGetInt("OSVersionBuild");
		if(Tmp)
			vi->dwBuildNumber=Tmp;	
	}
	if(vi->dwOSVersionInfoSize>=sizeof(OSVERSIONINFOEXA))
	{
		Tmp=CompatGetInt("OSServicepackHi");
		if(Tmp)
		{
			if(vi->dwOSVersionInfoSize>=sizeof(OSVERSIONINFOEXA))
			{
				vi->wServicePackMajor=Tmp;
				vi->wServicePackMinor=CompatGetInt("OSServicepackLo");
			}
			wsprintfA(vi->szCSDVersion,"Service Pack %d",Tmp);
		} 
	}
	return TRUE;
}
//DEFINE_FUNC1(GetVersionExW)
EXTERN_C DW STUB_EXPORT yact_GetVersionExW(DW *R)
{
	OSVERSIONINFOEXW *vi=(OSVERSIONINFOEXW*)p1;
	BOOL Ret=GetVersionExW((LPOSVERSIONINFOW)vi);
	if(Ret==FALSE)
		return FALSE;
	int Tmp=CompatGetInt("OSVersionHi");
	if(Tmp)
	{
		vi->dwMajorVersion=Tmp;
		vi->dwMinorVersion=CompatGetInt("OSVersionLo");
		Tmp=CompatGetInt("OSVersionBuild");
		if(Tmp)
			vi->dwBuildNumber=Tmp;	
	}
	if(vi->dwOSVersionInfoSize>=sizeof(OSVERSIONINFOEXW))
	{
		Tmp=CompatGetInt("OSServicepackHi");
		if(Tmp)
		{
			if(vi->dwOSVersionInfoSize>=sizeof(OSVERSIONINFOEXA))
			{
				vi->wServicePackMajor=Tmp;
				vi->wServicePackMinor=CompatGetInt("OSServicepackLo");
			}
			wsprintfW(vi->szCSDVersion,L"Service Pack %d",Tmp);
		}
	}
	return TRUE;
}

DW FixResHandle(DW Handle)
{
	if(Handle==0)
	{
		PE_HANDLE Pe=PeLdrFindModuleA(0);
		Handle = (DW)PeLdrGetModuleBase(Pe);
	}
	return Handle;
}

//DEFINE_FUNC1(InitializeCriticalSection)
EXTERN_C DW STUB_EXPORT yact_InitializeCriticalSection(DW *R)
{
	LPCRITICAL_SECTION cs=(LPCRITICAL_SECTION)p1;
	DW Ret=InitializeCriticalSectionEx(cs,0,RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#ifndef _ARM_		// desktop W7/8 does not support RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO flag
	if(Ret==0)
		Ret=InitializeCriticalSectionEx(cs,0,0);
#endif
	return Ret;
}

//DEFINE_FUNC2(InitializeCriticalSectionAndSpinCount)
EXTERN_C DW STUB_EXPORT yact_InitializeCriticalSectionAndSpinCount(DW *R)
{
	LPCRITICAL_SECTION cs=(LPCRITICAL_SECTION)p1;
	DW Ret=InitializeCriticalSectionEx(cs,p2,RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#ifndef _ARM_
	if(Ret==0)
		Ret=InitializeCriticalSectionEx(cs,p2,0);
#endif
	return Ret;
}

//DEFINE_FUNC3(InitializeCriticalSectionEx)
EXTERN_C DW STUB_EXPORT yact_InitializeCriticalSectionEx(DW *R)
{
	LPCRITICAL_SECTION cs=(LPCRITICAL_SECTION)p1;
	DW Ret=InitializeCriticalSectionEx(cs,p2,RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO|(p3&~CRITICAL_SECTION_NO_DEBUG_INFO));
#ifndef _ARM_
	if(Ret==0)
		Ret=InitializeCriticalSectionEx(cs,p2,(p3&~CRITICAL_SECTION_NO_DEBUG_INFO));
#endif
	return Ret;
}
