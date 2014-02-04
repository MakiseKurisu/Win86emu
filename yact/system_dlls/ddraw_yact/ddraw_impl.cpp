#define CINTERFACE	// I need C interface for DDraw for lpVtbl member
#include "dllyact_def.h"
#include "comobject_def.h"
#include <windows.h>
#include <ddraw.h>
#include <assert.h>
#include "classes.h"
#include "callbacks.h"
#include "x86emul.h"
#include "ddraw_common.h"

/* Functions that need different implementation:
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7 FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7 FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2 ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID,LPDDENUMSURFACESCALLBACK7 ) PURE;

    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC2) PURE;

    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE7 FAR *) PURE;

    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;

    STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE7 *) PURE;

    STDMETHOD(GetDeviceIdentifier)(THIS_ LPDDDEVICEIDENTIFIER2, DWORD ) PURE;

Surface:
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK7) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK7) PURE;

    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS2, LPDIRECTDRAWSURFACE7 FAR *) PURE;

    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS2) PURE;

    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC2) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC2) PURE;

    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE) PURE;

    STDMETHOD(Unlock)(THIS_ LPRECT) PURE;

    STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC2, DWORD) PURE;
*/



EXTERN_C DW COMSTUB_EXPORT DDS_QueryInterface(DW *R);

HRESULT WINAPI EnumSurfacesCallback( 
  LPDIRECTDRAWSURFACE lpDDSurface, 
  LPDDSURFACEDESC lpDDSurfaceDesc, 
  LPVOID lpContext
);
HRESULT WINAPI EnumSurfacesCallback2( 
  LPDIRECTDRAWSURFACE4 lpDDSurface, 
  LPDDSURFACEDESC2 lpDDSurfaceDesc, 
  LPVOID lpContext
);
HRESULT WINAPI EnumSurfacesCallback7( 
  LPDIRECTDRAWSURFACE7 lpDDSurface, 
  LPDDSURFACEDESC2 lpDDSurfaceDesc, 
  LPVOID lpContext
);

//DEFINE_COMFUNC5(DD,IDirectDrawVtbl,EnumDisplayModes);
HRESULT WINAPI EnumModesCallback(
  LPDDSURFACEDESC lpDDSurfaceDesc, 
  LPVOID lpContext
)
{
	DWORD *Ctx=(DWORD*)lpContext;
	DWORD Ret;
	Ret=EmuExecute(Ctx[1],2,lpDDSurfaceDesc,Ctx[0]);
	if(Ret!=DDENUMRET_OK)
		return (HRESULT)Ret;

	lpDDSurfaceDesc->ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags=DDPF_RGB;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount=16;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask=0xf800;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask=0x7E0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask=0x1f;
	lpDDSurfaceDesc->lPitch/=2;
	Ret=EmuExecute(Ctx[1],2,lpDDSurfaceDesc,Ctx[0]);
	if(Ret!=DDENUMRET_OK)
		return (HRESULT)Ret;

	lpDDSurfaceDesc->lPitch/=2;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount=8;
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask=0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask=0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask=0;
	Ret=EmuExecute(Ctx[1],2,lpDDSurfaceDesc,Ctx[0]);

	return (HRESULT)Ret;
}

HRESULT WINAPI EnumModesCallback2(
  LPDDSURFACEDESC2 lpDDSurfaceDesc, 
  LPVOID lpContext
)
{
	DWORD *Ctx=(DWORD*)lpContext;
	DWORD Ret;
	Ret=EmuExecute(Ctx[1],2,lpDDSurfaceDesc,Ctx[0]);
	if(Ret!=DDENUMRET_OK)
		return (HRESULT)Ret;

	lpDDSurfaceDesc->ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags=DDPF_RGB;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount=16;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask=0xf800;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask=0x7E0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask=0x1f;
	Ret=EmuExecute(Ctx[1],2,lpDDSurfaceDesc,Ctx[0]);
	if(Ret!=DDENUMRET_OK)
		return (HRESULT)Ret;

	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount=8;
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask=0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask=0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask=0;
	Ret=EmuExecute(Ctx[1],2,lpDDSurfaceDesc,Ctx[0]);

	return (HRESULT)Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD_EnumDisplayModes(DW *R)
{
	DWORD Ctx[2];
	Ctx[0]=p4;
	Ctx[1]=p5;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumDisplayModes((LPDIRECTDRAW)Tmp->IOriginal,p2,(LPDDSURFACEDESC)p3,Ctx,EnumModesCallback);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_EnumDisplayModes(DW *R)
{
	DWORD Ctx[2];
	Ctx[0]=p4;
	Ctx[1]=p5;
	ICallbackUnknown<IDirectDraw2Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw2Vtbl>*)p1;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumDisplayModes((LPDIRECTDRAW2)Tmp->IOriginal,p2,(LPDDSURFACEDESC)p3,Ctx,EnumModesCallback);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD4_GetSurfaceFromDC(DW *R)
{
	return E_FAIL;
}

EXTERN_C DW COMSTUB_EXPORT DD7_GetSurfaceFromDC(DW *R)
{
	return E_FAIL;
}

EXTERN_C DW COMSTUB_EXPORT DD4_EnumDisplayModes(DW *R)
{
	DWORD Ctx[2];
	Ctx[0]=p4;
	Ctx[1]=p5;
	ICallbackUnknown<IDirectDraw4Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw4Vtbl>*)p1;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumDisplayModes((LPDIRECTDRAW4)Tmp->IOriginal,p2,(LPDDSURFACEDESC2)p3,Ctx,EnumModesCallback2);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD7_EnumDisplayModes(DW *R)
{
	DWORD Ctx[2];
	Ctx[0]=p4;
	Ctx[1]=p5;
	ICallbackUnknown<IDirectDraw7Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw7Vtbl>*)p1;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumDisplayModes((LPDIRECTDRAW7)Tmp->IOriginal,p2,(LPDDSURFACEDESC2)p3,Ctx,EnumModesCallback2);
	return Ret;
}

//DEFINE_COMFUNC5(DD,IDirectDrawVtbl,EnumSurfaces);
EXTERN_C DW COMSTUB_EXPORT DD_EnumSurfaces(DW *R)
{
	DWORD Ctx[2];
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	Ctx[0]=p4;
	Ctx[1]=p5;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumSurfaces((LPDIRECTDRAW)Tmp->IOriginal,p2,(LPDDSURFACEDESC)p3,Ctx,EnumSurfacesCallback);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_EnumSurfaces(DW *R)
{
	DWORD Ctx[2];
	ICallbackUnknown<IDirectDraw2Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw2Vtbl>*)p1;
	Ctx[0]=p4;
	Ctx[1]=p5;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumSurfaces((LPDIRECTDRAW2)Tmp->IOriginal,p2,(LPDDSURFACEDESC)p3,Ctx,EnumSurfacesCallback);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD4_EnumSurfaces(DW *R)
{
	DWORD Ctx[2];
	ICallbackUnknown<IDirectDraw4Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw4Vtbl>*)p1;
	Ctx[0]=p4;
	Ctx[1]=p5;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumSurfaces((LPDIRECTDRAW4)Tmp->IOriginal,p2,(LPDDSURFACEDESC2)p3,Ctx,EnumSurfacesCallback2);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD7_EnumSurfaces(DW *R)
{
	DWORD Ctx[2];
	ICallbackUnknown<IDirectDraw7Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw7Vtbl>*)p1;
	Ctx[0]=p4;
	Ctx[1]=p5;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumSurfaces((LPDIRECTDRAW7)Tmp->IOriginal,p2,(LPDDSURFACEDESC2)p3,Ctx,EnumSurfacesCallback7);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_GetClipper(DW *R);

EXTERN_C DW COMSTUB_EXPORT DD_SetCooperativeLevel(DW *R) 
{
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	HWND hwnd=(HWND)p2;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;
	DDE->Hwnd=hwnd;
	DWORD dwFlags=p3;
	DWORD Ret=Tmp->IOriginal->Vtbl->SetCooperativeLevel((LPDIRECTDRAW)Tmp->IOriginal,hwnd,dwFlags);
	return Ret;
} 

EXTERN_C DW COMSTUB_EXPORT DD2_SetCooperativeLevel(DW *R) 
{
	ICallbackUnknown<IDirectDraw2Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw2Vtbl>*)p1;
	HWND hwnd=(HWND)p2;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;
	DDE->Hwnd=hwnd;
	DWORD dwFlags=p3;
	DWORD Ret=Tmp->IOriginal->Vtbl->SetCooperativeLevel((LPDIRECTDRAW2)Tmp->IOriginal,hwnd,dwFlags);
	return Ret;
} 

EXTERN_C DW COMSTUB_EXPORT DD4_SetCooperativeLevel(DW *R) 
{
	ICallbackUnknown<IDirectDraw4Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw4Vtbl>*)p1;
	HWND hwnd=(HWND)p2;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;
	DDE->Hwnd=hwnd;
	DWORD dwFlags=p3;
	DWORD Ret=Tmp->IOriginal->Vtbl->SetCooperativeLevel((LPDIRECTDRAW4)Tmp->IOriginal,hwnd,dwFlags);
	return Ret;
} 

EXTERN_C DW COMSTUB_EXPORT DD7_SetCooperativeLevel(DW *R) 
{
	ICallbackUnknown<IDirectDraw7Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw7Vtbl>*)p1;
	HWND hwnd=(HWND)p2;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;
	DDE->Hwnd=hwnd;
	DWORD dwFlags=p3;
	DWORD Ret=Tmp->IOriginal->Vtbl->SetCooperativeLevel((LPDIRECTDRAW7)Tmp->IOriginal,hwnd,dwFlags);
	return Ret;
} 

