#include "dllyact_def.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <x86emul.h>

void CALLBACK waveOutProc(
  HWAVEOUT hwo,      
  UINT uMsg,         
  DWORD_PTR lpParameter, 
  DWORD_PTR dwParam1,    
  DWORD_PTR dwParam2)
{
	DWORD *Parm=(DWORD*)lpParameter;
	DWORD parm0=Parm[0];
	DWORD parm1=Parm[1];
//	delete Parm;
	EmuExecute(parm0,5,(DWORD)hwo,uMsg,parm1,dwParam1,dwParam2);
}

EXTERN_C DW STUB_EXPORT yact_waveOutOpen(DW *R)
{
	DWORD* Parm=new DWORD[2];
	Parm[0]=p4;
	Parm[1]=p5;
	LPHWAVEOUT phwo=(LPHWAVEOUT)p1;
	UINT did=p2;
	LPCWAVEFORMATEX wfe=(LPCWAVEFORMATEX)p3;
	DWORD opf=p6;
	
	if(p6==CALLBACK_FUNCTION)
		return waveOutOpen(phwo,did,wfe,(DWORD)waveOutProc,(DWORD)Parm,opf);
	else
	{
		delete Parm;
		return waveOutOpen(phwo,did,wfe,p4,p5,opf);
	}
}

void CALLBACK midiOutProc(
  HMIDIOUT hmo,
  UINT wMsg,
  DWORD_PTR dwInstance,
  DWORD_PTR dwParam1,
  DWORD_PTR dwParam2
)
{
	DWORD *Parm=(DWORD*)dwInstance;
	DWORD parm0=Parm[0];
	DWORD parm1=Parm[1];
//	delete Parm;
	EmuExecute(parm0,5,(DWORD)hmo,wMsg,parm1,dwParam1,dwParam2);
}

EXTERN_C DW STUB_EXPORT yact_midiOutOpen(DW *R)
{
	DWORD* Parm=new DWORD[2];
	Parm[0]=p3;
	Parm[1]=p4;
	if(p5==CALLBACK_FUNCTION)
		return midiOutOpen((LPHMIDIOUT)p1,p2,(DWORD_PTR)midiOutProc,(DWORD)Parm,p6);
	else
	{
		delete Parm;
		return midiOutOpen((LPHMIDIOUT)p1,p2,p3,p4,p5);
	}
}

//DEFINE_FUNC5(midiOutOpen)
//DEFINE_FUNC6(waveOutOpen)
//DEFINE_FUNC5(timeSetEvent)

void CALLBACK timeCallback(
    UINT uTimerID,
    UINT uMsg,
    DWORD_PTR dwUser,
    DWORD_PTR dw1,
    DWORD_PTR dw2
)
{
	DWORD *Parm=(DWORD*)dwUser;
	DWORD parm0=Parm[0];
	DWORD parm1=Parm[1];
//	delete Parm;
	EmuExecute(parm0,5,uTimerID,uMsg,parm1,dw1,dw2);
}

EXTERN_C DW STUB_EXPORT yact_timeSetEvent(DW *R)
{
	DWORD* Parm=new DWORD[2];
	Parm[0]=p3;
	Parm[1]=p4;

	if(p5==(TIME_CALLBACK_FUNCTION|TIME_ONESHOT)||p5==(TIME_CALLBACK_FUNCTION|TIME_PERIODIC))
		return timeSetEvent(p1,p2,timeCallback,(DWORD)Parm,p5);
	else
	{
		delete Parm;
		return timeSetEvent(p1,p2,(LPTIMECALLBACK)p3,p4,p5);
	}
}

//DEFINE_FUNC1(mciExecute) - no such func in win8
EXTERN_C DW STUB_EXPORT yact_mciExecute(DW *R)
{
	return mciSendStringA((LPCSTR)p1,NULL,NULL,NULL);
}

//DEFINE_FUNC0(mmsystemGetVersion)
EXTERN_C DW STUB_EXPORT yact_mmsystemGetVersion(DW *R)
{
	return 0x030a;	// borrowed from ReactOS
}
