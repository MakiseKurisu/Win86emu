#include "dllstub_def.h"
#include <windows.h>
#include <stdio.h>
#include <util.h>
#include <ShlGuid.h>
#include <ShlDisp.h>
#include <intrin.h>

#define OLE_EXPORT extern "C" HRESULT __declspec(dllexport) __stdcall
#define OLE_EXPORT_(ret) extern "C" ret __declspec(dllexport) __stdcall

typedef HRESULT __stdcall t_DllGetClassObject(const CLSID& clsid, const IID& iid, void **ppv);

//DEFINE_FUNC2(CoLoadLibrary)
OLE_EXPORT_(HINSTANCE) stub_CoLoadLibrary(
  _In_  LPOLESTR lpszLibName,
  _In_  BOOL bAutoFree
)
{
	return LoadLibraryW(lpszLibName);
}

// To simplify debugging I don't unload COM DLLs
OLE_EXPORT_(void) stub_CoFreeAllLibraries(void)
{
}

OLE_EXPORT_(void) stub_CoFreeLibrary(
  _In_  HINSTANCE hInst
)
{
}

OLE_EXPORT_(void) stub_CoFreeUnusedLibraries(void)
{
}

OLE_EXPORT_(void) stub_CoFreeUnusedLibrariesEx(
  _In_  DWORD dwUnloadDelay,
  _In_  DWORD dwReserved
)
{
}

OLE_EXPORT stub_CoGetClassObject(
  _In_      REFCLSID rclsid,
  _In_      DWORD dwClsContext,
  _In_opt_  VOID *pServerInfo,
  _In_      REFIID riid,
  _Out_     LPVOID *ppv
)
{
	*ppv=0;
	char StrIID[128];
	char StrDll[MAX_PATH];
	DWORD Len=sizeof(StrDll);
	sprintf(StrIID,"CLSID\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\InProcServer32",
             rclsid.Data1, rclsid.Data2, rclsid.Data3,
             rclsid.Data4[0], rclsid.Data4[1], rclsid.Data4[2], rclsid.Data4[3],
             rclsid.Data4[4], rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7]);
	LogInfo("CoGetClassObject: HKCR\\%s",
             StrIID);
	if(RegGetValueA(HKEY_CLASSES_ROOT,StrIID,0,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,NULL,StrDll,&Len)!=ERROR_SUCCESS)
		return REGDB_E_CLASSNOTREG;

	LogInfo("CLSID found in DLL: '%s'",StrDll);

	HINSTANCE HI=LoadLibraryA(StrDll);
	if(HI==0)
	{
		LogErr("CLSID DLL '%s' failed to load, err=%d",StrDll,GetLastError());
		return CO_E_DLLNOTFOUND;
	}

	t_DllGetClassObject *DGCO=(t_DllGetClassObject*)GetProcAddress(HI,"DllGetClassObject");
	if(HI==0)
	{
		LogErr("DllGetClassObject not found in '%s'",StrDll);
		return CO_E_DLLNOTFOUND;
	}

	LogInfo("CoGetClassObject IID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             riid.Data1, riid.Data2, riid.Data3,
             riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
             riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

	return DGCO(rclsid,riid,ppv);
}

extern "C" HRESULT __stdcall stub_CoCreateInstanceNT(
  _In_   REFCLSID rclsid,
  _In_   LPUNKNOWN pUnkOuter,
  _In_   DWORD dwClsContext,
  _In_   REFIID riid,
  _Out_  LPVOID *ppv
);

//DEFINE_FUNC5(CoCreateInstance)
OLE_EXPORT stub_CoCreateInstance(
  _In_   REFCLSID rclsid,
  _In_   LPUNKNOWN pUnkOuter,
  _In_   DWORD dwClsContext,
  _In_   REFIID riid,
  _Out_  LPVOID *ppv
)
{
	IClassFactory *pCF=0;
	LogInfo("CoCreateInstance CLSID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             rclsid.Data1, rclsid.Data2, rclsid.Data3,
             rclsid.Data4[0], rclsid.Data4[1], rclsid.Data4[2], rclsid.Data4[3],
             rclsid.Data4[4], rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7]);
	LogInfo("CoCreateInstance IID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             riid.Data1, riid.Data2, riid.Data3,
             riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
             riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
	HRESULT Ret=CoGetClassObject(rclsid, dwClsContext, NULL, IID_IClassFactory, (void**)&pCF); 
	if(pCF==0 && Ret==S_OK)
		Ret=E_FAIL;
	if(Ret==S_OK)
	{
		Ret = pCF->CreateInstance(pUnkOuter, riid, ppv);
		pCF->Release(); 
	}
	if(Ret!=S_OK)
		return stub_CoCreateInstanceNT(rclsid,pUnkOuter,dwClsContext,riid,ppv);
	return Ret;
}

OLE_EXPORT stub_CoCreateInstanceEx(
  _In_     REFCLSID rclsid,
  _In_     IUnknown *punkOuter,
  _In_     DWORD dwClsCtx,
  _In_     COSERVERINFO *pServerInfo,
  _In_     DWORD dwCount,
  _Inout_  MULTI_QI *pResults
)
{
	return E_NOINTERFACE;	// TODO
}