EXTERN_C DW COMSTUB_EXPORT DD_SetDisplayMode(DW *R) 
{
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	DWORD W=p2;
	DWORD H=p3;
	DWORD Bpp=p4;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;

	DDE->Bpp=Bpp;
	Tmp->Data=DDE;
	Bpp=32;
	if(H<MinHeight)
	{
		W=MinWidth;
		H=MinHeight;
	}

	DWORD Ret=Tmp->IOriginal->Vtbl->SetDisplayMode((LPDIRECTDRAW)Tmp->IOriginal,W,H,Bpp);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_SetDisplayMode(DW *R) 
{
	ICallbackUnknown<IDirectDraw2Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw2Vtbl>*)p1;
	DWORD W=p2;
	DWORD H=p3;
	DWORD Bpp=p4;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;

	DDE->Bpp=Bpp;
	Tmp->Data=DDE;
	Bpp=32;
	if(H<MinHeight)
	{
		W=MinWidth;
		H=MinHeight;
	}

	DWORD Ret=Tmp->IOriginal->Vtbl->SetDisplayMode((LPDIRECTDRAW2)Tmp->IOriginal,W,H,Bpp,p5,p6);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_AddAttachedSurface(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->AddAttachedSurface((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPDIRECTDRAWSURFACE)Tmp1->IOriginal);
}

DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,AddOverlayDirtyRect)

EXTERN_C DW COMSTUB_EXPORT DDS_Blt(DW *R)
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p3;
	LPRECT Dst=(LPRECT)p2;
	LPRECT Src=(LPRECT)p4;
	DWORD Flags=p5;
	LPDDBLTFX bltfx=(LPDDBLTFX)p6;
	if(Tmp->Data)
	{
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		DWORD Ret=Tmp->IOriginal->Vtbl->Blt((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPRECT)p2,Tmp1?(LPDIRECTDRAWSURFACE)Tmp1->IOriginal:NULL,(LPRECT)p4,p5|DDBLT_WAIT,(LPDDBLTFX)p6);	// blit to fake surface
		if(Ret!=DD_OK)
			return Ret;
		Ret=BltEx((LPDIRECTDRAWSURFACE)DDSE->Screen,(LPRECT)p2,(LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPRECT)p2,0,(LPDDBLTFX)p6);				// now blit to screen, the same rect, no flags/no alpha
		if(Ret!=DD_OK)
			Ret=DDERR_SURFACELOST;
		return Ret;	
	}
/*	if(Flags&DDBLT_COLORFILL)	// DDBLT_COLORFILL is E_NOTIMPL on RT
	{
		HDC dc;
		HRESULT hr=Tmp->IOriginal->Vtbl->GetDC((LPDIRECTDRAWSURFACE)Tmp->IOriginal,&dc);	// it fails
		BOOL Failed=FALSE;
		if(!SUCCEEDED(hr))
		{
			Failed=TRUE;
			dc=GetDC(GetDesktopWindow());
		}

		HBRUSH hb=CreateSolidBrush(bltfx->dwFillColor&0xFFFFFF);	// TODO: implement non-32bit colors
		FillRect(dc,Dst,hb);
		DeleteObject(hb);
		if(!Failed)
			Tmp->IOriginal->Vtbl->ReleaseDC((LPDIRECTDRAWSURFACE)Tmp->IOriginal,dc);
		else
			ReleaseDC(GetDesktopWindow(),dc);

		return (DW)hr;
	}*/
	DW Ret=Tmp->IOriginal->Vtbl->Blt((LPDIRECTDRAWSURFACE)Tmp->IOriginal,Dst,Tmp1?(LPDIRECTDRAWSURFACE)Tmp1->IOriginal:NULL,Src,Flags,bltfx);
	if(Ret==E_NOTIMPL)	// On RT DDBLT_COLORFILL is not implemented, so just silently ignore it
		Ret=0;
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_BltBatch(DW *R)
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	DDBLTBATCH *Batch=(DDBLTBATCH*)p2;
	DW Ret=0;
	for(int i=0; i<(int)p3 && Ret==0; i++)
	{
		ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)Batch[i].lpDDSSrc;
		Ret=Tmp->IOriginal->Vtbl->Blt((LPDIRECTDRAWSURFACE)Tmp->IOriginal,Batch[i].lprDest,Tmp1?(LPDIRECTDRAWSURFACE)Tmp1->IOriginal:NULL,Batch[i].lprSrc,Batch[i].dwFlags,Batch[i].lpDDBltFx);
	}
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_BltFast(DW *R)
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p4;
	if(Tmp->Data)
	{
		RECT Dst;
		LPRECT Src=(LPRECT)p5;
		RECT TmpR;
		if(Src==0)
		{
		    DDSURFACEDESC2 ddsdSource;
		    ::ZeroMemory(&ddsdSource, sizeof(ddsdSource));
		    HRESULT hr = Tmp1->IOriginal->Vtbl->GetSurfaceDesc((LPDIRECTDRAWSURFACE)Tmp1->IOriginal,(DDSURFACEDESC*)&ddsdSource);
		    if(FAILED(hr))
		    {
				ddsdSource.dwSize = sizeof(DDSURFACEDESC);    
				hr = Tmp1->IOriginal->Vtbl->GetSurfaceDesc((LPDIRECTDRAWSURFACE)Tmp1->IOriginal,(DDSURFACEDESC*)&ddsdSource);
				Src=&TmpR;
				TmpR.left=TmpR.top=0;
				TmpR.right=ddsdSource.dwWidth;
				TmpR.bottom=ddsdSource.dwHeight;
			}
		}
		Dst.left=p2;
		Dst.top=p3;
		Dst.right=Src->right-Src->left+Dst.left;
		Dst.bottom=Src->bottom-Src->top+Dst.top;
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		DWORD Ret=Tmp->IOriginal->Vtbl->BltFast((LPDIRECTDRAWSURFACE)Tmp->IOriginal,p2,p3,Tmp1?(LPDIRECTDRAWSURFACE)Tmp1->IOriginal:NULL,(LPRECT)p5,p6|DDBLTFAST_WAIT);
		if(Ret!=DD_OK)
			return Ret;
		Ret=BltEx((LPDIRECTDRAWSURFACE)DDSE->Screen,&Dst,(LPDIRECTDRAWSURFACE)Tmp->IOriginal,&Dst,0,(LPDDBLTFX)0);				// now blit to screen, the same rect, no flags/no alpha, fake DDBLTFX
		if(Ret!=DD_OK)
			return DDERR_SURFACELOST;
		return Ret;
	}
	return Tmp->IOriginal->Vtbl->BltFast((LPDIRECTDRAWSURFACE)Tmp->IOriginal,p2,p3,Tmp1?(LPDIRECTDRAWSURFACE)Tmp1->IOriginal:NULL,(LPRECT)p5,p6);
}

EXTERN_C DW COMSTUB_EXPORT DDS_DeleteAttachedSurface(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p3;
	return Tmp->IOriginal->Vtbl->DeleteAttachedSurface((LPDIRECTDRAWSURFACE)Tmp->IOriginal,p2,(LPDIRECTDRAWSURFACE)Tmp1->IOriginal);
}

//DEFINE_COMFUNC3(DDS,IDirectDrawSurfaceVtbl,EnumAttachedSurfaces)
EXTERN_C DW COMSTUB_EXPORT DDS_EnumAttachedSurfaces(DW *R) 
{
	DWORD Ctx[2];
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	Ctx[0]=p2;
	Ctx[1]=p3;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumAttachedSurfaces((LPDIRECTDRAWSURFACE)Tmp->IOriginal,Ctx,EnumSurfacesCallback);
	return Ret;
}

//DEFINE_COMFUNC4(DDS,IDirectDrawSurfaceVtbl,EnumOverlayZOrders)
EXTERN_C DW COMSTUB_EXPORT DDS_EnumOverlayZOrders(DW *R) 
{
	DWORD Ctx[2];
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	Ctx[0]=p3;
	Ctx[1]=p4;
	HRESULT Ret = Tmp->IOriginal->Vtbl->EnumOverlayZOrders((LPDIRECTDRAWSURFACE)Tmp->IOriginal,p2,Ctx,EnumSurfacesCallback);
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_Flip(DW *R)
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->Flip((LPDIRECTDRAWSURFACE)Tmp->IOriginal,Tmp1?(LPDIRECTDRAWSURFACE)Tmp1->IOriginal:NULL,p3);
}

EXTERN_C DW COMSTUB_EXPORT DDS_GetPalette(DW *R);
DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,GetBltStatus)
DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,GetCaps)
DEFINE_COMFUNC3(DDS,IDirectDrawSurfaceVtbl,GetColorKey)
DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,GetDC)
DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,GetFlipStatus)
DEFINE_COMFUNC3(DDS,IDirectDrawSurfaceVtbl,GetOverlayPosition)
DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,GetPixelFormat)
//DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,GetSurfaceDesc)
EXTERN_C DW COMSTUB_EXPORT DDS_GetSurfaceDesc(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	LPDDSURFACEDESC ddsd=(LPDDSURFACEDESC)p2;
	return Tmp->IOriginal->Vtbl->GetSurfaceDesc((LPDIRECTDRAWSURFACE)Tmp->IOriginal,ddsd);
}
//DEFINE_COMFUNC3(DDS,IDirectDrawSurfaceVtbl,Initialize)
EXTERN_C DW COMSTUB_EXPORT DDS_Initialize(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->Initialize((LPDIRECTDRAWSURFACE)Tmp->IOriginal,Tmp1?(LPDIRECTDRAW)Tmp1->IOriginal:0,(LPDDSURFACEDESC)p3);
}

