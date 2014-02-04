#include "dllyact_def.h"
#include <windows.h>
#include <regredir.h>
#include <util.h>
#include "peldr.h"
#include "callbacks.h"

// RegOverridePredefKey can't be used as it would redirect my own queries too

#define REDIR_PATH "Software\\x86node\\"

bool RedirReg=true;	// true == redirect, false - no

HKEY RedirHKCR=0;
HKEY RedirHKCU=0;
HKEY RedirHKLM=0;

char *PreCreateKeys[]={"HKCU\\Software","HKCU\\Environment","HKLM\\Software","HKCU\\System","HKLM\\System","HKCU\\System\\CurrentControlSet","HKLM\\System\\CurrentControlSet","HKCU\\Software\\Microsoft","HKLM\\Software\\Microsoft",
					   "HKCU\\System\\CurrentControlSet\\Control","HKLM\\System\\CurrentControlSet\\Control","HKLM\\System\\CurrentControlSet\\Services","Settings","Compatibility",
					   "HKCU\\Software\\Microsoft\\Windows","HKLM\\Software\\Microsoft\\Windows","HKCU\\Software\\Microsoft\\Windows NT","HKLM\\Software\\Microsoft\\Windows NT","HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
					   "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer","HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths","HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
					   "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion","HKLM\\Software\\Microsoft\\Windows\\CurrentVersion","HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion","HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion",
					   "HKCR\\CLSID",0};

char *FakeCommonSF[][2]={
	{"Common Administrative Tools","Administrative Tools"},
	{"Common AppData","AppData"},
	{"Common Desktop","Desktop"},
	{"Common Documents","Personal"},
	{"Common Programs","Programs"},
	{"Common Start Menu","Start Menu"},
	{"Common Startup","Startup"},
	{"Common Templates","Templates"},
	{"CommonMusic","My Music"},
	{"CommonPictures","My Pictures"},
	{"CommonVideo","My Video"},
	0,0
};

char *FakeCommonUSF[][2]={
	{"{3D644C9B-1FB8-4f30-9B45-F670235F79C0}","{374DE290-123F-4565-9164-39C4925E467B}"},	// downloads
	{"Common AppData","AppData"},
	{"Common Desktop","Desktop"},
	{"Common Documents","Personal"},
	{"Common Programs","Programs"},
	{"Common Start Menu","Start Menu"},
	{"Common Startup","Startup"},
	{"Common Templates","Templates"},
	{"CommonMusic","My Music"},
	{"CommonPictures","My Pictures"},
	{"CommonVideo","My Video"},
	0,0
};

void RedirectCommon(const char *SrcKey, HKEY DstKey, char *Table[][2])
{
	for(int i=0; Table[i][0]; i++)
	{
		DWORD Type=0;
		BYTE Data[65536];
		DWORD DataLen=65536;

		if(RegGetValueA(HKEY_CURRENT_USER,SrcKey,Table[i][1],RRF_RT_REG_SZ|RRF_RT_REG_MULTI_SZ|RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND,&Type,&Data,&DataLen)==ERROR_SUCCESS)
			RegSetValueExA(DstKey,Table[i][0],0,Type,Data,DataLen);
	}
}


// Copy values ignoring the security and nested keys
static void RegCopyValues(HKEY SrcKey, LPCSTR SrcSubKey, HKEY DstKey)
{
	bool NeedClose=false;
	if(SrcSubKey)
	{
		if(RegOpenKeyA(SrcKey,SrcSubKey,&SrcKey)==ERROR_SUCCESS)
			NeedClose=true;
		else
			return;
	}

	wchar_t ValueName[32767];
	BYTE ValueData[65536];
	int Index=0;
	DWORD NameLen=32767;
	DWORD DataLen=65536;
	DWORD ValueType=0;

	while(RegEnumValueW(SrcKey,Index++,ValueName,&NameLen,0,&ValueType,ValueData,&DataLen)!=ERROR_NO_MORE_ITEMS)
	{
		RegSetValueExW(DstKey,ValueName,0,ValueType,ValueData,DataLen);
		NameLen=32767;
		DataLen=65536;
	}

	if(NeedClose)
		RegCloseKey(SrcKey);
}

