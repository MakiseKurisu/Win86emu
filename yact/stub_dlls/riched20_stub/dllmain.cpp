// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

extern "C" void __stdcall stub_Dummy(void);

DWORD WINAPI ThreadProc(
  _In_  LPVOID lpParameter
)
{
	stub_Dummy();	// call the dummy function to get native riched20.dll loaded
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CloseHandle(CreateThread(0,0,ThreadProc,0,0,0));
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