//DEFINE_COMFUNC1(DDS,IDirectDrawSurfaceVtbl,IsLost)
EXTERN_C DW COMSTUB_EXPORT DDS_IsLost(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	LPDDSURFACEDESC lpddsd=(LPDDSURFACEDESC)p3;
	if(Tmp->Data)
	{
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		if(DDSE->Screen->lpVtbl->IsLost(DDSE->Screen)==DDERR_SURFACELOST)
			return DDERR_SURFACELOST;
	}
	DWORD Ret= Tmp->IOriginal->Vtbl->IsLost((LPDIRECTDRAWSURFACE)Tmp->IOriginal);		
	return Ret;
}

//DEFINE_COMFUNC5(DDS,IDirectDrawSurfaceVtbl,Lock)
EXTERN_C DW COMSTUB_EXPORT DDS_Lock(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	LPDDSURFACEDESC lpddsd=(LPDDSURFACEDESC)p3;
	if(Tmp->Data)
	{
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		if(DDSE->Screen->lpVtbl->IsLost(DDSE->Screen)==DDERR_SURFACELOST)
			return DDERR_SURFACELOST;
	}
	DWORD Ret= Tmp->IOriginal->Vtbl->Lock((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPRECT)p2,lpddsd,p4,(HANDLE)p5);
		
	return Ret;
}

//DEFINE_COMFUNC1(DDS,IDirectDrawSurfaceVtbl,Restore)
EXTERN_C DW COMSTUB_EXPORT DDS_Restore(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	if(Tmp->Data)
	{
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		DWORD Ret=DDSE->Screen->lpVtbl->Restore(DDSE->Screen);
		if(Ret!=DD_OK)
			return Ret;
	}
	return Tmp->IOriginal->Vtbl->Restore((LPDIRECTDRAWSURFACE)Tmp->IOriginal);
}

