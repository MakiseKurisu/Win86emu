#define CINTERFACE	// I need C interface for lpVtbl member
#include "dllyact_def.h"
#include "comobject_def.h"
#define NO_COMMCTRL_DA
#include <shlobj.h>
#include <windows.h>

DEFINE_COMFUNC2(IM,IMallocVtbl,Alloc);
DEFINE_COMFUNC3(IM,IMallocVtbl,Realloc);
DEFINE_COMFUNC2(IM,IMallocVtbl,Free);
DEFINE_COMFUNC2(IM,IMallocVtbl,GetSize);
DEFINE_COMFUNC2(IM,IMallocVtbl,DidAlloc);
DEFINE_COMFUNC1(IM,IMallocVtbl,HeapMinimize);

//DEFINE_FUNC1(SHGetMalloc)
EXTERN_C DW STUB_EXPORT yact_SHGetMalloc(DW *R)
{
	LPMALLOC *lplpM=(LPMALLOC*)p1;
	HRESULT Ret=SHGetMalloc(lplpM);
	if(Ret!=S_OK)
		return (DWORD)Ret;

	ICallbackUnknown<IMallocVtbl> *IM=new ICallbackUnknown<IMallocVtbl>(*lplpM);
	*lplpM=(LPMALLOC)IM;

	DEFINE_CALLBACK2(IM,IMallocVtbl,Alloc);
	DEFINE_CALLBACK3(IM,IMallocVtbl,Realloc);
	DEFINE_CALLBACK2(IM,IMallocVtbl,Free);
	DEFINE_CALLBACK2(IM,IMallocVtbl,GetSize);
	DEFINE_CALLBACK2(IM,IMallocVtbl,DidAlloc);
	DEFINE_CALLBACK1(IM,IMallocVtbl,HeapMinimize);
	
	return Ret;
}

DEFINE_COMFUNC8(IS,IShellFolderVtbl,ParseDisplayName);
DEFINE_COMFUNC5(IS,IShellFolderVtbl,BindToStorage);        
DEFINE_COMFUNC4(IS,IShellFolderVtbl,CompareIDs);        
DEFINE_COMFUNC4(IS,IShellFolderVtbl,CreateViewObject);        
DEFINE_COMFUNC4(IS,IShellFolderVtbl,GetAttributesOf);
DEFINE_COMFUNC8(IS,IShellFolderVtbl,GetUIObjectOf);
DEFINE_COMFUNC4(IS,IShellFolderVtbl,GetDisplayNameOf);
DEFINE_COMFUNC6(IS,IShellFolderVtbl,SetNameOf);


DEFINE_COMFUNC4(EIL,IEnumIDListVtbl,Next);        
DEFINE_COMFUNC2(EIL,IEnumIDListVtbl,Skip);
DEFINE_COMFUNC1(EIL,IEnumIDListVtbl,Reset);

//DEFINE_COMFUNC6(EIL,IEnumIDListVtbl,Clone);
EXTERN_C DW COMSTUB_EXPORT EIL_Clone(DW *R)
{
	ICallbackUnknown<IEnumIDListVtbl> *Tmp=(ICallbackUnknown<IEnumIDListVtbl>*)p1;
	IEnumIDList **ppenumIDList=(IEnumIDList **)p2;
	DWORD Ret=Tmp->IOriginal->Vtbl->Clone((IEnumIDList*)Tmp->IOriginal,ppenumIDList);

	if(Ret!=S_OK)
		return (DWORD)Ret;

	ICallbackUnknown<IEnumIDListVtbl> *EIL=new ICallbackUnknown<IEnumIDListVtbl>(*ppenumIDList);
	*ppenumIDList=(IEnumIDList*)EIL;

	DEFINE_CALLBACK4(EIL,IEnumIDListVtbl,Next);        
	DEFINE_CALLBACK2(EIL,IEnumIDListVtbl,Skip);
	DEFINE_CALLBACK1(EIL,IEnumIDListVtbl,Reset);
	DEFINE_CALLBACK6(EIL,IEnumIDListVtbl,Clone);

	return Ret;
}

//DEFINE_COMFUNC4(IS,IShellFolderVtbl,EnumObjects);
EXTERN_C DW COMSTUB_EXPORT IS_EnumObjects(DW *R)
{
	ICallbackUnknown<IShellFolderVtbl> *Tmp=(ICallbackUnknown<IShellFolderVtbl>*)p1;
	HWND hWO=(HWND)p1;
	SHCONTF scf=(SHCONTF)p2;
	IEnumIDList **ppenumIDList=(IEnumIDList **)p4;
	DWORD Ret=Tmp->IOriginal->Vtbl->EnumObjects((IShellFolder*)Tmp->IOriginal,hWO,scf,ppenumIDList);

	if(Ret!=S_OK)
		return (DWORD)Ret;

	ICallbackUnknown<IEnumIDListVtbl> *EIL=new ICallbackUnknown<IEnumIDListVtbl>(*ppenumIDList);
	*ppenumIDList=(IEnumIDList*)EIL;

	DEFINE_CALLBACK4(EIL,IEnumIDListVtbl,Next);        
	DEFINE_CALLBACK2(EIL,IEnumIDListVtbl,Skip);
	DEFINE_CALLBACK1(EIL,IEnumIDListVtbl,Reset);
	DEFINE_CALLBACK6(EIL,IEnumIDListVtbl,Clone);

	return Ret;
}

