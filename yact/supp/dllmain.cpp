// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <util.h>
#include "../supp/logging.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Do not open the log file from DllMain (loader lock): just report the
		// load to the debug output. The file is opened on the first log call
		// or by InitDebugLog, which x86_emu.dll calls from EmuInitialize.
		OutputDebugStringA("[supp.nt.dll] loaded\n");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		// Flush and close the log file (writes the shutdown line).
		ShutdownDebugLog();
		break;
	}
	return TRUE;
}