EXTERN_C DW COMSTUB_EXPORT DDS_SetClipper(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawClipperVtbl> *TmpC=(ICallbackUnknown<IDirectDrawClipperVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->SetClipper((LPDIRECTDRAWSURFACE)Tmp->IOriginal,TmpC?(LPDIRECTDRAWCLIPPER)TmpC->IOriginal:0);
}

DEFINE_COMFUNC3(DDS,IDirectDrawSurfaceVtbl,SetColorKey)
DEFINE_COMFUNC3(DDS,IDirectDrawSurfaceVtbl,SetOverlayPosition)

EXTERN_C DW COMSTUB_EXPORT DDS_SetPalette(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawPaletteVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawPaletteVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->SetPalette((LPDIRECTDRAWSURFACE)Tmp->IOriginal,Tmp1?(LPDIRECTDRAWPALETTE)Tmp1->IOriginal:0);
}

//DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,ReleaseDC)
EXTERN_C DW COMSTUB_EXPORT DDS_ReleaseDC(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	HDC hDC=(HDC)p2;
	DWORD Ret= Tmp->IOriginal->Vtbl->ReleaseDC((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(HDC)p2);
	if(Tmp->Data)
	{
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		if(DDSE->Screen->lpVtbl->IsLost(DDSE->Screen)==DDERR_SURFACELOST)
			return DDERR_SURFACELOST;
		DDSURFACEDESC2 ddsdDest;
		::ZeroMemory(&ddsdDest, sizeof(ddsdDest));
		ddsdDest.dwSize = sizeof(ddsdDest);    
		HRESULT hr = DDSE->Screen->lpVtbl->GetSurfaceDesc(DDSE->Screen,(DDSURFACEDESC*)&ddsdDest);
	    if(FAILED(hr))
	    {
		   ddsdDest.dwSize = sizeof(DDSURFACEDESC);    
		   hr = DDSE->Screen->lpVtbl->GetSurfaceDesc(DDSE->Screen,(DDSURFACEDESC*)&ddsdDest);
	    }
		RECT Dst;
		RECT Src;
		Src.left=0;
		Src.top=0;
		Src.right=ddsdDest.dwWidth;
		Src.bottom=ddsdDest.dwHeight;
		Dst.left=Src.left;
		Dst.top=Src.top;
		Dst.right=Src.right-Src.left+Dst.left;
		Dst.bottom=Src.bottom-Src.top+Dst.top;
				
		Ret=BltEx((LPDIRECTDRAWSURFACE)DDSE->Screen,&Dst,(LPDIRECTDRAWSURFACE)Tmp->IOriginal,&Src,0,0);				// now blit to screen, the same rect, no flags/no alpha
		if(Ret!=DD_OK)
			return DDERR_SURFACELOST;
		return Ret;
	}
	
	return Ret;
}

//DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,Unlock)
EXTERN_C DW COMSTUB_EXPORT DDS_Unlock(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	LPRECT lpR=(LPRECT)p2;
	DWORD Ret= Tmp->IOriginal->Vtbl->Unlock((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPRECT)p2);
	if(Tmp->Data)
	{
		DDS_Emul *DDSE=(DDS_Emul*)Tmp->Data;
		if(DDSE->Screen->lpVtbl->IsLost(DDSE->Screen)==DDERR_SURFACELOST)
			return DDERR_SURFACELOST;
		DDSURFACEDESC2 ddsdDest;
		::ZeroMemory(&ddsdDest, sizeof(ddsdDest));
		ddsdDest.dwSize = sizeof(ddsdDest);    
		HRESULT hr = DDSE->Screen->lpVtbl->GetSurfaceDesc(DDSE->Screen,(DDSURFACEDESC*)&ddsdDest);
		if(FAILED(hr))
		{
		   ddsdDest.dwSize = sizeof(DDSURFACEDESC);    
		   hr = DDSE->Screen->lpVtbl->GetSurfaceDesc(DDSE->Screen,(DDSURFACEDESC*)&ddsdDest);
		}
		RECT Dst;
		RECT Src;
		Src.left=0;
		Src.top=0;
		Src.right=ddsdDest.dwWidth;
		Src.bottom=ddsdDest.dwHeight;
		Dst.left=Src.left;
		Dst.top=Src.top;
		Dst.right=Src.right-Src.left+Dst.left;
		Dst.bottom=Src.bottom-Src.top+Dst.top;
				
		Ret=BltEx((LPDIRECTDRAWSURFACE)DDSE->Screen,&Dst,(LPDIRECTDRAWSURFACE)Tmp->IOriginal,&Src,0,0);				// now blit to screen, the same rect, no flags/no alpha
		if(Ret!=DD_OK)
			return DDERR_SURFACELOST;
		return Ret;
	}
	
	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_UpdateOverlay(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p3;
	return Tmp->IOriginal->Vtbl->UpdateOverlay((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPRECT)p2,(LPDIRECTDRAWSURFACE)Tmp1->IOriginal,(LPRECT)p4,p5,(LPDDOVERLAYFX)p6);
}
DEFINE_COMFUNC2(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayDisplay)

EXTERN_C DW COMSTUB_EXPORT DDS_UpdateOverlayZOrder(DW *R) 
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p3;
	return Tmp->IOriginal->Vtbl->UpdateOverlayZOrder((LPDIRECTDRAWSURFACE)Tmp->IOriginal,p2,(LPDIRECTDRAWSURFACE)Tmp1->IOriginal);
}

EXTERN_C DW COMSTUB_EXPORT DDS_GetAttachedSurface(DW *R) 
{
	LPDIRECTDRAWSURFACE * lplpDDS=(LPDIRECTDRAWSURFACE*)p3;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->GetAttachedSurface((LPDIRECTDRAWSURFACE)Tmp->IOriginal,(LPDDSCAPS)p2,lplpDDS);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawSurfaceVtbl> *DDS=new ICallbackUnknown<IDirectDrawSurfaceVtbl>(*lplpDDS);
	*lplpDDS=(LPDIRECTDRAWSURFACE)DDS;

	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,QueryInterface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddOverlayDirtyRect)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,Blt)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,BltBatch)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,BltFast)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,DeleteAttachedSurface)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,EnumAttachedSurfaces)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,EnumOverlayZOrders)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Flip)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetBltStatus)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetCaps)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetColorKey)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetDC)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetFlipStatus)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPixelFormat)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetSurfaceDesc)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Initialize)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,IsLost)
	DEFINE_CALLBACK5(DDS,IDirectDrawSurfaceVtbl,Lock)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,ReleaseDC)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,Restore)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetColorKey)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,Unlock)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,UpdateOverlay)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayDisplay)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayZOrder)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD_GetGDISurface(DW *R) 
{
	LPDIRECTDRAWSURFACE * lplpDDS=(LPDIRECTDRAWSURFACE*)p2;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->GetGDISurface((LPDIRECTDRAW)Tmp->IOriginal,lplpDDS);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawSurfaceVtbl> *DDS=new ICallbackUnknown<IDirectDrawSurfaceVtbl>(*lplpDDS);
	*lplpDDS=(LPDIRECTDRAWSURFACE)DDS;

	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,QueryInterface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddOverlayDirtyRect)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,Blt)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,BltBatch)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,BltFast)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,DeleteAttachedSurface)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,EnumAttachedSurfaces)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,EnumOverlayZOrders)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Flip)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetBltStatus)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetCaps)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetColorKey)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetDC)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetFlipStatus)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPixelFormat)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetSurfaceDesc)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Initialize)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,IsLost)
	DEFINE_CALLBACK5(DDS,IDirectDrawSurfaceVtbl,Lock)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,ReleaseDC)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,Restore)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetColorKey)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,Unlock)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,UpdateOverlay)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayDisplay)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayZOrder)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_GetGDISurface(DW *R) 
{
	return DD_GetGDISurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD4_GetGDISurface(DW *R) 
{
	return DD_GetGDISurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD7_GetGDISurface(DW *R) 
{
	return DD_GetGDISurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD_DuplicateSurface(DW *R) 
{
	LPDIRECTDRAWSURFACE * lplpDDS=(LPDIRECTDRAWSURFACE*)p3;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p2;
	DWORD Ret=Tmp->IOriginal->Vtbl->DuplicateSurface((LPDIRECTDRAW)Tmp->IOriginal,(LPDIRECTDRAWSURFACE)Tmp->IOriginal,lplpDDS);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawSurfaceVtbl> *DDS=new ICallbackUnknown<IDirectDrawSurfaceVtbl>(*lplpDDS);
	*lplpDDS=(LPDIRECTDRAWSURFACE)DDS;

	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,QueryInterface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddOverlayDirtyRect)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,Blt)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,BltBatch)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,BltFast)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,DeleteAttachedSurface)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,EnumAttachedSurfaces)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,EnumOverlayZOrders)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Flip)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetBltStatus)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetCaps)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetColorKey)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetDC)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetFlipStatus)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPixelFormat)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetSurfaceDesc)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Initialize)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,IsLost)
	DEFINE_CALLBACK5(DDS,IDirectDrawSurfaceVtbl,Lock)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,ReleaseDC)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,Restore)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetColorKey)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,Unlock)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,UpdateOverlay)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayDisplay)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayZOrder)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_DuplicateSurface(DW *R) 
{
	return DD_DuplicateSurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD4_DuplicateSurface(DW *R) 
{
	return DD_DuplicateSurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD7_DuplicateSurface(DW *R) 
{
	return DD_DuplicateSurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD_CreateSurface(DW *R) 
{
	LPDIRECTDRAWSURFACE * lplpDDS=(LPDIRECTDRAWSURFACE*)p3;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	LPDDSURFACEDESC lpddsd=(LPDDSURFACEDESC)p2;
	DDSURFACEDESC ddt;
	DWORD Ret;
	bool FakeSurface=false;
	IDirectDrawSurface *DDSurf=0;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;
	if(DDE && DDE->Bpp!=32) 
	{
		if(lpddsd->dwFlags&DDSD_CAPS)
		{
			if(lpddsd->ddsCaps.dwCaps&DDSCAPS_FLIP)		// TODO: currently I don't emulate flips
				return DDERR_NOFLIPHW;
			if(lpddsd->ddsCaps.dwCaps&DDSCAPS_OVERLAY)
				return DDERR_NOOVERLAYHW;

			if(lpddsd->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE)
			{
				Ret=Tmp->IOriginal->Vtbl->CreateSurface((LPDIRECTDRAW)Tmp->IOriginal,lpddsd,&DDSurf,(LPUNKNOWN)p4);
				if(Ret!=DD_OK)
					return Ret;

				memset(&ddt,0,sizeof(ddt));
				ddt.dwSize=sizeof(ddt);
				DDSurf->lpVtbl->GetSurfaceDesc(DDSurf,&ddt);
				ddt.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
				ddt.ddpfPixelFormat.dwFlags=DDPF_RGB;
				ddt.ddpfPixelFormat.dwRGBBitCount=DDE->Bpp;
				switch (DDE->Bpp)
				{
				case 15:
					ddt.lPitch/=2;
					ddt.ddpfPixelFormat.dwRBitMask=0x7c00;
					ddt.ddpfPixelFormat.dwGBitMask=0x3E0;
					ddt.ddpfPixelFormat.dwBBitMask=0x1f;
					break;
				case 16:
					ddt.lPitch/=2;
					ddt.ddpfPixelFormat.dwRBitMask=0xf800;
					ddt.ddpfPixelFormat.dwGBitMask=0x7E0;
					ddt.ddpfPixelFormat.dwBBitMask=0x1f;
					break;
				case 24:
				case 32:
					ddt.ddpfPixelFormat.dwRBitMask=0xFF0000;
					ddt.ddpfPixelFormat.dwGBitMask=0xFF00;
					ddt.ddpfPixelFormat.dwBBitMask=0xFF;
					break;
				case 8:
					ddt.lPitch/=4;
					ddt.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
					// Fall through
				default:
					ddt.ddpfPixelFormat.dwRBitMask=0;
					ddt.ddpfPixelFormat.dwGBitMask=0;
					ddt.ddpfPixelFormat.dwBBitMask=0;
					break;
				}
				lpddsd=&ddt;
				FakeSurface=true;
				lpddsd->ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
				lpddsd->dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT; 
				lpddsd->lpSurface=malloc(lpddsd->lPitch*lpddsd->dwHeight*ddt.ddpfPixelFormat.dwRGBBitCount/8);	// TODO: this memory is leaked
			}
		}
		if((lpddsd->dwFlags&DDSD_PIXELFORMAT)==0)		// if user does not specify pixel format - set it to be as the emulated display mode 
		{
			ddt=*lpddsd;
			lpddsd=&ddt;
			lpddsd->ddsCaps.dwCaps&=~DDSCAPS_VIDEOMEMORY;
			lpddsd->ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;
			ddt.dwFlags|=DDSD_PIXELFORMAT;
			ddt.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
			ddt.ddpfPixelFormat.dwFlags=DDPF_RGB;
			ddt.ddpfPixelFormat.dwRGBBitCount=DDE->Bpp;
			switch (DDE->Bpp)
			{
			case 15:
				ddt.lPitch/=2;
				ddt.ddpfPixelFormat.dwRBitMask=0x7c00;
				ddt.ddpfPixelFormat.dwGBitMask=0x3E0;
				ddt.ddpfPixelFormat.dwBBitMask=0x1f;
				break;
			case 16:
				ddt.lPitch/=2;
				ddt.ddpfPixelFormat.dwRBitMask=0xf800;
				ddt.ddpfPixelFormat.dwGBitMask=0x7E0;
				ddt.ddpfPixelFormat.dwBBitMask=0x1f;
				break;
			case 24:
			case 32:
				ddt.ddpfPixelFormat.dwRBitMask=0xFF0000;
				ddt.ddpfPixelFormat.dwGBitMask=0xFF00;
				ddt.ddpfPixelFormat.dwBBitMask=0xFF;
				break;
			case 8:
				ddt.lPitch/=4;
				ddt.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
				// Fall through
			default:
				ddt.ddpfPixelFormat.dwRBitMask=0;
				ddt.ddpfPixelFormat.dwGBitMask=0;
				ddt.ddpfPixelFormat.dwBBitMask=0;
				break;
			}
		}
	}

	Ret=Tmp->IOriginal->Vtbl->CreateSurface((LPDIRECTDRAW)Tmp->IOriginal,lpddsd,lplpDDS,(LPUNKNOWN)p4);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawSurfaceVtbl> *DDS=new ICallbackUnknown<IDirectDrawSurfaceVtbl>(*lplpDDS);
	*lplpDDS=(LPDIRECTDRAWSURFACE)DDS;

	if(FakeSurface)
	{
		DDS_Emul *DDSE=new DDS_Emul;
		DDSE->Screen=DDSurf;
		DDSE->DDraw=(LPDIRECTDRAW)Tmp->IOriginal;
		DDSE->Hwnd=DDE->Hwnd;
		DDS->Data=DDSE;
	}

	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,QueryInterface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddOverlayDirtyRect)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,Blt)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,BltBatch)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,BltFast)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,DeleteAttachedSurface)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,EnumAttachedSurfaces)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,EnumOverlayZOrders)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Flip)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetBltStatus)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetCaps)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetColorKey)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetDC)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetFlipStatus)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPixelFormat)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetSurfaceDesc)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Initialize)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,IsLost)
	DEFINE_CALLBACK5(DDS,IDirectDrawSurfaceVtbl,Lock)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,ReleaseDC)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,Restore)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetColorKey)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,Unlock)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,UpdateOverlay)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayDisplay)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayZOrder)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_CreateSurface(DW *R) 
{
	return DD_CreateSurface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD4_CreateSurface(DW *R) 
{
	LPDIRECTDRAWSURFACE4 * lplpDDS=(LPDIRECTDRAWSURFACE4*)p3;
	ICallbackUnknown<IDirectDraw4Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw4Vtbl>*)p1;
	LPDDSURFACEDESC2 lpddsd=(LPDDSURFACEDESC2)p2;
	DDSURFACEDESC2 ddt;
	DWORD Ret;
	bool FakeSurface=false;
	IDirectDrawSurface4 *DDSurf=0;
	DD_Emul *DDE=(DD_Emul*)Tmp->Data;
	if(DDE && DDE->Bpp!=32) 
	{
		if(lpddsd->dwFlags&DDSD_CAPS)
		{
			if(lpddsd->ddsCaps.dwCaps&DDSCAPS_FLIP)		// TODO: currently I don't emulate flips
				return DDERR_NOFLIPHW;
			if(lpddsd->ddsCaps.dwCaps&DDSCAPS_OVERLAY)
				return DDERR_NOOVERLAYHW;

			if(lpddsd->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE)
			{
				Ret=Tmp->IOriginal->Vtbl->CreateSurface((LPDIRECTDRAW4)Tmp->IOriginal,lpddsd,&DDSurf,(LPUNKNOWN)p4);
				if(Ret!=DD_OK)
					return Ret;

				memset(&ddt,0,sizeof(ddt));
				ddt.dwSize=sizeof(ddt);
				DDSurf->lpVtbl->GetSurfaceDesc(DDSurf,&ddt);
				ddt.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
				ddt.ddpfPixelFormat.dwFlags=DDPF_RGB;
				ddt.ddpfPixelFormat.dwRGBBitCount=DDE->Bpp;
				switch (DDE->Bpp)
				{
				case 15:
					ddt.lPitch/=2;
					ddt.ddpfPixelFormat.dwRBitMask=0x7c00;
					ddt.ddpfPixelFormat.dwGBitMask=0x3E0;
					ddt.ddpfPixelFormat.dwBBitMask=0x1f;
					break;
				case 16:
					ddt.lPitch/=2;
					ddt.ddpfPixelFormat.dwRBitMask=0xf800;
					ddt.ddpfPixelFormat.dwGBitMask=0x7E0;
					ddt.ddpfPixelFormat.dwBBitMask=0x1f;
					break;
				case 24:
				case 32:
					ddt.ddpfPixelFormat.dwRBitMask=0xFF0000;
					ddt.ddpfPixelFormat.dwGBitMask=0xFF00;
					ddt.ddpfPixelFormat.dwBBitMask=0xFF;
					break;
				case 8:
					ddt.lPitch/=4;
					ddt.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
					// Fall through
				default:
					ddt.ddpfPixelFormat.dwRBitMask=0;
					ddt.ddpfPixelFormat.dwGBitMask=0;
					ddt.ddpfPixelFormat.dwBBitMask=0;
					break;
				}
				lpddsd=&ddt;
				FakeSurface=true;
				lpddsd->ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
				lpddsd->dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT; 
				lpddsd->lpSurface=malloc(lpddsd->lPitch*lpddsd->dwHeight*ddt.ddpfPixelFormat.dwRGBBitCount/8);	// TODO: this memory is leaked
			}
		}
		if((lpddsd->dwFlags&DDSD_PIXELFORMAT)==0)		// if user does not specify pixel format - set it to be as the emulated display mode 
		{
			ddt=*lpddsd;
			lpddsd=&ddt;
			lpddsd->ddsCaps.dwCaps&=~DDSCAPS_VIDEOMEMORY;
			lpddsd->ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;
			ddt.dwFlags|=DDSD_PIXELFORMAT;
			ddt.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
			ddt.ddpfPixelFormat.dwFlags=DDPF_RGB;
			ddt.ddpfPixelFormat.dwRGBBitCount=DDE->Bpp;
			switch (DDE->Bpp)
			{
			case 15:
				ddt.lPitch/=2;
				ddt.ddpfPixelFormat.dwRBitMask=0x7c00;
				ddt.ddpfPixelFormat.dwGBitMask=0x3E0;
				ddt.ddpfPixelFormat.dwBBitMask=0x1f;
				break;
			case 16:
				ddt.lPitch/=2;
				ddt.ddpfPixelFormat.dwRBitMask=0xf800;
				ddt.ddpfPixelFormat.dwGBitMask=0x7E0;
				ddt.ddpfPixelFormat.dwBBitMask=0x1f;
				break;
			case 24:
			case 32:
				ddt.ddpfPixelFormat.dwRBitMask=0xFF0000;
				ddt.ddpfPixelFormat.dwGBitMask=0xFF00;
				ddt.ddpfPixelFormat.dwBBitMask=0xFF;
				break;
			case 8:
				ddt.lPitch/=4;
				ddt.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
				// Fall through
			default:
				ddt.ddpfPixelFormat.dwRBitMask=0;
				ddt.ddpfPixelFormat.dwGBitMask=0;
				ddt.ddpfPixelFormat.dwBBitMask=0;
				break;
			}
		}
	}

	Ret=Tmp->IOriginal->Vtbl->CreateSurface((LPDIRECTDRAW4)Tmp->IOriginal,lpddsd,lplpDDS,(LPUNKNOWN)p4);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawSurfaceVtbl> *DDS=new ICallbackUnknown<IDirectDrawSurfaceVtbl>(*lplpDDS);
	*lplpDDS=(LPDIRECTDRAWSURFACE4)DDS;

	if(FakeSurface)
	{
		DDS_Emul *DDSE=new DDS_Emul;
		DDSE->Screen=(IDirectDrawSurface*)DDSurf;
		DDSE->DDraw=(LPDIRECTDRAW)Tmp->IOriginal;
		DDSE->Hwnd=DDE->Hwnd;
		DDS->Data=DDSE;
	}

	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,QueryInterface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,AddAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,AddOverlayDirtyRect)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurface4Vtbl,Blt)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurface4Vtbl,BltBatch)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurface4Vtbl,BltFast)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,DeleteAttachedSurface)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,EnumAttachedSurfaces)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurface4Vtbl,EnumOverlayZOrders)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,Flip)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,GetAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetBltStatus)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetCaps)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,GetColorKey)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetDC)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetFlipStatus)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,GetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetPixelFormat)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,GetSurfaceDesc)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,Initialize)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurface4Vtbl,IsLost)
	DEFINE_CALLBACK5(DDS,IDirectDrawSurface4Vtbl,Lock)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,ReleaseDC)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurface4Vtbl,Restore)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,SetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,SetColorKey)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,SetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,SetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,Unlock)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurface4Vtbl,UpdateOverlay)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurface4Vtbl,UpdateOverlayDisplay)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurface4Vtbl,UpdateOverlayZOrder)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD7_CreateSurface(DW *R) 
{
	return DD4_CreateSurface(R);
}

