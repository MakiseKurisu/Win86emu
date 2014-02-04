#include "dllyact_def.h"
#include <windows.h>
#include <util.h>
#include <Shlwapi.h>
#include <classes.h>
#include <callbacks.h>
#include <x86emul.h>

//DEFINE_FUNC1(DllGetVersion)
EXTERN_C DW STUB_EXPORT yact_DllGetVersion(DW *R)
{
	HRESULT hr=E_NOTIMPL;
	DLLVERSIONINFO2 *dv=(DLLVERSIONINFO2*)p1;
	DLLGETVERSIONPROC pDllGetVersion;
	pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(GetModuleHandleA("comctl32.dll"), "DllGetVersion");
	if(pDllGetVersion)
		hr = (*pDllGetVersion)((DLLVERSIONINFO*)dv);
	return hr;
}

//DEFINE_FUNC1(PropertySheetA)
EXTERN_C DW STUB_EXPORT yact_PropertySheetA(DW *R)
{
	PROPSHEETHEADERA *psh=(PROPSHEETHEADERA*)p1;

	if(psh->dwFlags&PSH_PROPSHEETPAGE)
	{
		char *Ptr=(char*)psh->ppsp;
		for(int i=0; i<psh->nPages; i++)
		{
			LPPROPSHEETPAGEA lpp=(LPPROPSHEETPAGEA)Ptr;
			lpp->pfnDlgProc=(DLGPROC)CbCreateNativeStdcallCallback((DWORD)lpp->pfnDlgProc,4);
			if(lpp->dwFlags&PSP_USECALLBACK)
				lpp->pfnCallback=(LPFNPSPCALLBACKA)CbCreateNativeStdcallCallback((DWORD)lpp->pfnCallback,3);
			Ptr+=lpp->dwSize;
		}
	} 

	if(psh->dwFlags&PSH_USECALLBACK)
		psh->pfnCallback=(PFNPROPSHEETCALLBACK)CbCreateNativeStdcallCallback((DWORD)psh->pfnCallback,3);

	return PropertySheetA(psh);
}

EXTERN_C DW STUB_EXPORT yact_PropertySheet(DW *R)
{
	return yact_PropertySheetA(R);
}

//DEFINE_FUNC1(PropertySheetW)
EXTERN_C DW STUB_EXPORT yact_PropertySheetW(DW *R)
{
	PROPSHEETHEADERW *psh=(PROPSHEETHEADERW*)p1;

	if(psh->dwFlags&PSH_PROPSHEETPAGE)	// FIXME: make a copy of psh struct and modify a copy
	{
		char *Ptr=(char*)psh->ppsp;
		for(int i=0; i<psh->nPages; i++)
		{
			LPPROPSHEETPAGEW lpp=(LPPROPSHEETPAGEW)Ptr;
			lpp->pfnDlgProc=(DLGPROC)CbCreateNativeStdcallCallback((DWORD)lpp->pfnDlgProc,4);
			if(lpp->dwFlags&PSP_USECALLBACK)
				lpp->pfnCallback=(LPFNPSPCALLBACK)CbCreateNativeStdcallCallback((DWORD)lpp->pfnCallback,3);
			Ptr+=lpp->dwSize;
		}
	} 

	if(psh->dwFlags&PSH_USECALLBACK)
		psh->pfnCallback=(PFNPROPSHEETCALLBACK)CbCreateNativeStdcallCallback((DWORD)psh->pfnCallback,3);

	return PropertySheetW(psh);
}

//DEFINE_FUNC1(CreatePropertySheetPage)
EXTERN_C DW STUB_EXPORT yact_CreatePropertySheetPage(DW *R)
{
	LPPROPSHEETPAGEA lppsp=(LPPROPSHEETPAGEA)p1;
	lppsp->pfnDlgProc=(DLGPROC)CbCreateNativeStdcallCallback((DWORD)lppsp->pfnDlgProc,4);
	if(lppsp->dwFlags&PSP_USECALLBACK)
		lppsp->pfnCallback=(LPFNPSPCALLBACKA)CbCreateNativeStdcallCallback((DWORD)lppsp->pfnCallback,3);

	return (DWORD)CreatePropertySheetPageA(lppsp);
}

//DEFINE_FUNC1(CreatePropertySheetPageA)
EXTERN_C DW STUB_EXPORT yact_CreatePropertySheetPageA(DW *R)
{
	LPPROPSHEETPAGEA lppsp=(LPPROPSHEETPAGEA)p1;
	lppsp->pfnDlgProc=(DLGPROC)CbCreateNativeStdcallCallback((DWORD)lppsp->pfnDlgProc,4);
	if(lppsp->dwFlags&PSP_USECALLBACK)
		lppsp->pfnCallback=(LPFNPSPCALLBACKA)CbCreateNativeStdcallCallback((DWORD)lppsp->pfnCallback,3);

	return (DWORD)CreatePropertySheetPageA(lppsp);
}

//DEFINE_FUNC1(CreatePropertySheetPageW)
EXTERN_C DW STUB_EXPORT yact_CreatePropertySheetPageW(DW *R)
{
	LPPROPSHEETPAGEW lppsp=(LPPROPSHEETPAGEW)p1;
	lppsp->pfnDlgProc=(DLGPROC)CbCreateNativeStdcallCallback((DWORD)lppsp->pfnDlgProc,4);
	if(lppsp->dwFlags&PSP_USECALLBACK)
		lppsp->pfnCallback=(LPFNPSPCALLBACKW)CbCreateNativeStdcallCallback((DWORD)lppsp->pfnCallback,3);

	return (DWORD)CreatePropertySheetPageW(lppsp);
}

