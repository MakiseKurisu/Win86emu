// ShellIntf.cpp : Defines the exported functions for the DLL application.
//

#define __INLINE_ISEQUAL_GUID 1
#include "stdafx.h"
#include <windows.h>
#include <ShlGuid.h>
#include <ShlDisp.h>
#include <stdio.h>
#include <util.h>
#include <intrin.h>

#define SHLAPI extern "C" HRESULT __declspec(dllexport) __stdcall

class CAutoComplete2 : public IAutoComplete2
{
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(const IID&, void**);
    virtual ULONG   __stdcall AddRef();
    virtual ULONG   __stdcall Release();

    virtual HRESULT STDMETHODCALLTYPE Init(
        /* [annotation][in] */
        _In_  HWND hwndEdit,
        /* [annotation][unique][in] */
        _In_  IUnknown *punkACL,
        /* [annotation][unique][in] */
        _In_opt_  LPCWSTR pwszRegKeyPath,
        /* [annotation][in] */
        _In_opt_  LPCWSTR pwszQuickComplete)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE Enable(
        /* [in] */ BOOL fEnable)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE SetOptions(
        /* [in] */ DWORD dwFlag)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE GetOptions(
        /* [annotation][out] */
        _Out_  DWORD *pdwFlag)
    {
        return -1;
    }

    CAutoComplete2() : cRef(1)
    {
    }
    ~CAutoComplete2()
    {
    }

private:
    long cRef;
};


HRESULT __stdcall CAutoComplete2::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IAutoComplete2*>(this));
    }
    else if (iid == IID_IAutoComplete2 || iid == IID_IAutoComplete)
    {
        *ppv = static_cast<IAutoComplete2*>(this);
    }
    else
    {
        LogErr("CAutoComplete2::QueryInterface: Tried to get invalid iface");
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    LogWarn("CAutoComplete2::QueryInterface: returned dummy IAutoComplete");
    return S_OK;
}

ULONG __stdcall CAutoComplete2::AddRef()
{
    return InterlockedIncrement(&cRef);
}

ULONG __stdcall CAutoComplete2::Release()
{
    if (InterlockedDecrement(&cRef) == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

class CFactory : public IClassFactory
{
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(const IID&, void**);
    virtual ULONG   __stdcall AddRef();
    virtual ULONG   __stdcall Release();

    virtual HRESULT __stdcall CreateInstance(IUnknown* UnkOuter, const IID& iid, void** ppv);
    virtual HRESULT __stdcall LockServer(BOOL Lock);

    CFactory() : cRef(1)
    {
    }
    ~CFactory()
    {
    }

private:
    long cRef;
};

HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IClassFactory*>(this));
    }
    else if (iid == IID_IClassFactory)
    {
        *ppv = static_cast<IClassFactory*>(this);
    }
    else
    {
        CAutoComplete2 *I = new CAutoComplete2();
        HRESULT hr = I->QueryInterface(iid, ppv);
        I->Release();
        return hr;
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
    if (InterlockedDecrement(&cRef) == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

HRESULT __stdcall CFactory::CreateInstance(IUnknown* UnkOuter, const IID& iid, void** ppv)
{
    if (UnkOuter && iid != IID_IUnknown)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (iid == IID_IUnknown)
    {
    }

    LogErr("DDRaw::CreateInstance: Tried to get invalid iface");
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT __stdcall CFactory::LockServer(BOOL Lock)
{
    return S_OK;
}

SHLAPI   stub_DllCanUnloadNow()
{
    return S_FALSE;	// don't support unload now
}

SHLAPI   stub_DllRegisterServer()
{
    return S_OK;
}

SHLAPI   stub_DllUnregisterServer()
{
    return S_OK;
}

SHLAPI   stub_DllGetClassObject(const CLSID& clsid, const IID& iid, void **ppv)
{
    if ((clsid == CLSID_AutoComplete) && iid == IID_IClassFactory)
    {
        CFactory *F = new CFactory();

        HRESULT hr = F->QueryInterface(iid, ppv);
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

    return CLASS_E_CLASSNOTAVAILABLE;
}