DEFINE_COMFUNC4(DDC,IDirectDrawClipperVtbl,GetClipList)
DEFINE_COMFUNC2(DDC,IDirectDrawClipperVtbl,GetHWnd)

EXTERN_C DW COMSTUB_EXPORT DDC_Initialize(DW *R) 
{
	ICallbackUnknown<IDirectDrawVtbl> *TmpD=(ICallbackUnknown<IDirectDrawVtbl>*)p2;
	ICallbackUnknown<IDirectDrawClipperVtbl> *Tmp=(ICallbackUnknown<IDirectDrawClipperVtbl>*)p1;
	return Tmp->IOriginal->Vtbl->Initialize((LPDIRECTDRAWCLIPPER)Tmp->IOriginal,(LPDIRECTDRAW)TmpD->IOriginal,p3);
}

DEFINE_COMFUNC2(DDC,IDirectDrawClipperVtbl,IsClipListChanged)
DEFINE_COMFUNC3(DDC,IDirectDrawClipperVtbl,SetClipList)
DEFINE_COMFUNC3(DDC,IDirectDrawClipperVtbl,SetHWnd)

EXTERN_C DW COMSTUB_EXPORT DD_CreateClipper(DW *R) 
{
	LPDIRECTDRAWCLIPPER * lplpDDC=(LPDIRECTDRAWCLIPPER*)p3;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->CreateClipper((LPDIRECTDRAW)Tmp->IOriginal,p2,lplpDDC,(LPUNKNOWN)p4);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawClipperVtbl> *DDC=new ICallbackUnknown<IDirectDrawClipperVtbl>(*lplpDDC);
	*lplpDDC=(LPDIRECTDRAWCLIPPER)DDC;

	DEFINE_CALLBACK4(DDC,IDirectDrawClipperVtbl,GetClipList)
	DEFINE_CALLBACK2(DDC,IDirectDrawClipperVtbl,GetHWnd)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,Initialize)
	DEFINE_CALLBACK2(DDC,IDirectDrawClipperVtbl,IsClipListChanged)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,SetClipList)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,SetHWnd)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_CreateClipper(DW *R) 
{
	return DD_CreateClipper(R);
}

EXTERN_C DW COMSTUB_EXPORT DD4_CreateClipper(DW *R) 
{
	return DD_CreateClipper(R);
}

EXTERN_C DW COMSTUB_EXPORT DD7_CreateClipper(DW *R) 
{
	LPDIRECTDRAWCLIPPER * lplpDDC=(LPDIRECTDRAWCLIPPER*)p3;
	ICallbackUnknown<IDirectDraw7Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw7Vtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->CreateClipper((LPDIRECTDRAW7)Tmp->IOriginal,p2,lplpDDC,(LPUNKNOWN)p4);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawClipperVtbl> *DDC=new ICallbackUnknown<IDirectDrawClipperVtbl>(*lplpDDC);
	*lplpDDC=(LPDIRECTDRAWCLIPPER)DDC;

	DEFINE_CALLBACK4(DDC,IDirectDrawClipperVtbl,GetClipList)
	DEFINE_CALLBACK2(DDC,IDirectDrawClipperVtbl,GetHWnd)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,Initialize)
	DEFINE_CALLBACK2(DDC,IDirectDrawClipperVtbl,IsClipListChanged)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,SetClipList)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,SetHWnd)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_GetClipper(DW *R) 
{
	LPDIRECTDRAWCLIPPER * lplpDDC=(LPDIRECTDRAWCLIPPER*)p2;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->GetClipper((LPDIRECTDRAWSURFACE)Tmp->IOriginal,lplpDDC);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawClipperVtbl> *DDC=new ICallbackUnknown<IDirectDrawClipperVtbl>(*lplpDDC);
	*lplpDDC=(LPDIRECTDRAWCLIPPER)DDC;

	DEFINE_CALLBACK4(DDC,IDirectDrawClipperVtbl,GetClipList)
	DEFINE_CALLBACK2(DDC,IDirectDrawClipperVtbl,GetHWnd)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,Initialize)
	DEFINE_CALLBACK2(DDC,IDirectDrawClipperVtbl,IsClipListChanged)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,SetClipList)
	DEFINE_CALLBACK3(DDC,IDirectDrawClipperVtbl,SetHWnd)

	return Ret;
}

DEFINE_COMFUNC2(DDP,IDirectDrawPaletteVtbl,GetCaps)
DEFINE_COMFUNC5(DDP,IDirectDrawPaletteVtbl,GetEntries)

