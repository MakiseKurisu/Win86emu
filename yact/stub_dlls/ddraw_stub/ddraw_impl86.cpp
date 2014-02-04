#include "dllstub_def.h"
#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <util.h>
#include <intrin.h>

#define DDAPI extern "C" HRESULT __declspec(dllexport) __stdcall

class CFactory : public IClassFactory
{
public:
// IUnknown
    virtual HRESULT __stdcall QueryInterface(const IID&, void**);
    virtual ULONG   __stdcall AddRef();
    virtual ULONG   __stdcall Release();

    virtual HRESULT __stdcall CreateInstance(IUnknown* UnkOuter, const IID& iid,void** ppv);
    virtual HRESULT __stdcall LockServer(BOOL Lock);

    CFactory() : cRef(1) {    }
    ~CFactory() {   }

private:
    long cRef;
};

HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{
    if(iid==IID_IUnknown)
    {
        *ppv=static_cast<IUnknown*>(static_cast<IClassFactory*>(this));
    } else if(iid==IID_IClassFactory)
    {
        *ppv=static_cast<IClassFactory*>(this);
    } else
    {
		LogErr("DDRaw::ClassFactory: Tried to get invalid iface");
        *ppv=NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall CFactory::AddRef()
{
    return InterlockedIncrement(&cRef);
}

ULONG __stdcall CFactory::Release()
{
    if(InterlockedDecrement(&cRef)==0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

HRESULT __stdcall CFactory::CreateInstance(IUnknown* UnkOuter, const IID& iid,void** ppv)
{
    if(UnkOuter && iid!=IID_IUnknown)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if(iid==IID_IDirectDraw7)
	{
		return DirectDrawCreateEx(0,ppv,IID_IDirectDraw7,(IUnknown*)0xFFFFFFFF);	// special magic to create DD via COM
	}

    if(iid==IID_IDirectDrawClipper)
	{
		return DirectDrawCreateClipper(0,(LPDIRECTDRAWCLIPPER*)ppv,(IUnknown*)0);	// TODO: untested!!
	}

	LogErr("DDRaw::CreateInstance: Tried to get invalid iface");
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT __stdcall CFactory::LockServer(BOOL Lock)
{
    return S_OK;
}

DDAPI   stub_DllCanUnloadNow()
{
	return S_FALSE;	// don't support unload now
}

DDAPI   stub_DllRegisterServer()
{
    return S_OK;
}

DDAPI   stub_DllUnregisterServer()
{
    return S_OK;
}

DDAPI   stub_DllGetClassObject(const CLSID& clsid, const IID& iid, void **ppv)
{
	if((clsid==CLSID_DirectDraw || clsid==CLSID_DirectDraw7 || clsid==CLSID_DirectDrawClipper) && iid==IID_IClassFactory)
	{
		CFactory *F=new CFactory();

		HRESULT hr=F->QueryInterface(iid, ppv);
		F->Release();

	    return hr;
	}
	LogWarn("Not implemented CLSID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             clsid.Data1, clsid.Data2, clsid.Data3,
             clsid.Data4[0], clsid.Data4[1], clsid.Data4[2], clsid.Data4[3],
             clsid.Data4[4], clsid.Data4[5], clsid.Data4[6], clsid.Data4[7]);
	LogWarn("Not implemented IID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             iid.Data1, iid.Data2, iid.Data3,
             iid.Data4[0], iid.Data4[1], iid.Data4[2], iid.Data4[3],
             iid.Data4[4], iid.Data4[5], iid.Data4[6], iid.Data4[7]);
//    if((clsid!=CLSID_DirectDraw && clsid!=CLSID_DirectDraw7) || iid!=IID_IDirectDraw7)
    return CLASS_E_CLASSNOTAVAILABLE;
	
//	DWORD Ret=DirectDrawCreateEx(0,ppv,IID_IDirectDraw7,0);

//    return Ret;
}
