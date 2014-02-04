#define CINTERFACE	// I need C interface for DDraw for lpVtbl member
#include "dllyact_def.h"
#include "comobject_def.h"
#include <windows.h>
#include <dsound.h>
#include "callbacks.h"

DEFINE_COMFUNC3(DS,IDirectSoundVtbl,SetCooperativeLevel)
DEFINE_COMFUNC2(DS,IDirectSoundVtbl,GetCaps)
DEFINE_COMFUNC1(DS,IDirectSoundVtbl,Compact)
DEFINE_COMFUNC2(DS,IDirectSoundVtbl,GetSpeakerConfig)
DEFINE_COMFUNC2(DS,IDirectSoundVtbl,SetSpeakerConfig)
DEFINE_COMFUNC2(DS,IDirectSoundVtbl,Initialize)

DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,GetCaps)
DEFINE_COMFUNC3(DSB,IDirectSoundBufferVtbl,GetCurrentPosition)
DEFINE_COMFUNC4(DSB,IDirectSoundBufferVtbl,GetFormat)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,GetVolume)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,GetPan)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,GetFrequency)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,GetStatus)
//DEFINE_COMFUNC3(DSB,IDirectSoundBufferVtbl,Initialize)
EXTERN_C DW COMSTUB_EXPORT DSB_Initialize(DW *R)
{
	ICallbackUnknown<IDirectSoundBufferVtbl> *Tmp=(ICallbackUnknown<IDirectSoundBufferVtbl>*)p1;
	ICallbackUnknown<IDirectSoundVtbl> *Tmp1=(ICallbackUnknown<IDirectSoundVtbl>*)p2;
	return Tmp->IOriginal->Vtbl->Initialize((LPDIRECTSOUNDBUFFER)Tmp->IOriginal,(LPDIRECTSOUND)Tmp1->IOriginal,(LPCDSBUFFERDESC)p3);
}

DEFINE_COMFUNC8(DSB,IDirectSoundBufferVtbl,Lock)
DEFINE_COMFUNC4(DSB,IDirectSoundBufferVtbl,Play)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,SetCurrentPosition)
//DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,SetFormat)

EXTERN_C DW COMSTUB_EXPORT DSB_SetFormat(DW *R)
{
	LPCWAVEFORMATEX lplpDSB=(LPCWAVEFORMATEX)p2;
	ICallbackUnknown<IDirectSoundBufferVtbl> *Tmp=(ICallbackUnknown<IDirectSoundBufferVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->SetFormat((LPDIRECTSOUNDBUFFER)Tmp->IOriginal,lplpDSB);
	return Ret;
}

DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,SetVolume)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,SetPan)
DEFINE_COMFUNC2(DSB,IDirectSoundBufferVtbl,SetFrequency)
DEFINE_COMFUNC1(DSB,IDirectSoundBufferVtbl,Stop)
DEFINE_COMFUNC5(DSB,IDirectSoundBufferVtbl,Unlock)
DEFINE_COMFUNC1(DSB,IDirectSoundBufferVtbl,Restore)

EXTERN_C DW COMSTUB_EXPORT DS_CreateSoundBuffer(DW *R)
{
	LPDIRECTSOUNDBUFFER* lplpDSB=(LPDIRECTSOUNDBUFFER*)p3;
	ICallbackUnknown<IDirectSoundVtbl> *Tmp=(ICallbackUnknown<IDirectSoundVtbl>*)p1;
	DWORD Ret=Tmp->IOriginal->Vtbl->CreateSoundBuffer((LPDIRECTSOUND)Tmp->IOriginal,(LPCDSBUFFERDESC)p2,lplpDSB,(LPUNKNOWN)p4);
	if(Ret!=DS_OK)
		return Ret;

	ICallbackUnknown<IDirectSoundBufferVtbl> *DSB=new ICallbackUnknown<IDirectSoundBufferVtbl>(*lplpDSB);
	*lplpDSB=(LPDIRECTSOUNDBUFFER)DSB;

	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetCaps)
	DEFINE_CALLBACK3(DSB,IDirectSoundBufferVtbl,GetCurrentPosition)
	DEFINE_CALLBACK4(DSB,IDirectSoundBufferVtbl,GetFormat)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetVolume)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetPan)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetFrequency)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetStatus)
	DEFINE_CALLBACK3(DSB,IDirectSoundBufferVtbl,Initialize)
	DEFINE_CALLBACK8(DSB,IDirectSoundBufferVtbl,Lock)
	DEFINE_CALLBACK4(DSB,IDirectSoundBufferVtbl,Play)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetCurrentPosition)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetFormat)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetVolume)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetPan)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetFrequency)
	DEFINE_CALLBACK1(DSB,IDirectSoundBufferVtbl,Stop)
	DEFINE_CALLBACK5(DSB,IDirectSoundBufferVtbl,Unlock)
	DEFINE_CALLBACK1(DSB,IDirectSoundBufferVtbl,Restore)

	return Ret;
}