//DEFINE_COMFUNC4(DDP,IDirectDrawPaletteVtbl,Initialize)
EXTERN_C DW COMSTUB_EXPORT DDP_Initialize(DW *R) 
{
	ICallbackUnknown<IDirectDrawPaletteVtbl> *Tmp=(ICallbackUnknown<IDirectDrawPaletteVtbl>*)p1;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->Initialize((LPDIRECTDRAWPALETTE)Tmp->IOriginal,(LPDIRECTDRAW)Tmp1->IOriginal,p3,(LPPALETTEENTRY)p4);
}


DEFINE_COMFUNC5(DDP,IDirectDrawPaletteVtbl,SetEntries)
EXTERN_C DW COMSTUB_EXPORT DD_CreatePalette(DW *R) 
{
	LPDIRECTDRAWPALETTE * lplpDDP=(LPDIRECTDRAWPALETTE*)p4;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->CreatePalette((LPDIRECTDRAW)Tmp->IOriginal,p2,(LPPALETTEENTRY)p3,lplpDDP,(LPUNKNOWN)p5);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawPaletteVtbl> *DDP=new ICallbackUnknown<IDirectDrawPaletteVtbl>(*lplpDDP);
	*lplpDDP=(LPDIRECTDRAWPALETTE)DDP;

	DEFINE_CALLBACK2(DDP,IDirectDrawPaletteVtbl,GetCaps)
	DEFINE_CALLBACK5(DDP,IDirectDrawPaletteVtbl,GetEntries)
	DEFINE_CALLBACK4(DDP,IDirectDrawPaletteVtbl,Initialize)
	DEFINE_CALLBACK5(DDP,IDirectDrawPaletteVtbl,SetEntries)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD2_CreatePalette(DW *R) 
{
	return DD_CreatePalette(R);
}

EXTERN_C DW COMSTUB_EXPORT DD4_CreatePalette(DW *R) 
{
	return DD_CreatePalette(R);
}

EXTERN_C DW COMSTUB_EXPORT DD7_CreatePalette(DW *R) 
{
	LPDIRECTDRAWPALETTE * lplpDDP=(LPDIRECTDRAWPALETTE*)p4;
	ICallbackUnknown<IDirectDraw7Vtbl> *Tmp=(ICallbackUnknown<IDirectDraw7Vtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->CreatePalette((LPDIRECTDRAW7)Tmp->IOriginal,p2,(LPPALETTEENTRY)p3,lplpDDP,(LPUNKNOWN)p5);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawPaletteVtbl> *DDP=new ICallbackUnknown<IDirectDrawPaletteVtbl>(*lplpDDP);
	*lplpDDP=(LPDIRECTDRAWPALETTE)DDP;

	DEFINE_CALLBACK2(DDP,IDirectDrawPaletteVtbl,GetCaps)
	DEFINE_CALLBACK5(DDP,IDirectDrawPaletteVtbl,GetEntries)
	DEFINE_CALLBACK4(DDP,IDirectDrawPaletteVtbl,Initialize)
	DEFINE_CALLBACK5(DDP,IDirectDrawPaletteVtbl,SetEntries)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_GetPalette(DW *R) 
{
	LPDIRECTDRAWPALETTE * lplpDDP=(LPDIRECTDRAWPALETTE*)p2;
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->GetPalette((LPDIRECTDRAWSURFACE)Tmp->IOriginal,lplpDDP);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawPaletteVtbl> *DDP=new ICallbackUnknown<IDirectDrawPaletteVtbl>(*lplpDDP);
	*lplpDDP=(LPDIRECTDRAWPALETTE)DDP;

	DEFINE_CALLBACK2(DDP,IDirectDrawPaletteVtbl,GetCaps)
	DEFINE_CALLBACK5(DDP,IDirectDrawPaletteVtbl,GetEntries)
	DEFINE_CALLBACK4(DDP,IDirectDrawPaletteVtbl,Initialize)
	DEFINE_CALLBACK5(DDP,IDirectDrawPaletteVtbl,SetEntries)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DD_QueryInterface(DW *R);

EXTERN_C DW COMSTUB_EXPORT DD2_QueryInterface(DW *R)
{
	return DD_QueryInterface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD4_QueryInterface(DW *R)
{
	return DD_QueryInterface(R);
}

EXTERN_C DW COMSTUB_EXPORT DD7_QueryInterface(DW *R)
{
	return DD_QueryInterface(R);
}

EXTERN_C DW STUB_EXPORT yact_DirectDrawCreate(DW *R)
{
	GUID *lpGUID=(GUID*)p1;
	LPDIRECTDRAW *lplpDD=(LPDIRECTDRAW*)p2;
	IUnknown *pUnkOuter=(IUnknown*)p3;
	DWORD Ret=DirectDrawCreate(lpGUID,lplpDD,pUnkOuter);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawVtbl> *DD=new ICallbackUnknown<IDirectDrawVtbl>(*lplpDD);
	*lplpDD=(LPDIRECTDRAW)DD;
	DD_Emul *DDE=new DD_Emul;	// FIXME: memory leak
	DDE->Bpp=32;
	DDE->Hwnd=0;
	DD->Data=DDE;

	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,QueryInterface);	
	DEFINE_CALLBACK1(DD,IDirectDrawVtbl,Compact);
	DEFINE_CALLBACK4(DD,IDirectDrawVtbl,CreateClipper);
	DEFINE_CALLBACK5(DD,IDirectDrawVtbl,CreatePalette);
	DEFINE_CALLBACK4(DD,IDirectDrawVtbl,CreateSurface);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,DuplicateSurface);
	DEFINE_CALLBACK5(DD,IDirectDrawVtbl,EnumDisplayModes);
	DEFINE_CALLBACK5(DD,IDirectDrawVtbl,EnumSurfaces);
	DEFINE_CALLBACK1(DD,IDirectDrawVtbl,FlipToGDISurface);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,GetCaps);
	DEFINE_VER_CALLBACK2(12,DD,IDirectDrawVtbl,GetDisplayMode);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,GetFourCCCodes);
	DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetGDISurface);
	DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetMonitorFrequency);
	DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetScanLine);
	DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetVerticalBlankStatus);
	DEFINE_CALLBACK2(DD,IDirectDrawVtbl,Initialize);
	DEFINE_CALLBACK1(DD,IDirectDrawVtbl,RestoreDisplayMode);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,SetCooperativeLevel);
	DEFINE_CALLBACK4(DD,IDirectDrawVtbl,SetDisplayMode);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,WaitForVerticalBlank);
	
	return Ret;
}

//DEFINE_FUNC2(DirectDrawEnumerateA)
EXTERN_C DW STUB_EXPORT yact_DirectDrawEnumerateA(DW *R)
{
	return DirectDrawEnumerateA((LPDDENUMCALLBACKA)CbCreateNativeStdcallCallback(p1,4),(void*)p2);
}
//DEFINE_FUNC2(DirectDrawEnumerateW)
EXTERN_C DW STUB_EXPORT yact_DirectDrawEnumerateW(DW *R)
{
	return DirectDrawEnumerateW((LPDDENUMCALLBACKW)CbCreateNativeStdcallCallback(p1,4),(void*)p2);
}
//DEFINE_FUNC3(DirectDrawEnumerateExA)
EXTERN_C DW STUB_EXPORT yact_DirectDrawEnumerateExA(DW *R)
{
	return DirectDrawEnumerateExA((LPDDENUMCALLBACKEXA)CbCreateNativeStdcallCallback(p1,5),(void*)p2,p3);
}
//DEFINE_FUNC3(DirectDrawEnumerateExW)
EXTERN_C DW STUB_EXPORT yact_DirectDrawEnumerateExW(DW *R)
{
	return DirectDrawEnumerateExW((LPDDENUMCALLBACKEXW)CbCreateNativeStdcallCallback(p1,5),(void*)p2,p3);
}

