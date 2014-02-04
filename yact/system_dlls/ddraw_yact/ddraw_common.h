// Common DirectDraw prototypes, structures, etc

#ifndef __DD_COMMON_H
#define __DD_COMMON_H


struct DD_Emul
{
	HWND Hwnd;	// main program window from SetCoopLevel
	DWORD Bpp;	// the requested BPP
};

struct DDS_Emul
{
	HWND Hwnd;	// main program window from SetCoopLevel
	IDirectDraw *DDraw;
	IDirectDrawSurface *Screen;	// The "real" surface, 32 bpp
};

extern DWORD MinWidth;
extern DWORD MinHeight;

HRESULT BltEx(IDirectDrawSurface *pDestSurface, RECT *destRect, IDirectDrawSurface *pSourceSurface, RECT *sourceRect, 
			DWORD dwFlags, LPDDBLTFX lpDDBltFx);

PROTO_COMFUNC1(DD,IDirectDrawVtbl,Compact);
PROTO_COMFUNC1(DD,IDirectDrawVtbl,FlipToGDISurface);
PROTO_COMFUNC3(DD,IDirectDrawVtbl,GetCaps);
PROTO_COMFUNC3(DD,IDirectDrawVtbl,GetFourCCCodes);
PROTO_COMFUNC2(DD,IDirectDrawVtbl,GetMonitorFrequency);
PROTO_COMFUNC2(DD,IDirectDrawVtbl,GetScanLine);
PROTO_COMFUNC2(DD,IDirectDrawVtbl,GetVerticalBlankStatus);
PROTO_COMFUNC2(DD,IDirectDrawVtbl,Initialize);
PROTO_COMFUNC1(DD,IDirectDrawVtbl,RestoreDisplayMode);
PROTO_COMFUNC3(DD,IDirectDrawVtbl,WaitForVerticalBlank);
PROTO_COMFUNC4(DD,IDirectDraw2Vtbl,GetAvailableVidMem);
PROTO_COMFUNC1(DD,IDirectDraw4Vtbl,RestoreAllSurfaces);
PROTO_COMFUNC1(DD,IDirectDraw4Vtbl,TestCooperativeLevel);
PROTO_COMFUNC3(DD,IDirectDraw4Vtbl,GetDeviceIdentifier);
PROTO_COMFUNC4(DD,IDirectDraw7Vtbl,StartModeTest);
PROTO_COMFUNC3(DD,IDirectDraw7Vtbl,EvaluateMode);

PROTO_COMFUNC2(DD12,IDirectDrawVtbl,GetDisplayMode);
PROTO_COMFUNC2(DD47,IDirectDraw4Vtbl,GetDisplayMode);

#endif //__DD_COMMON_H
