// cbacks.cpp : Defines the exported functions for the DLL application.
//

#define CB_EXPORT __declspec(dllexport)
#include "dllyact_def.h"
#include <windows.h>
#include <callbacks.h>
#include <x86emul.h>
#include <classes.h>
#include <util.h>
#include <map>
using namespace std;

static LPCRITICAL_SECTION CSCb=0;
struct CbDescr
{
	DWORD NativeFunc;
	DWORD FuncParam;
	int NParams;
	bool operator==(const CbDescr&c) const
	{
		return c.NativeFunc==NativeFunc && c.FuncParam==FuncParam && c.NParams==NParams;
	}
	bool operator<(const CbDescr&c) const
	{
		if(c.NativeFunc<NativeFunc)
			return true;
		if(c.NativeFunc==NativeFunc)
		{
			if(c.FuncParam<FuncParam)
				return true;
			if(c.FuncParam==FuncParam)
				return c.NParams<NParams;
		}
		return false;
	}
};
static map<CbDescr,DWORD> NativeCallbacks;	
static map<CbDescr,DWORD> X86Callbacks;	

// Return address of callback "stop emulation and return to host"
CB_EXPORT DW CbReturnToHost(void)
{
	static char Return_to_host[]="\xF4\xEB\xFD";	// HLT; JMPS $-1
	return (DWORD)Return_to_host;
}

// Check for address inside "return to host" callback
CB_EXPORT bool CbIsReturnToHost(DW Addr)
{
	return Addr >= CbReturnToHost() && Addr < CbReturnToHost()+3;
}

typedef void CbNotifyFunc(void *);
struct CbNotif
{
	CbNotifyFunc *Func;
	void *Data;
};

CSet Notifications;

CB_EXPORT void CbNotifyThreadExit(DW ThrId)
{
	CList *Nl=(CList*)Notifications.Get(GetCurrentThreadId());
	if(Nl)
	{
		Notifications.Set(GetCurrentThreadId(),0);
		CbNotif *Nf=(CbNotif*)Nl->GetHead();
		while(Nf)
		{
			if(Nf->Func)
				Nf->Func(Nf->Data);
			free(Nf);
			Nf=(CbNotif*)Nl->GetHead();
		}
		delete Nl;
	}
}

void CbNotifyThreadExit()
{
	DWORD ThrId=GetCurrentThreadId();
	CbNotifyThreadExit(ThrId);
}

// Call the given func when current thread exits (by ExitThread for example)
CB_EXPORT void* CbCallAtThreadExit(void* Func, void*Param)
{
	CList *Nl=(CList*)Notifications.Get(GetCurrentThreadId());
	if(Nl==0)
	{
		Nl=new CList;
		if(Nl==0)
			return FALSE;
		Notifications.Set(GetCurrentThreadId(),Nl);
	}
	CbNotif *Nf=(CbNotif*)malloc(sizeof(CbNotif));
	if(Nf==0)
		return NULL;
	Nf->Func=(CbNotifyFunc*)Func;
	Nf->Data=Param;
	return Nl->AddHead(Nf)?Param:NULL;	// Adding at head optimises the remove process
}

static bool CbRemoveCallAtThreadExitEnum(void *Data, void*Param)
{
	CbNotif *Enum=(CbNotif*)Data;
	CbNotif *ToDel=(CbNotif*)Param;
	if(Data==0 || Param==0)
		return true;
	if(Enum->Data==ToDel->Data && Enum->Func==ToDel->Func)
	{
		Enum->Func=0;
		Enum->Data=0;
		return false;
	}
	return true;
}

CB_EXPORT bool CbRemoveCallAtThreadExit(void* Func, void*Param)
{
	CList *Nl=(CList*)Notifications.Get(GetCurrentThreadId());
	if(Nl==0)
		return FALSE;

	CbNotif Nf;

	Nf.Func=(CbNotifyFunc*)Func;
	Nf.Data=Param;
	Nl->ForEach(CbRemoveCallAtThreadExitEnum,&Nf);
	return TRUE;
}

// Allocate memory and initialize callback to native stdcall func taking NumParams
CB_EXPORT DW CbCreateX86StdcallCallback(void *NativeFunc, int NumParams)
{
/* 
55                             push        ebp
8BEC                           mov         ebp,esp
8D4508                         lea         eax,[ebp][8]
EB05                           jmps       .01001483E --1
B8CCAE0310                     mov         eax,01003AECC --2
E5E5                           in          eax,0E5 ;'õ'
8BE5                           mov         esp,ebp
5D                             pop         ebp
C22000                         retn        00020 
*/
	static BYTE CBCode[]={0x55,0x8B,0xEC,0x8D,0x45,0x08,0xEB,0x05,0xB8,0xDE,0xC0,0xAD,0xDE,0xE5,0xE5,0x8B,0xE5,0x5D,0xC2,0x20,0};
	if(NativeFunc==0)
		return 0;

	CbDescr Cbd;
	Cbd.NativeFunc=(DWORD)NativeFunc;
	Cbd.FuncParam=0;
	Cbd.NParams=NumParams;
	CLock L(&CSCb); L.Lock();
	DWORD Nca=X86Callbacks[Cbd];
	if(Nca)
		return Nca;

	BYTE *Ret=(BYTE*)malloc(sizeof(CBCode));
	if(Ret==0)
		return 0;
	memcpy(Ret,CBCode,sizeof(CBCode));
	*(DWORD*)(Ret+9)=0x80000000|(DWORD)NativeFunc;
	*(WORD*)(Ret+sizeof(CBCode)-2)=(WORD)NumParams*4;
	X86Callbacks[Cbd]=(DWORD)Ret;
	FlushProcessWriteBuffers();
#ifdef _ARM_
	_DataSynchronizationBarrier();
#endif
	return (DWORD)Ret;
}

