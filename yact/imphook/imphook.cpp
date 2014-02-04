// imphook.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <peldr.h>
#include <util.h>
#include <callbacks.h>

PE_HANDLE Shell32Hook=0;

void* ImpCallback(PE_HANDLE Pe, LPCSTR DllName, LPCSTR FuncName, LPVOID Param)
{
	if(FuncName==0 || Pe==0 || DllName==0)
		return 0;
/*
	if(_strcmpi(DllName,"shell32.dll")==0)
	{
		if(Shell32Hook==0)
			Shell32Hook=PeLdrLoadModuleA("shell32hook.86.dll");
		if(Shell32Hook==0)
			return 0;

		char *Funcs[]={"SHBrowseForFolder","SHBrowseForFolderA","SHBrowseForFolderW",0};
		char *FuncsStub[]={"_stub_SHBrowseForFolder@4","_stub_SHBrowseForFolderA@4","_stub_SHBrowseForFolderW@4",0};
		for(int i=0; Funcs[i]; i++)
			if(strcmp(FuncName,Funcs[i])==0)
				return PeLdrGetProcAddressA(Shell32Hook,FuncsStub[i]);
	}
*/
	return 0;
}

extern "C" __declspec(dllexport) bool ImpMakeHook()
{
	return NULL==PeLdrSetImportCallback(ImpCallback,0);
}