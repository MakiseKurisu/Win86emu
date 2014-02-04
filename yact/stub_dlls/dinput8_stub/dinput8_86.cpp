#define CINTERFACE	
#include "dllstub_def.h"
#include <windows.h>
#include <stdio.h>
#include <util.h>
#include <intrin.h>
#include <dinput.h>

HMODULE DInput=0;
#define DIAPI extern "C" HRESULT __declspec(dllexport) __stdcall

typedef HRESULT __stdcall t_DI_CreateDevice(IDirectInputW* _this, REFGUID guid,LPDIRECTINPUTDEVICE *out,LPUNKNOWN unk);
extern t_DI_CreateDevice *org_DI_CreateDevice;
HRESULT __stdcall DI_CreateDevice(IDirectInputW* _this, REFGUID guid,LPDIRECTINPUTDEVICE *out,LPUNKNOWN unk);

typedef HRESULT __stdcall t_DirectInput8Create(
         HINSTANCE hinst,
         DWORD dwVersion,
         REFIID riidltf,
         LPVOID * ppvOut,
         LPUNKNOWN punkOuter);
DIAPI stub_DirectInput8Create(
         HINSTANCE hinst,
         DWORD dwVersion,
         REFIID riidltf,
         LPVOID * ppvOut,
         LPUNKNOWN punkOuter
)
{
	LogWarn("DirectInput8Create: REFIID {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             riidltf.Data1, riidltf.Data2, riidltf.Data3,
             riidltf.Data4[0], riidltf.Data4[1], riidltf.Data4[2], riidltf.Data4[3],
             riidltf.Data4[4], riidltf.Data4[5], riidltf.Data4[6], riidltf.Data4[7]);

	if(DInput==0)
		DInput=LoadLibraryA("DInput8_org.dll");

	t_DirectInput8Create *dic=(t_DirectInput8Create*)GetProcAddress(DInput,"DirectInput8Create");
	HRESULT hr=dic(hinst,dwVersion,riidltf,ppvOut,punkOuter);

	LPDIRECTINPUT8 din=(LPDIRECTINPUT8)*ppvOut;
	if(din)
	{
		org_DI_CreateDevice=(t_DI_CreateDevice*)din->lpVtbl->CreateDevice;
		*(void**)&din->lpVtbl->CreateDevice=DI_CreateDevice;
	}
	return hr;
}

DIAPI stub_Ord_1(
         HINSTANCE hinst,
         DWORD dwVersion,
         REFIID riidltf,
         LPVOID * ppvOut,
         LPUNKNOWN punkOuter
)
{
	return stub_DirectInput8Create(
         hinst,
         dwVersion,
         riidltf,
         ppvOut,
         punkOuter);
}

DIAPI stub_DllCanUnloadNow()
{
//	LogInfo("Dinput8::DllCanUnloadNow");
	if(DInput==0)
		DInput=LoadLibraryA("DInput8_org.dll");
	return GetProcAddress(DInput,"DllCanUnloadNow")();
}

DIAPI stub_DllRegisterServer()
{
//	LogInfo("Dinput8::DllRegisterServer");
	if(DInput==0)
		DInput=LoadLibraryA("DInput8_org.dll");
	return GetProcAddress(DInput,"DllRegisterServer")();
}

DIAPI stub_DllUnregisterServer()
{
//	LogInfo("Dinput8::DllUnregisterServer");
	if(DInput==0)
		DInput=LoadLibraryA("DInput8_org.dll");
	return GetProcAddress(DInput,"DllUnregisterServer")();
}

typedef HRESULT __stdcall t_DllGetClassObject(const CLSID& clsid, const IID& iid, void **ppv);
DIAPI stub_DllGetClassObject(const CLSID& clsid, const IID& iid, void **ppv)
{
	LogWarn("Dinput8: Querying CLSID {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             clsid.Data1, clsid.Data2, clsid.Data3,
             clsid.Data4[0], clsid.Data4[1], clsid.Data4[2], clsid.Data4[3],
             clsid.Data4[4], clsid.Data4[5], clsid.Data4[6], clsid.Data4[7]);
	LogWarn("Dinput8: Querying IID {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             iid.Data1, iid.Data2, iid.Data3,
             iid.Data4[0], iid.Data4[1], iid.Data4[2], iid.Data4[3],
             iid.Data4[4], iid.Data4[5], iid.Data4[6], iid.Data4[7]);

	if(DInput==0)
		DInput=LoadLibraryA("DInput8_org.dll");

	t_DllGetClassObject *gco=(t_DllGetClassObject*)GetProcAddress(DInput,"DllGetClassObject");
	HRESULT hr=gco(clsid, iid, ppv);
	if(SUCCEEDED(hr) && (clsid==CLSID_DirectInput8 || clsid==CLSID_DirectInput))
	{
		LPDIRECTINPUT8 din=(LPDIRECTINPUT8)*ppv;
		if(din)
		{
			org_DI_CreateDevice=(t_DI_CreateDevice*)din->lpVtbl->CreateDevice;
			*(void**)&din->lpVtbl->CreateDevice=DI_CreateDevice;
		}
	}
	return gco(clsid, iid, ppv);
}
