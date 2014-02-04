#include "dllstub_def.h"
#include <windows.h>
#include <stdio.h>
#include <util.h>
#include <intrin.h>
#include <Objbase.h>

//DEFINE_FUNC1(SHGetMalloc)
extern "C" HRESULT __declspec(dllexport) __stdcall stub_SHGetMalloc(IMalloc **ppMalloc)
{
	return CoGetMalloc(MEMCTX_TASK,ppMalloc);	// to avoid double implementation
}

//DEFINE_FUNC5(SHCoCreateInstance)
extern "C" HRESULT __declspec(dllexport) __stdcall stub_SHCoCreateInstance(PCWSTR pszCLSID, const CLSID *pclsid, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	if(pszCLSID)
		LogInfo("Not implemented SHCoCreateInstance CLSID: '%S'",pszCLSID);
	if(pclsid)
		LogInfo("Not implemented SHCoCreateInstance CLSID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             pclsid->Data1, pclsid->Data2, pclsid->Data3,
             pclsid->Data4[0], pclsid->Data4[1], pclsid->Data4[2], pclsid->Data4[3],
             pclsid->Data4[4], pclsid->Data4[5], pclsid->Data4[6], pclsid->Data4[7]);
	LogInfo("Not implemented SHCoCreateInstance IID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             riid.Data1, riid.Data2, riid.Data3,
             riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
             riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
	return E_NOTIMPL;
}