EXTERN_C DW COMSTUB_EXPORT DS_DuplicateSoundBuffer(DW *R)
{
	LPDIRECTSOUNDBUFFER* lplpDSB=(LPDIRECTSOUNDBUFFER*)p3;
	ICallbackUnknown<IDirectSoundVtbl> *Tmp=(ICallbackUnknown<IDirectSoundVtbl>*)p1;
	ICallbackUnknown<IDirectSoundBufferVtbl> *Tmp1=(ICallbackUnknown<IDirectSoundBufferVtbl>*)p2;
	DWORD Ret=Tmp->IOriginal->Vtbl->DuplicateSoundBuffer((LPDIRECTSOUND)Tmp->IOriginal,(LPDIRECTSOUNDBUFFER)Tmp->IOriginal,lplpDSB);
	if(Ret!=DS_OK)
		return Ret;

	ICallbackUnknown<IDirectSoundBufferVtbl> *DSB=new ICallbackUnknown<IDirectSoundBufferVtbl>(*lplpDSB);
	*lplpDSB=(LPDIRECTSOUNDBUFFER)DSB;

	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetCaps)
	DEFINE_CALLBACK3(DSB,IDirectSoundBufferVtbl,GetCurrentPosition)
	DEFINE_CALLBACK4(DSB,IDirectSoundBufferVtbl,GetFormat)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetVolume)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetPan)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetFrequency)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,GetStatus)
	DEFINE_CALLBACK3(DSB,IDirectSoundBufferVtbl,Initialize)
	DEFINE_CALLBACK8(DSB,IDirectSoundBufferVtbl,Lock)
	DEFINE_CALLBACK4(DSB,IDirectSoundBufferVtbl,Play)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetCurrentPosition)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetFormat)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetVolume)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetPan)
	DEFINE_CALLBACK2(DSB,IDirectSoundBufferVtbl,SetFrequency)
	DEFINE_CALLBACK1(DSB,IDirectSoundBufferVtbl,Stop)
	DEFINE_CALLBACK5(DSB,IDirectSoundBufferVtbl,Unlock)
	DEFINE_CALLBACK1(DSB,IDirectSoundBufferVtbl,Restore)

	return Ret;
}

//DEFINE_FUNC3(DirectSoundCreate)
EXTERN_C DW STUB_EXPORT yact_DirectSoundCreate(DW *R)
{
	GUID *lpGUID=(GUID*)p1;
	LPDIRECTSOUND *lplpDS=(LPDIRECTSOUND*)p2;
	IUnknown *pUnkOuter=(IUnknown*)p3;
	DWORD Ret=DirectSoundCreate(lpGUID,lplpDS,pUnkOuter);
	if(Ret!=DS_OK)
		return Ret;

	ICallbackUnknown<IDirectSoundVtbl> *DS=new ICallbackUnknown<IDirectSoundVtbl>(*lplpDS);
	*lplpDS=(LPDIRECTSOUND)DS;

	DEFINE_CALLBACK4(DS,IDirectSoundVtbl,CreateSoundBuffer);
	DEFINE_CALLBACK2(DS,IDirectSoundVtbl,GetCaps);
	DEFINE_CALLBACK3(DS,IDirectSoundVtbl,DuplicateSoundBuffer);
	DEFINE_CALLBACK3(DS,IDirectSoundVtbl,SetCooperativeLevel);
	DEFINE_CALLBACK1(DS,IDirectSoundVtbl,Compact);
	DEFINE_CALLBACK2(DS,IDirectSoundVtbl,GetSpeakerConfig);
	DEFINE_CALLBACK2(DS,IDirectSoundVtbl,SetSpeakerConfig);
	DEFINE_CALLBACK2(DS,IDirectSoundVtbl,Initialize);
	return Ret;
}