#include "dllyact_def.h"
#include <windows.h>
#include <util.h>
#include "callbacks.h"
#include "x86emul.h"

//DEFINE_FUNC1(GetOpenFileNameA)
EXTERN_C DW STUB_EXPORT yact_GetOpenFileNameA(DW *R)
{
	BOOL Ret;
	LPOPENFILENAMEA lpo=(LPOPENFILENAMEA)p1;
	DWORD Hook=0;
	if(lpo->Flags&OFN_ENABLEHOOK)
	{
		Hook=CbCreateNativeStdcallCallback((DWORD)lpo->lpfnHook,4);
		lpo->lpfnHook=(LPOFNHOOKPROC)Hook;
	}
	Ret=GetOpenFileNameA(lpo);
//	if(Hook)
//		CbFreeCallback(Hook);
	return Ret;
}

//DEFINE_FUNC1(GetOpenFileNameW)
EXTERN_C DW STUB_EXPORT yact_GetOpenFileNameW(DW *R)
{
	BOOL Ret;
	LPOPENFILENAMEW lpo=(LPOPENFILENAMEW)p1;
	DWORD Hook=0;
	if(lpo->Flags&OFN_ENABLEHOOK)
	{
		Hook=CbCreateNativeStdcallCallback((DWORD)lpo->lpfnHook,4);
		lpo->lpfnHook=(LPOFNHOOKPROC)Hook;
	}
	Ret=GetOpenFileNameW(lpo);
//	if(Hook)
//		CbFreeCallback(Hook);
	return Ret;
}

//DEFINE_FUNC1(GetSaveFileNameA)
//DEFINE_FUNC1(GetSaveFileNameW)
EXTERN_C DW STUB_EXPORT yact_GetSaveFileNameA(DW *R)
{
	BOOL Ret;
	LPOPENFILENAMEA lpo=(LPOPENFILENAMEA)p1;
	DWORD Hook=0;
	if(lpo->Flags&OFN_ENABLEHOOK)
	{
		Hook=CbCreateNativeStdcallCallback((DWORD)lpo->lpfnHook,4);
		lpo->lpfnHook=(LPOFNHOOKPROC)Hook;
	}
	Ret=GetSaveFileNameA(lpo);
//	if(Hook)
//		CbFreeCallback(Hook);
	return Ret;
}

//DEFINE_FUNC1(GetOpenFileNameW)
EXTERN_C DW STUB_EXPORT yact_GetSaveFileNameW(DW *R)
{
	BOOL Ret;
	LPOPENFILENAMEW lpo=(LPOPENFILENAMEW)p1;
	DWORD Hook=0;
	if(lpo->Flags&OFN_ENABLEHOOK)
	{
		Hook=CbCreateNativeStdcallCallback((DWORD)lpo->lpfnHook,4);
		lpo->lpfnHook=(LPOFNHOOKPROC)Hook;
	}
	Ret=GetSaveFileNameW(lpo);
//	if(Hook)
//		CbFreeCallback(Hook);
	return Ret;
}
