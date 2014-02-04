// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

DWORD MinWidth=-1;
DWORD MinHeight=-1;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			MinWidth=GetSystemMetrics(SM_CXSCREEN);
			MinHeight=GetSystemMetrics(SM_CYSCREEN);
			DEVMODE dm = { 0 };
			dm.dmSize = sizeof(dm);
			for( int iModeNum = 0; EnumDisplaySettings( NULL, iModeNum, &dm ) != 0; iModeNum++ ) 
			{
				if(dm.dmPelsHeight<=MinHeight)
				{
					if(dm.dmPelsHeight==MinHeight)
					{
						if(dm.dmPelsWidth<MinWidth)
							MinWidth=dm.dmPelsWidth;
					} else
						MinWidth=dm.dmPelsWidth;

					MinHeight=dm.dmPelsHeight;
				}
			}	
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

