#define CINTERFACE	// I need C interface for DDraw for lpVtbl member
#include "dllyact_def.h"
#include "comobject_def.h"
#include <windows.h>
#include <ddraw.h>
#include <assert.h>
#include "classes.h"
#include "callbacks.h"
#include "ddraw_common.h"
#include "x86emul.h"

// Code borrowed from http://www.codeproject.com/Articles/71/DirectDraw-extension
HRESULT BltEx(IDirectDrawSurface *pDestSurface, RECT *destRect, IDirectDrawSurface *pSourceSurface, RECT *sourceRect, 
			DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
   RECT r1,r2;
   // blit void surfaces : just kidding or....
   if (pSourceSurface==NULL || pDestSurface==NULL)
	  return E_FAIL; 
	
   DDSURFACEDESC2 ddsdDest,ddsdSource;
   ::ZeroMemory(&ddsdDest, sizeof(ddsdDest));
   ddsdDest.dwSize = sizeof(ddsdDest);    
   ::ZeroMemory(&ddsdSource, sizeof(ddsdSource));
   ddsdSource.dwSize = sizeof(ddsdSource);    
   HRESULT hr = pDestSurface->lpVtbl->GetSurfaceDesc(pDestSurface,(DDSURFACEDESC*)&ddsdDest);
   if(FAILED(hr))
   {
	   ddsdDest.dwSize = sizeof(DDSURFACEDESC);    
	   ddsdSource.dwSize = sizeof(DDSURFACEDESC);    
	   hr = pDestSurface->lpVtbl->GetSurfaceDesc(pDestSurface,(DDSURFACEDESC*)&ddsdDest);
   }
   if (SUCCEEDED(hr))
   {
	  if(destRect==0)
	  {
		  destRect=&r1;
		  r1.left=r1.top=0;
		  r1.right=ddsdDest.dwWidth;
		  r1.bottom=ddsdDest.dwHeight;
	  }
      hr = pSourceSurface->lpVtbl->GetSurfaceDesc(pSourceSurface,(DDSURFACEDESC*)&ddsdSource);
      if (SUCCEEDED(hr))
      {
		if(sourceRect==0)
		{
			sourceRect=&r2;
			r2.left=r2.top=0;
			r2.right=ddsdSource.dwWidth;
			r2.bottom=ddsdSource.dwHeight;
		}
         // check the two pixel formats (surface type & bit count)
        BOOL bSamePixelFormat=
		 (ddsdDest.ddpfPixelFormat.dwFlags==ddsdSource.ddpfPixelFormat.dwFlags) &&
		 (ddsdDest.ddpfPixelFormat.dwRGBBitCount==ddsdSource.ddpfPixelFormat.dwRGBBitCount);
        if (!bSamePixelFormat)
        {
			// perform the color blit using the GDI (implicitely will remap colors)
			HDC dcDest,dcSource;
			hr=pSourceSurface->lpVtbl->GetDC(pSourceSurface,&dcSource);
		    if(!SUCCEEDED(hr))
				return hr;
			hr=pDestSurface->lpVtbl->GetDC(pDestSurface,&dcDest);
		    if(!SUCCEEDED(hr))
			{
				pSourceSurface->lpVtbl->ReleaseDC(pSourceSurface,dcSource);
				return hr;
			}

			::StretchBlt(dcDest, destRect->left,destRect->top, 
				destRect->right-destRect->left,  destRect->bottom-destRect->top,
				dcSource, sourceRect->left,sourceRect->top, 
				sourceRect->right-sourceRect->left, sourceRect->bottom-sourceRect->top,
				SRCCOPY);

			pDestSurface->lpVtbl->ReleaseDC(pDestSurface,dcDest);
			pSourceSurface->lpVtbl->ReleaseDC(pSourceSurface,dcSource);

			return S_OK;
		} else	   // common blit allowed between clonable surfaces
		{
			DDBLTFX  ddbltfx;  
			if(lpDDBltFx==0)
			{
				ZeroMemory(&ddbltfx, sizeof(ddbltfx));
				ddbltfx.dwSize = sizeof(ddbltfx);
				lpDDBltFx=&ddbltfx;
			}
						
			return pDestSurface->lpVtbl->Blt(pDestSurface,destRect, pSourceSurface, sourceRect, dwFlags, lpDDBltFx);
		}
     } 
   }
   return hr;
}

DEFINE_COMFUNC1(DD,IDirectDrawVtbl,Compact);
DEFINE_COMFUNC1(DD,IDirectDrawVtbl,FlipToGDISurface);
DEFINE_COMFUNC3(DD,IDirectDrawVtbl,GetCaps);
DEFINE_COMFUNC3(DD,IDirectDrawVtbl,GetFourCCCodes);
DEFINE_COMFUNC2(DD,IDirectDrawVtbl,GetMonitorFrequency);
DEFINE_COMFUNC2(DD,IDirectDrawVtbl,GetScanLine);
DEFINE_COMFUNC2(DD,IDirectDrawVtbl,GetVerticalBlankStatus);
DEFINE_COMFUNC2(DD,IDirectDrawVtbl,Initialize);
DEFINE_COMFUNC1(DD,IDirectDrawVtbl,RestoreDisplayMode);
DEFINE_COMFUNC3(DD,IDirectDrawVtbl,WaitForVerticalBlank);
DEFINE_COMFUNC4(DD,IDirectDraw2Vtbl,GetAvailableVidMem);
DEFINE_COMFUNC1(DD,IDirectDraw4Vtbl,RestoreAllSurfaces);
DEFINE_COMFUNC1(DD,IDirectDraw4Vtbl,TestCooperativeLevel);
DEFINE_COMFUNC3(DD,IDirectDraw4Vtbl,GetDeviceIdentifier);
DEFINE_COMFUNC4(DD,IDirectDraw7Vtbl,StartModeTest);
DEFINE_COMFUNC3(DD,IDirectDraw7Vtbl,EvaluateMode);

DEFINE_COMFUNC2(DD12,IDirectDrawVtbl,GetDisplayMode);
DEFINE_COMFUNC2(DD47,IDirectDraw4Vtbl,GetDisplayMode);
