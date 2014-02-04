#include "stdafx.h"
#include <peldr.h>
#include <peldr\i_peldr.h>
#include <dbghelp.h>
#include <x86emul.h>
#include <util.h>
#include <winternl.h>
#include <stdlib.h>

static void MyTlsCB(PE_HANDLE Pe, PIMAGE_TLS_CALLBACK Ptr, DWORD Reason,LPVOID Param)
{
//	printf("CB: %08X\n",Ptr);
	if(Ptr)
	{
		EmuExecute((DWORD)Ptr, 3,PeLdrGetModuleBase(Pe),Reason,(DWORD)Param);
	}
}

extern "C" NTSTATUS
NTAPI 
NtSetInformationProcess(
  IN HANDLE               ProcessHandle,
  IN PROCESSINFOCLASS ProcessInformationClass,
  IN PVOID                ProcessInformation,
  IN ULONG                ProcessInformationLength );

PE_EXPORT int PeLdrStartProgram(LPWSTR ExePath)
{
	BOOLEAN EnFault=1;
	SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);	// this is really unnecessary - but just to be sure
	NtSetInformationProcess(GetCurrentProcess(),(PROCESSINFOCLASS)0x11 /*ProcessEnableAlignmentFaultFixup*/,&EnFault,sizeof(EnFault));

#ifdef _DEBUG
//	SetCurrentDirectoryA("d:\\Games\\Heroes3");	////////////////////////
#endif

	int ret=-1;
	EmuInitialize();
	PeLdrSetExecutionCallback(MyTlsCB,0);
	
	char InstDir[MAX_PATH]="c:\\x86\\";

	GetCurrentDirectoryA(MAX_PATH,InstDir);
	LogInfo("Current directory: %s",InstDir);
	LogInfo("Command line is: %s",GetCommandLineA());

// TODO: get these settings from registry
	strcpy(InstDir,PeLdrGetSystemDirectoryA());
	char *Tmp=strrchr(InstDir,'\\');
	if(Tmp)
		*Tmp=0;
	SetEnvironmentVariableA("windir",InstDir);

	wchar_t MyPath[1024];
	GetEnvironmentVariableW(L"windir",MyPath,1024);
	wcscat_s(MyPath,L";");
	wcscat_s<1024>(MyPath,PeLdrGetSystemDirectoryW());
	wcscat_s(MyPath,L";c:\\windows;c:\\windows\\system32");
	SetEnvironmentVariableW(L"PATH",MyPath);

	SetEnvironmentVariableA("ProgramFiles","C:\\x86node\\Program Files");
	SetEnvironmentVariableA("CommonProgramFiles","C:\\x86node\\Program Files\\Common Files");

	DWORD Ret=GetModuleFileNameA(0,InstDir,MAX_PATH);
	char *P=strrchr(InstDir,'\\');
	if(P)
		*P=0;
	CfgSetString("InstallDir",InstDir);
	CfgSetString("Version",EmGetVersionString());

	HMODULE HM=LoadLibraryA("cpihook.dll");
	if(HM==0)
		LogErr("Can't load the CreateProcessInternal hook");
	FARPROC hook=GetProcAddress(HM,"CpiMakeHook");
	if(hook==0)
		LogErr("Can't find the CreateProcessInternal hook");
	if(!hook())
		LogErr("Can't install the CreateProcessInternal hook");
	else
		LogInfo("CreateProcessInternal hook installed");

	HM=LoadLibraryA("imphook.dll");
	if(HM==0)
		LogErr("Can't load the import hook");
	hook=GetProcAddress(HM,"ImpMakeHook");
	if(hook==0)
		LogErr("Can't find the import hook");
	if(!hook())
		LogErr("Can't install the import hook");
	else
		LogInfo("Import hook installed");

	
	PE_HANDLE PE;
	__try {
		PE=PeLdrLoadModule(ExePath);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		LogErr("Exception in PeLdrLoadModule");
		PE=0;
	}
	if(PE==0)
	{
		LogErr("Error loading program: %d",GetLastError());
		return 0xBAD00004;
	} else
	{
		typedef int t_WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
		t_WinMain *WM=(t_WinMain*)PeLdrGetEntryPoint(PE);

		int Affinity=CompatGetInt("SetProcessAffinityMask");
		if(Affinity)
			SetThreadAffinityMask(GetCurrentThread(),Affinity);	// we set the affinity of all emulator's threads - so that our own or OS threads may still run on diffirent cores
	
		__try {
			ret=EmuExecute((DWORD)WM,4,PeLdrGetModuleBase(PE),0,(DWORD)GetCommandLineA(),SW_SHOWNORMAL);
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			LogErr("Unhandled exception running program");
			return 0xBAD00004;
		}
		LogInfo("Program exited with code: %08x",ret);
#ifndef _DEBUG
		__try {
			PeLdrCloseAllModules();
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			LogErr("Exception in PeLdrCloseAllModules");
		}
#endif
	}	
	return ret;
}