// Allocate memory and initialize callback to native cdecl func taking NumParams
CB_EXPORT DW CbCreateX86CdeclCallback(void *NativeFunc)
{
	return CbCreateX86StdcallCallback(NativeFunc, 0);
}

CB_EXPORT DW CbCreateNativeCdeclCallbackEx(void *NativeFunc, DW Param)
{
	return CbCreateNativeStdcallCallbackEx(NativeFunc, Param, 0);
}

CB_EXPORT DW CbCreateNativeCdeclCallback(DW Param)
{
	return CbCreateNativeStdcallCallback(Param, 0);
}

// Allocate memory and initialize stdcall callback taking NumParams
CB_EXPORT DW CbCreateNativeStdcallCallbackEx(void *NativeFunc, DW FuncParam, int NumParams)
{
	if(NativeFunc==0 || FuncParam==0)
		return 0;
	CbDescr Cbd;
	Cbd.NativeFunc=(DWORD)NativeFunc;
	Cbd.FuncParam=FuncParam;
	Cbd.NParams=NumParams;
	CLock L(&CSCb); L.Lock();
	DWORD Nca=NativeCallbacks[Cbd];
	if(Nca)
		return Nca;

#ifdef _ARM_
/*
0F B4                                   PUSH            {R0-R3}
2D E9 00 48                             PUSH.W          {R11,LR}
EB 46                                   MOV             R11, SP
04 4B                                   LDR             R3, =0x88776655
03 48                                   LDR             R0, =0x44332211
02 A9                                   ADD             R1, SP, #8
98 47                                   BLX             R3
5D F8 04 BB                             LDR.W           R11, [SP],#4
5D F8 14 FB                             LDR.W           PC, [SP],#0x14
11 22 33 44             dword_100017E0  DCD 0x44332211        
55 66 77 88             dword_100017E4  DCD 0x88776655          
*/
	static BYTE CBCode[]={/*0xfe,0xde,0xfe,0xde,*/  0x0F,0xB4,0x2D,0xE9,0x00,0x48,0xEB,0x46,0x04,0x4B,0x03,0x48,0x02,0xA9,0x98,0x47,0x5D,0xF8,0x04,0xBB,0x5D,0xF8,0x14,0xFB,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
	BYTE *Ret=(BYTE*)malloc(sizeof(CBCode));
	if(Ret==0)
		return 0;

	memcpy(Ret,CBCode,sizeof(CBCode));
	*(DWORD*)(Ret+sizeof(CBCode)-8)=(DWORD)FuncParam;
	*(DWORD*)(Ret+sizeof(CBCode)-4)=1|(DWORD)NativeFunc;
	Ret=(BYTE*)(1|(DWORD)Ret);	// THUMB
#else
/*
55                             push        ebp
8BEC                           mov         ebp,esp
8D4508                         lea         eax,[ebp][8]
50                             push        eax
6844332211                     push        011223344  
B844332211                     mov         eax,011223344  
FFD0                           call        eax
89EC                           mov         esp,ebp
5D                             pop         ebp
C22000                         retn        00020 
*/
	// added NOP at the beginning as int3 placeholder
	static BYTE CBCode[]={0x90,0x55,0x8B,0xEC,0x8D,0x45,0x08,0x50,0x68,0xCC,0xCC,0xCC,0xCC,0xb8,0xcc,0xcc,0xcc,0xcc,0xff,0xd0,0x89,0xec,0x5d,0xc2,0x00,0x00};
	BYTE *Ret=(BYTE*)malloc(sizeof(CBCode));
	if(Ret==0)
		return 0;
	memcpy(Ret,CBCode,sizeof(CBCode));
	*(DWORD*)(Ret+8+1)=(DWORD)FuncParam;
	*(DWORD*)(Ret+13+1)=(DWORD)NativeFunc;
	*(WORD*)(Ret+sizeof(CBCode)-2)=(WORD)NumParams*4;
#endif
	NativeCallbacks[Cbd]=(DWORD)Ret;
	DWORD Tmp;
	VirtualProtect(Ret,sizeof(CBCode),PAGE_EXECUTE_READWRITE,&Tmp);
#ifdef _ARM_
	_DataSynchronizationBarrier();
#endif
	FlushProcessWriteBuffers();
	FlushInstructionCache(GetCurrentProcess(),Ret,sizeof(CBCode));
	return (DWORD)Ret;
}

static DWORD Cb0P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,0);
}
static DWORD Cb1P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,1,p1);
}
static DWORD Cb2P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,2,p1,p2);
}
static DWORD Cb3P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,3,p1,p2,p3);
}
static DWORD Cb4P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,4,p1,p2,p3,p4);
}
static DWORD Cb5P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,5,p1,p2,p3,p4,p5);
}
static DWORD Cb6P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,5,p1,p2,p3,p4,p5,p6);
}
static DWORD Cb7P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,5,p1,p2,p3,p4,p5,p6,p7);
}
static DWORD Cb8P(void* Param, DW *R)
{
	return EmuExecute((DWORD)Param,5,p1,p2,p3,p4,p5,p6,p7,p8);
}
static void *CbFuncs[]={Cb0P,Cb1P,Cb2P,Cb3P,Cb4P,Cb5P,Cb6P,Cb7P,Cb8P};

CB_EXPORT DW CbCreateNativeStdcallCallback(DW Param, int NumParams)
{
	if(Param==0)
		return 0;
	if(NumParams>sizeof(CbFuncs)/sizeof(CbFuncs[0]))
		return 0;
	return CbCreateNativeStdcallCallbackEx(CbFuncs[NumParams], Param, NumParams);
}
