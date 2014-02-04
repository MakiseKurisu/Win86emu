// yactemu_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <peldr.h>
#include <x86emul.h>
#include <yact_emu\yact_emu.h>

void MyTlsCB(PE_HANDLE Pe, PIMAGE_TLS_CALLBACK Ptr, DWORD Reason,LPVOID Param)
{
	printf("CB: %08X\n",Ptr);
//	if(Ptr)
//		EmuExecute((DWORD)Ptr, 3,PeLdrGetModuleBase(Pe),Reason,(DWORD)Param);
}

int _tmain(int argc, _TCHAR* argv[])
{
	EmuInitialize();
	PeLdrSetExecutionCallback(MyTlsCB,0);

	PE_HANDLE PE=PeLdrLoadModule(L"U:\\Heroes\\HEROESW.EXE");

	if(PE==0)
	{
		printf("GetLastError()=%d\n\n",GetLastError());
	} else
	{
		typedef int t_WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
		t_WinMain *WM=(t_WinMain*)PeLdrGetEntryPoint(PE);

		EmuExecute((DWORD)WM,4,PeLdrGetModuleBase(PE),0,(DWORD)GetCommandLineA(),SW_SHOWNORMAL);
	}

	PeLdrCloseAllModules();
	return 0;
}