DEFINE_COMFUNC4(DC,IDirectDrawClipperVtbl,GetClipList);
DEFINE_COMFUNC2(DC,IDirectDrawClipperVtbl,GetHWnd);
//DEFINE_COMFUNC3(DC,IDirectDrawClipperVtbl,Initialize);
EXTERN_C DW COMSTUB_EXPORT DC_Initialize(DW *R) 
{
	ICallbackUnknown<IDirectDrawClipperVtbl> *Tmp=(ICallbackUnknown<IDirectDrawClipperVtbl>*)p1;
	ICallbackUnknown<IDirectDrawVtbl> *Tmp1=(ICallbackUnknown<IDirectDrawVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->Initialize((LPDIRECTDRAWCLIPPER)Tmp->IOriginal,Tmp1?(LPDIRECTDRAW)Tmp1->IOriginal:0,p3);
}

DEFINE_COMFUNC2(DC,IDirectDrawClipperVtbl,IsClipListChanged);
DEFINE_COMFUNC3(DC,IDirectDrawClipperVtbl,SetClipList);
DEFINE_COMFUNC3(DC,IDirectDrawClipperVtbl,SetHWnd);

//DEFINE_FUNC3(DirectDrawCreateClipper)
EXTERN_C DW STUB_EXPORT yact_DirectDrawCreateClipper(DW *R)
{
	LPDIRECTDRAWCLIPPER *lplpDC=(LPDIRECTDRAWCLIPPER*)p2;
	IUnknown *pUnkOuter=(IUnknown*)p3;
	DWORD Ret=DirectDrawCreateClipper(p1,lplpDC,pUnkOuter);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDrawClipperVtbl> *DC=new ICallbackUnknown<IDirectDrawClipperVtbl>(*lplpDC);
	*lplpDC=(LPDIRECTDRAWCLIPPER)DC;

    DEFINE_CALLBACK4(DC,IDirectDrawClipperVtbl,GetClipList);
    DEFINE_CALLBACK2(DC,IDirectDrawClipperVtbl,GetHWnd);
    DEFINE_CALLBACK3(DC,IDirectDrawClipperVtbl,Initialize);
    DEFINE_CALLBACK2(DC,IDirectDrawClipperVtbl,IsClipListChanged);
    DEFINE_CALLBACK3(DC,IDirectDrawClipperVtbl,SetClipList);
    DEFINE_CALLBACK3(DC,IDirectDrawClipperVtbl,SetHWnd);
	
	return Ret;
}

HRESULT WINAPI EnumSurfacesCallback( 
  LPDIRECTDRAWSURFACE lpDDSurface, 
  LPDDSURFACEDESC lpDDSurfaceDesc, 
  LPVOID lpContext
)
{
	// TODO: untested!
	DWORD *Ctx=(DWORD*)lpContext;

	ICallbackUnknown<IDirectDrawSurfaceVtbl> *DDS=new ICallbackUnknown<IDirectDrawSurfaceVtbl>(lpDDSurface);
	lpDDSurface=(LPDIRECTDRAWSURFACE)DDS;

	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,QueryInterface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,AddOverlayDirtyRect)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,Blt)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,BltBatch)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,BltFast)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,DeleteAttachedSurface)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,EnumAttachedSurfaces)
	DEFINE_CALLBACK4(DDS,IDirectDrawSurfaceVtbl,EnumOverlayZOrders)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Flip)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetAttachedSurface)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetBltStatus)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetCaps)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetColorKey)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetDC)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetFlipStatus)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,GetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetPixelFormat)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,GetSurfaceDesc)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,Initialize)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,IsLost)
	DEFINE_CALLBACK5(DDS,IDirectDrawSurfaceVtbl,Lock)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,ReleaseDC)
	DEFINE_CALLBACK1(DDS,IDirectDrawSurfaceVtbl,Restore)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetClipper)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetColorKey)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,SetOverlayPosition)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,SetPalette)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,Unlock)
	DEFINE_CALLBACK6(DDS,IDirectDrawSurfaceVtbl,UpdateOverlay)
	DEFINE_CALLBACK2(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayDisplay)
	DEFINE_CALLBACK3(DDS,IDirectDrawSurfaceVtbl,UpdateOverlayZOrder)

	DWORD Ret;
	Ret=EmuExecute(Ctx[1],3,DDS,lpDDSurfaceDesc,Ctx[0]);
	return (HRESULT)Ret;
}

HRESULT WINAPI EnumSurfacesCallback2( 
  LPDIRECTDRAWSURFACE4 lpDDSurface, 
  LPDDSURFACEDESC2 lpDDSurfaceDesc, 
  LPVOID lpContext
)
{
	// TODO!!
	return S_OK;
}

HRESULT WINAPI EnumSurfacesCallback7( 
  LPDIRECTDRAWSURFACE7 lpDDSurface, 
  LPDDSURFACEDESC2 lpDDSurfaceDesc, 
  LPVOID lpContext
)
{
	// TODO!!
	return S_OK;
}

EXTERN_C DW STUB_EXPORT yact_DirectDrawCreateEx(DW *R)
{
	GUID *lpGUID=(GUID*)p1;
	LPDIRECTDRAW7 *lplpDD=(LPDIRECTDRAW7*)p2;
	IID *lpIID=(GUID*)p3;
	IUnknown *pUnkOuter=(IUnknown*)p4;
	DWORD Ret;
	if(pUnkOuter==(IUnknown*)0xFFFFFFFF)	// special magic to create DD via COM
		Ret=CoCreateInstance(CLSID_DirectDraw, NULL, CLSCTX_ALL, IID_IDirectDraw7, (LPVOID*)lplpDD);
	else
		Ret=DirectDrawCreateEx(lpGUID,(LPVOID*)lplpDD,*lpIID,pUnkOuter);
	if(Ret!=DD_OK)
		return Ret;

	ICallbackUnknown<IDirectDraw7Vtbl> *DD=new ICallbackUnknown<IDirectDraw7Vtbl>(*lplpDD);
	ICallbackUnknown<IDirectDraw7Vtbl> *DD7=DD;
	*lplpDD=(LPDIRECTDRAW7)DD;
	DD_Emul *DDE=new DD_Emul;	// FIXME: memory leak
	DDE->Bpp=0;
	DDE->Hwnd=0;
	DD7->Data=DDE;
	
	DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,QueryInterface);	// !!!
	DEFINE_CALLBACK1(DD,IDirectDrawVtbl,Compact);
	DEFINE_CALLBACK4(DD7,IDirectDraw7Vtbl,CreateClipper);
	DEFINE_CALLBACK5(DD7,IDirectDraw7Vtbl,CreatePalette);
	DEFINE_CALLBACK4(DD7,IDirectDraw7Vtbl,CreateSurface);
	DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,DuplicateSurface);
	DEFINE_CALLBACK5(DD7,IDirectDraw7Vtbl,EnumDisplayModes);
	DEFINE_CALLBACK5(DD7,IDirectDraw7Vtbl,EnumSurfaces);
	DEFINE_CALLBACK1(DD,IDirectDrawVtbl,FlipToGDISurface);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,GetCaps);
	DEFINE_VER_CALLBACK2(47,DD,IDirectDrawVtbl,GetDisplayMode);
	DEFINE_CALLBACK3(DD,IDirectDrawVtbl,GetFourCCCodes);
	DEFINE_CALLBACK2(DD7,IDirectDraw7Vtbl,GetGDISurface);
	DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,GetMonitorFrequency);
	DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,GetScanLine);
	DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,GetVerticalBlankStatus);
	DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,Initialize);
	DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,RestoreDisplayMode);
	DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,SetCooperativeLevel);
	DEFINE_VER_CALLBACK4(2,DD,IDirectDraw7Vtbl,SetDisplayMode);
	DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,WaitForVerticalBlank);
	
	DEFINE_CALLBACK4(DD,IDirectDraw7Vtbl,GetAvailableVidMem);
	DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,GetSurfaceFromDC);
	DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,RestoreAllSurfaces);
	DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,TestCooperativeLevel);
	DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,GetDeviceIdentifier);
	DEFINE_CALLBACK4(DD,IDirectDraw7Vtbl,StartModeTest);
	DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,EvaluateMode);

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DDS_QueryInterface(DW *R)
{
	ICallbackUnknown<IDirectDrawSurfaceVtbl> *Tmp=(ICallbackUnknown<IDirectDrawSurfaceVtbl>*)p1;
	IID *iid=(IID*)p1;
	if(*iid==IID_IDirectDrawSurface)
	{
		LPDIRECTDRAWSURFACE *lplpDD=(LPDIRECTDRAWSURFACE*)p2;
		DWORD Ret=Tmp->IOriginal->Vtbl->QueryInterface((LPDIRECTDRAWSURFACE)Tmp->IOriginal,*iid,(LPVOID*)lplpDD);
		return Ret;
	}
	return E_NOINTERFACE;
}