DWORD WINAPI ThreadProc(
  _In_  LPVOID lpParameter
)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset(&si,0,sizeof(si));
	si.cb=sizeof(si);
	if(CreateProcessA(0,"c:\\Windows\\System32\\cmd.exe /c \"c:\\Windows\\System32\\reg.exe import c:\\x86node\\Windows\\SystemNT\\x86node.reg >%temp%\\initreg.log 2>&1\"",0,0,0,0,0,0,&si,&pi))
	{
		WaitForSingleObject(pi.hProcess,INFINITE);
		LogInfo("Imported x86node.reg");
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else
		LogInfo("Failed to import x86node.reg with reg.exe");

	CfgSetInt("RegistryInitialized",1);	// in the case if REG file resets this value
	return 0;
}

BOOL InitAdvapi()
{
	bool RedirReg=true;
	RedirReg=!CompatGetInt("UseDirectRegistry");
	if(CfgGetInt("RegistryInitialized",0)==0)
	{
		CfgSetInt("RegistryInitialized",1);	// so we don't try to reinit it in the other process
		HANDLE H=CreateThread(0,0,ThreadProc,0,0,0);
	}

	if(RedirReg)
	{
		RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCR",0,0,0,KEY_ALL_ACCESS,0,&RedirHKCR,0);
		RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU",0,0,0,KEY_ALL_ACCESS,0,&RedirHKCU,0);
		RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKLM",0,0,0,KEY_ALL_ACCESS,0,&RedirHKLM,0);

		for(int i=0; PreCreateKeys[i]; i++)
		{
			HKEY Tmp;
			char Key[1024]=REDIR_PATH;
			strcat_s(Key,PreCreateKeys[i]);
			RegCreateKeyExA(HKEY_CURRENT_USER,Key,0,0,0,KEY_ALL_ACCESS,0,&Tmp,0);
			RegCloseKey(Tmp);
		} 

//		char *Tmp=getenv("TEMP");	// some old progs query environment from registry and crash when they don't see TEMP there
		if(RedirHKCR==0 || RedirHKCU==0 || RedirHKLM==0)
			return FALSE;

		HKEY ToCopy=0;
		if(RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU\\Environment",0,0,0,KEY_ALL_ACCESS,0,&ToCopy,0)==ERROR_SUCCESS)
		{
			RegCopyValues(HKEY_CURRENT_USER,"Environment",ToCopy);
			RegCloseKey(ToCopy);
		}

		if(RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,0,0,KEY_ALL_ACCESS,0,&ToCopy,0)==ERROR_SUCCESS)
		{
			RegCopyValues(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",ToCopy);
			RegCloseKey(ToCopy);
		}

		if(RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,0,0,KEY_ALL_ACCESS,0,&ToCopy,0)==ERROR_SUCCESS)
		{
			RegCopyValues(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",ToCopy);
			if(CompatGetInt("SimulateAdminRights"))
				RedirectCommon("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",ToCopy,FakeCommonSF);
			RegCloseKey(ToCopy);
		}

		if(RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",0,0,0,KEY_ALL_ACCESS,0,&ToCopy,0)==ERROR_SUCCESS)
		{
			RegCopyValues(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",ToCopy);
			RegCloseKey(ToCopy);
		}

		if(RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",0,0,0,KEY_ALL_ACCESS,0,&ToCopy,0)==ERROR_SUCCESS)
		{
			RegCopyValues(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",ToCopy);
			if(CompatGetInt("SimulateAdminRights"))
				RedirectCommon("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",ToCopy,FakeCommonUSF);
			RegCloseKey(ToCopy);
		}

		if(RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU\\Control Panel\\Desktop",0,0,0,KEY_ALL_ACCESS,0,&ToCopy,0)==ERROR_SUCCESS)
		{
			RegCopyTreeA(HKEY_CURRENT_USER,"Control Panel\\Desktop",ToCopy);
			RegCloseKey(ToCopy);
		}

		return TRUE;
	} 

	RedirHKCR=HKEY_CLASSES_ROOT;
	RedirHKCU=HKEY_CURRENT_USER;
	RedirHKLM=HKEY_LOCAL_MACHINE;

	return TRUE;
}

void CleanupAdvapi()
{
	if(RedirReg)
	{
		RegCloseKey(RedirHKCR);
		RegCloseKey(RedirHKCU);
		RegCloseKey(RedirHKLM);
	}
}

extern "C" __declspec(dllexport) DWORD Redirect(DWORD Org)
{
	switch(Org)
	{
	case (DWORD)HKEY_CURRENT_USER:
		return (DWORD)RedirHKCU;
	case (DWORD)HKEY_CLASSES_ROOT:
		return (DWORD)RedirHKCR;
	case (DWORD)HKEY_LOCAL_MACHINE:
		return (DWORD)RedirHKLM;
	}
	return (DWORD)Org;
}

//DEFINE_FUNC1(RegCloseKey)
EXTERN_C DW STUB_EXPORT yact_RegCloseKey(DW *R)
{
	HKEY HK=(HKEY)p1;
	if(HK==RedirHKCR || HK==RedirHKCU || HK==RedirHKLM)
		return ERROR_SUCCESS;
	return RegCloseKey(HK);
}

#if 0
//DEFINE_FUNC3(RegCreateKeyA)
EXTERN_C DW STUB_EXPORT yact_RegCreateKeyA(DW *R)
{
	return RegCreateKeyA(Redirect(p1),(LPCSTR)p2,(PHKEY)p3);
}

//DEFINE_FUNC3(RegCreateKeyW)
EXTERN_C DW STUB_EXPORT yact_RegCreateKeyW(DW *R)
{
	return RegCreateKeyW(Redirect(p1),(LPCWSTR)p2,(PHKEY)p3);
}
#endif
//DEFINE_FUNC9(RegCreateKeyExA)
EXTERN_C DW STUB_EXPORT yact_RegCreateKeyExA(DW *R)
{
//	LogPrint("RegCreateKeyExA(%08x,%s,%08x,%s,%08x,%08x,%08x,%08x,%08x)",p1,p2,p3,p4,p5,p6,p7,p8,p9);
	if(p7)
	{
		LPSECURITY_ATTRIBUTES sa=(LPSECURITY_ATTRIBUTES)p7;
		if(CompatGetInt("SimulateAdminRights"))
			sa->lpSecurityDescriptor=0;		// support only the default security
//		LogPrint("RegCreateKeyExA ldpa %08x,%08x,%08x",sa->nLength,sa->lpSecurityDescriptor,sa->bInheritHandle);
	}
	DW Ret=RegCreateKeyExA((HKEY)Redirect(p1),(LPCSTR)p2,p3,(LPSTR)p4,p5,(REGSAM)p6,(LPSECURITY_ATTRIBUTES)p7,(PHKEY)p8,(LPDWORD)p9);
//	LogPrint("RegCreateKeyExA ret=%08x",Ret);
	return Ret;
}
//DEFINE_FUNC9(RegCreateKeyExW)
EXTERN_C DW STUB_EXPORT yact_RegCreateKeyExW(DW *R)
{
	if(p7)
	{
		LPSECURITY_ATTRIBUTES sa=(LPSECURITY_ATTRIBUTES)p7;
		if(CompatGetInt("SimulateAdminRights"))
			sa->lpSecurityDescriptor=0;		// support only the default security
	}
	DW Ret=RegCreateKeyExW((HKEY)Redirect(p1),(LPCWSTR)p2,p3,(LPWSTR)p4,p5,(REGSAM)p6,(LPSECURITY_ATTRIBUTES)p7,(PHKEY)p8,(LPDWORD)p9);
	return Ret;
}
#if 0
//DEFINE_FUNC2(RegDeleteKeyA)
EXTERN_C DW STUB_EXPORT yact_RegDeleteKeyA(DW *R)
{
	return RegDeleteKeyA(Redirect(p1),(LPCSTR)p2);
}

//DEFINE_FUNC2(RegDeleteKeyW)
EXTERN_C DW STUB_EXPORT yact_RegDeleteKeyW(DW *R)
{
	return RegDeleteKeyW(Redirect(p1),(LPCWSTR)p2);
}

//DEFINE_FUNC4(RegDeleteKeyExA)
EXTERN_C DW STUB_EXPORT yact_RegDeleteKeyExA(DW *R)
{
	return RegDeleteKeyExA(Redirect(p1),(LPCSTR)p2,(REGSAM)p3,p4);
}

//DEFINE_FUNC4(RegDeleteKeyExW)
EXTERN_C DW STUB_EXPORT yact_RegDeleteKeyExW(DW *R)
{
	return RegDeleteKeyExW(Redirect(p1),(LPCWSTR)p2,(REGSAM)p3,p4);
}

//DEFINE_FUNC3(RegOpenKeyA)
EXTERN_C DW STUB_EXPORT yact_RegOpenKeyA(DW *R)
{
	return RegOpenKeyA(Redirect(p1),(LPCSTR)p2,(PHKEY)p3);
}

//DEFINE_FUNC3(RegOpenKeyW)
EXTERN_C DW STUB_EXPORT yact_RegOpenKeyW(DW *R)
{
	return RegOpenKeyW(Redirect(p1),(LPCWSTR)p2,(PHKEY)p3);
}

//DEFINE_FUNC5(RegOpenKeyExA)
EXTERN_C DW STUB_EXPORT yact_RegOpenKeyExA(DW *R)
{
	return RegOpenKeyExA(Redirect(p1),(LPCSTR)p2,p3,(REGSAM)p4,(PHKEY)p5);
}

//DEFINE_FUNC5(RegOpenKeyExW)
EXTERN_C DW STUB_EXPORT yact_RegOpenKeyExW(DW *R)
{
	return RegOpenKeyExW(Redirect(p1),(LPCWSTR)p2,p3,(REGSAM)p4,(PHKEY)p5);
}
#endif

//DEFINE_FUNC2(RegOverridePredefKey)
EXTERN_C DW STUB_EXPORT yact_RegOverridePredefKey(DW *R)
{
	switch(p1)
	{
	case (DWORD)HKEY_CURRENT_USER:
		if(p2)
		{
			RegCloseKey(RedirHKCU);
			RedirHKCU=(HKEY)p2;
		} else
			RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU",0,0,0,KEY_ALL_ACCESS,0,&RedirHKCU,0);
		return ERROR_SUCCESS;
	case (DWORD)HKEY_CLASSES_ROOT:
		if(p2)
		{
			RegCloseKey(RedirHKCR);
			RedirHKCR=(HKEY)p2;
		} else
			RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU",0,0,0,KEY_ALL_ACCESS,0,&RedirHKCR,0);
		return ERROR_SUCCESS;
	case (DWORD)HKEY_LOCAL_MACHINE:
		if(p2)
		{
			RegCloseKey(RedirHKLM);
			RedirHKLM=(HKEY)p2;
		} else
			RegCreateKeyExA(HKEY_CURRENT_USER,REDIR_PATH "HKCU",0,0,0,KEY_ALL_ACCESS,0,&RedirHKLM,0);
		return ERROR_SUCCESS;
	}

	return RegOverridePredefKey((HKEY)p1,(HKEY)p2);
}

//DEFINE_FUNC3(OpenSCManagerA)
EXTERN_C DW STUB_EXPORT yact_OpenSCManagerA(DW *R)
{
	DW r=(DW)OpenSCManagerA((LPCSTR)p1,(LPCSTR)p2,p3);
	if(CompatGetInt("SimulateAdminRights"))
	{
		r=-1;
		SetLastError(0);
	}
	return r;
}
//DEFINE_FUNC3(OpenSCManagerW)
EXTERN_C DW STUB_EXPORT yact_OpenSCManagerW(DW *R)
{
	DW r=(DWORD)OpenSCManagerW((LPCWSTR)p1,(LPCWSTR)p2,p3);
	if(CompatGetInt("SimulateAdminRights"))
	{
		r=-1;
		SetLastError(0);
	}
	return r;
}

//DEFINE_FUNC1(CloseServiceHandle)
EXTERN_C DW STUB_EXPORT yact_CloseServiceHandle(DW *R)
{
	DW r;
	if(CompatGetInt("SimulateAdminRights") && p1==-1)
		r=1;
	else
		r=CloseServiceHandle((SC_HANDLE)p1);
	return r;
}

EXTERN_C DW STUB_EXPORT yact_RegCopyTreeA(DW *R)
{
	return RegCopyTreeA((HKEY)Redirect(p1),(LPCSTR)p2,(HKEY)Redirect(p3));
}

EXTERN_C DW STUB_EXPORT yact_RegCopyTreeW(DW *R)
{
	return RegCopyTreeW((HKEY)Redirect(p1),(LPCWSTR)p2,(HKEY)Redirect(p3));
}