//DEFINE_COMFUNC5(IS,IShellFolderVtbl,BindToObject);        
EXTERN_C DW COMSTUB_EXPORT IS_BindToObject(DW *R)
{
	ICallbackUnknown<IShellFolderVtbl> *Tmp=(ICallbackUnknown<IShellFolderVtbl>*)p1;
	PCUIDLIST_RELATIVE pidl=(PCUIDLIST_RELATIVE)p2;
	IBindCtx *pbc=(IBindCtx*)p3;
	REFIID riid=*(IID*)p4;
	void **ppvOut=(void**)p5;
	if(riid!=IID_IShellFolder || pbc!=NULL)		// TODO: implement IID_IStream and non-null IBindCtx
	{
		*ppvOut=0;
		return E_NOINTERFACE;
	}
	DWORD Ret=Tmp->IOriginal->Vtbl->BindToObject((IShellFolder*)Tmp->IOriginal,pidl,pbc,riid,ppvOut);

	if(Ret!=S_OK)
		return (DWORD)Ret;

	ICallbackUnknown<IShellFolderVtbl> *IS=new ICallbackUnknown<IShellFolderVtbl>(*ppvOut);
	*ppvOut=(IShellFolder*)IS;

	DEFINE_CALLBACK8(IS,IShellFolderVtbl,ParseDisplayName);
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,EnumObjects);        
	DEFINE_CALLBACK5(IS,IShellFolderVtbl,BindToObject);        
	DEFINE_CALLBACK5(IS,IShellFolderVtbl,BindToStorage);        
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,CompareIDs);        
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,CreateViewObject);        
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,GetAttributesOf);
	DEFINE_CALLBACK8(IS,IShellFolderVtbl,GetUIObjectOf);
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,GetDisplayNameOf);
	DEFINE_CALLBACK6(IS,IShellFolderVtbl,SetNameOf);

	return Ret;
}




//DEFINE_FUNC1(SHGetDesktopFolder)
EXTERN_C DW STUB_EXPORT yact_SHGetDesktopFolder(DW *R)
{
	IShellFolder **lplpSF=(IShellFolder **)p1;
	HRESULT Ret=SHGetDesktopFolder(lplpSF);
	if(Ret!=S_OK)
		return (DWORD)Ret;

	ICallbackUnknown<IShellFolderVtbl> *IS=new ICallbackUnknown<IShellFolderVtbl>(*lplpSF);
	*lplpSF=(IShellFolder*)IS;

	DEFINE_CALLBACK8(IS,IShellFolderVtbl,ParseDisplayName);
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,EnumObjects);        
	DEFINE_CALLBACK5(IS,IShellFolderVtbl,BindToObject);        
	DEFINE_CALLBACK5(IS,IShellFolderVtbl,BindToStorage);        
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,CompareIDs);        
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,CreateViewObject);        
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,GetAttributesOf);
	DEFINE_CALLBACK8(IS,IShellFolderVtbl,GetUIObjectOf);
	DEFINE_CALLBACK4(IS,IShellFolderVtbl,GetDisplayNameOf);
	DEFINE_CALLBACK6(IS,IShellFolderVtbl,SetNameOf);
	
	return Ret;
}

//DEFINE_FUNC1(SHBrowseForFolder)
EXTERN_C DW STUB_EXPORT yact_SHBrowseForFolder(DW *R)
{
	BROWSEINFOA *bi=(BROWSEINFOA*)p1;
	if(bi->lpfn)
		bi->lpfn=(BFFCALLBACK)CbCreateNativeStdcallCallback((DW)bi->lpfn,4);
	return (DW)SHBrowseForFolderA(bi);
}

//DEFINE_FUNC1(SHBrowseForFolderW)
EXTERN_C DW STUB_EXPORT yact_SHBrowseForFolderA(DW *R)
{
	BROWSEINFOA *bi=(BROWSEINFOA*)p1;
	if(bi->lpfn)
		bi->lpfn=(BFFCALLBACK)CbCreateNativeStdcallCallback((DW)bi->lpfn,4);
	return (DW)SHBrowseForFolderA(bi);
}
//DEFINE_FUNC1(SHBrowseForFolderA)
EXTERN_C DW STUB_EXPORT yact_SHBrowseForFolderW(DW *R)
{
	BROWSEINFOW *bi=(BROWSEINFOW*)p1;
	if(bi->lpfn)
		bi->lpfn=(BFFCALLBACK)CbCreateNativeStdcallCallback((DW)bi->lpfn,4);
	return (DW)SHBrowseForFolderW(bi);
}