EXTERN_C DW COMSTUB_EXPORT DD_QueryInterface(DW *R)
{
	ICallbackUnknown<IDirectDrawVtbl> *Tmp=(ICallbackUnknown<IDirectDrawVtbl>*)p1;
	IID *iid=(IID*)p1;
	if(*iid==IID_IDirectDraw)
	{
		LPDIRECTDRAW *lplpDD=(LPDIRECTDRAW*)p2;
		DWORD Ret=Tmp->IOriginal->Vtbl->QueryInterface((LPDIRECTDRAW)Tmp->IOriginal,*iid,(LPVOID*)lplpDD);
		if(Ret!=DD_OK)
			return Ret;
		ICallbackUnknown<IDirectDrawVtbl> *DD=new ICallbackUnknown<IDirectDrawVtbl>(*lplpDD);
		*lplpDD=(LPDIRECTDRAW)DD;
		DD_Emul *DDE=new DD_Emul;	// FIXME: memory leak
		DDE->Bpp=0;
		DDE->Hwnd=0;
		DD->Data=DDE;

		DEFINE_CALLBACK3(DD,IDirectDrawVtbl,QueryInterface);
		DEFINE_CALLBACK1(DD,IDirectDrawVtbl,Compact);
		DEFINE_CALLBACK4(DD,IDirectDrawVtbl,CreateClipper);
		DEFINE_CALLBACK5(DD,IDirectDrawVtbl,CreatePalette);
		DEFINE_CALLBACK4(DD,IDirectDrawVtbl,CreateSurface);
		DEFINE_CALLBACK3(DD,IDirectDrawVtbl,DuplicateSurface);
		DEFINE_CALLBACK5(DD,IDirectDrawVtbl,EnumDisplayModes);
		DEFINE_CALLBACK5(DD,IDirectDrawVtbl,EnumSurfaces);
		DEFINE_CALLBACK1(DD,IDirectDrawVtbl,FlipToGDISurface);
		DEFINE_CALLBACK3(DD,IDirectDrawVtbl,GetCaps);
		DEFINE_VER_CALLBACK2(12,DD,IDirectDrawVtbl,GetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDrawVtbl,GetFourCCCodes);
		DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetGDISurface);
		DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetMonitorFrequency);
		DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetScanLine);
		DEFINE_CALLBACK2(DD,IDirectDrawVtbl,GetVerticalBlankStatus);
		DEFINE_CALLBACK2(DD,IDirectDrawVtbl,Initialize);
		DEFINE_CALLBACK1(DD,IDirectDrawVtbl,RestoreDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDrawVtbl,SetCooperativeLevel);
		DEFINE_CALLBACK4(DD,IDirectDrawVtbl,SetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDrawVtbl,WaitForVerticalBlank);
	}
	if(*iid==IID_IDirectDraw2)
	{
		LPDIRECTDRAW2 *lplpDD=(LPDIRECTDRAW2*)p2;
		DWORD Ret=Tmp->IOriginal->Vtbl->QueryInterface((LPDIRECTDRAW)Tmp->IOriginal,*iid,(LPVOID*)lplpDD);
		if(Ret!=DD_OK)
			return Ret;
		ICallbackUnknown<IDirectDraw2Vtbl> *DD2=new ICallbackUnknown<IDirectDraw2Vtbl>(*lplpDD);
		ICallbackUnknown<IDirectDraw2Vtbl> *DD=DD2;
		*lplpDD=(LPDIRECTDRAW2)DD2;
		DD_Emul *DDE=new DD_Emul;	// FIXME: memory leak
		DDE->Bpp=0;
		DDE->Hwnd=0;
		DD2->Data=DDE;

		DEFINE_CALLBACK3(DD2,IDirectDraw2Vtbl,QueryInterface);
		DEFINE_CALLBACK1(DD,IDirectDraw2Vtbl,Compact);
		DEFINE_CALLBACK4(DD2,IDirectDraw2Vtbl,CreateClipper);
		DEFINE_CALLBACK5(DD2,IDirectDraw2Vtbl,CreatePalette);
		DEFINE_CALLBACK4(DD2,IDirectDraw2Vtbl,CreateSurface);
		DEFINE_CALLBACK3(DD2,IDirectDraw2Vtbl,DuplicateSurface);
		DEFINE_CALLBACK5(DD2,IDirectDraw2Vtbl,EnumDisplayModes);
		DEFINE_CALLBACK5(DD2,IDirectDraw2Vtbl,EnumSurfaces);
		DEFINE_CALLBACK1(DD,IDirectDraw2Vtbl,FlipToGDISurface);
		DEFINE_CALLBACK3(DD,IDirectDraw2Vtbl,GetCaps);
		DEFINE_VER_CALLBACK2(12,DD,IDirectDraw2Vtbl,GetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDraw2Vtbl,GetFourCCCodes);
		DEFINE_CALLBACK2(DD2,IDirectDraw2Vtbl,GetGDISurface);
		DEFINE_CALLBACK2(DD,IDirectDraw2Vtbl,GetMonitorFrequency);
		DEFINE_CALLBACK2(DD,IDirectDraw2Vtbl,GetScanLine);
		DEFINE_CALLBACK2(DD,IDirectDraw2Vtbl,GetVerticalBlankStatus);
		DEFINE_CALLBACK2(DD,IDirectDraw2Vtbl,Initialize);
		DEFINE_CALLBACK1(DD,IDirectDraw2Vtbl,RestoreDisplayMode);
		DEFINE_CALLBACK3(DD2,IDirectDraw2Vtbl,SetCooperativeLevel);
		DEFINE_CALLBACK4(DD2,IDirectDraw2Vtbl,SetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDraw2Vtbl,WaitForVerticalBlank);
		DEFINE_CALLBACK4(DD,IDirectDraw2Vtbl,GetAvailableVidMem);
	}
	if(*iid==IID_IDirectDraw4)
	{
		LPDIRECTDRAW4 *lplpDD=(LPDIRECTDRAW4*)p2;
		DWORD Ret=Tmp->IOriginal->Vtbl->QueryInterface((LPDIRECTDRAW)Tmp->IOriginal,*iid,(LPVOID*)lplpDD);
		if(Ret!=DD_OK)
			return Ret;
		ICallbackUnknown<IDirectDraw4Vtbl> *DD4=new ICallbackUnknown<IDirectDraw4Vtbl>(*lplpDD);
		ICallbackUnknown<IDirectDraw4Vtbl> *DD=DD4;
		*lplpDD=(LPDIRECTDRAW4)DD4;
		DD_Emul *DDE=new DD_Emul;	// FIXME: memory leak
		DDE->Bpp=0;
		DDE->Hwnd=0;
		DD4->Data=DDE;

		DEFINE_CALLBACK3(DD4,IDirectDraw4Vtbl,QueryInterface);
		DEFINE_CALLBACK1(DD,IDirectDraw4Vtbl,Compact);
		DEFINE_CALLBACK4(DD4,IDirectDraw4Vtbl,CreateClipper);
		DEFINE_CALLBACK5(DD4,IDirectDraw4Vtbl,CreatePalette);
		DEFINE_CALLBACK4(DD4,IDirectDraw4Vtbl,CreateSurface);
		DEFINE_CALLBACK3(DD4,IDirectDraw4Vtbl,DuplicateSurface);
		DEFINE_CALLBACK5(DD4,IDirectDraw4Vtbl,EnumDisplayModes);
		DEFINE_CALLBACK5(DD4,IDirectDraw4Vtbl,EnumSurfaces);
		DEFINE_CALLBACK1(DD,IDirectDraw4Vtbl,FlipToGDISurface);
		DEFINE_CALLBACK3(DD,IDirectDraw4Vtbl,GetCaps);
		DEFINE_VER_CALLBACK2(47,DD,IDirectDraw4Vtbl,GetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDraw4Vtbl,GetFourCCCodes);
		DEFINE_CALLBACK2(DD4,IDirectDraw4Vtbl,GetGDISurface);
		DEFINE_CALLBACK2(DD,IDirectDraw4Vtbl,GetMonitorFrequency);
		DEFINE_CALLBACK2(DD,IDirectDraw4Vtbl,GetScanLine);
		DEFINE_CALLBACK2(DD,IDirectDraw4Vtbl,GetVerticalBlankStatus);
		DEFINE_CALLBACK2(DD,IDirectDraw4Vtbl,Initialize);
		DEFINE_CALLBACK1(DD,IDirectDraw4Vtbl,RestoreDisplayMode);
		DEFINE_CALLBACK3(DD4,IDirectDraw4Vtbl,SetCooperativeLevel);
		DEFINE_VER_CALLBACK4(2,DD,IDirectDraw7Vtbl,SetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDraw4Vtbl,WaitForVerticalBlank);
		DEFINE_CALLBACK4(DD,IDirectDraw4Vtbl,GetAvailableVidMem);
		DEFINE_CALLBACK3(DD4,IDirectDraw4Vtbl,GetSurfaceFromDC);
		DEFINE_CALLBACK1(DD,IDirectDraw4Vtbl,RestoreAllSurfaces);
		DEFINE_CALLBACK1(DD,IDirectDraw4Vtbl,TestCooperativeLevel);
		DEFINE_CALLBACK3(DD,IDirectDraw4Vtbl,GetDeviceIdentifier);
	}
	if(*iid==IID_IDirectDraw7)
	{
		LPDIRECTDRAW7 *lplpDD=(LPDIRECTDRAW7*)p2;
		DWORD Ret=Tmp->IOriginal->Vtbl->QueryInterface((LPDIRECTDRAW)Tmp->IOriginal,*iid,(LPVOID*)lplpDD);
		if(Ret!=DD_OK)
			return Ret;
		ICallbackUnknown<IDirectDraw7Vtbl> *DD7=new ICallbackUnknown<IDirectDraw7Vtbl>(*lplpDD);
		ICallbackUnknown<IDirectDraw7Vtbl> *DD=DD7;
		*lplpDD=(LPDIRECTDRAW7)DD7;
		DD_Emul *DDE=new DD_Emul;	// FIXME: memory leak
		DDE->Bpp=0;
		DDE->Hwnd=0;
		DD7->Data=DDE;

		DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,QueryInterface);
		DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,Compact);
		DEFINE_CALLBACK4(DD7,IDirectDraw7Vtbl,CreateClipper);
		DEFINE_CALLBACK5(DD7,IDirectDraw7Vtbl,CreatePalette);
		DEFINE_CALLBACK4(DD7,IDirectDraw7Vtbl,CreateSurface);
		DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,DuplicateSurface);
		DEFINE_CALLBACK5(DD7,IDirectDraw7Vtbl,EnumDisplayModes);
		DEFINE_CALLBACK5(DD7,IDirectDraw7Vtbl,EnumSurfaces);
		DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,FlipToGDISurface);
		DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,GetCaps);
		DEFINE_VER_CALLBACK2(47,DD,IDirectDraw7Vtbl,GetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,GetFourCCCodes);
		DEFINE_CALLBACK2(DD7,IDirectDraw7Vtbl,GetGDISurface);
		DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,GetMonitorFrequency);
		DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,GetScanLine);
		DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,GetVerticalBlankStatus);
		DEFINE_CALLBACK2(DD,IDirectDraw7Vtbl,Initialize);
		DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,RestoreDisplayMode);
		DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,SetCooperativeLevel);
		DEFINE_VER_CALLBACK4(2,DD,IDirectDraw7Vtbl,SetDisplayMode);
		DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,WaitForVerticalBlank);
		DEFINE_CALLBACK4(DD,IDirectDraw7Vtbl,GetAvailableVidMem);
		DEFINE_CALLBACK3(DD7,IDirectDraw7Vtbl,GetSurfaceFromDC);
		DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,RestoreAllSurfaces);
		DEFINE_CALLBACK1(DD,IDirectDraw7Vtbl,TestCooperativeLevel);
		DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,GetDeviceIdentifier);
		DEFINE_CALLBACK4(DD,IDirectDraw7Vtbl,StartModeTest);
		DEFINE_CALLBACK3(DD,IDirectDraw7Vtbl,EvaluateMode);
	}
	return E_NOINTERFACE;
}
