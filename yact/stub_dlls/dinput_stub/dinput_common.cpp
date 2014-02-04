#ifndef DIRECTINPUT_VERSION 
#error DIRECTINPUT_VERSION is not defined in project settings
#endif

#define CINTERFACE	
#include "dllstub_def.h"
#include <windows.h>
#include <stdio.h>
#include <util.h>
#include <intrin.h>
#include <dinput.h>

// TODO: Hook QueryInterface and patch the returned ptrs

#define DIAPI extern "C" HRESULT __declspec(dllexport) __stdcall

DWORD VKtoDIK(DWORD VK)
{
	static bool Initialized=false;
	static DWORD Keys[256];
	if(!Initialized)
	{
		Initialized=true;
		for(int i=1; i<128; i++)
		{
			UINT k=MapVirtualKeyA(i,MAPVK_VSC_TO_VK_EX);			
			if(k>0 && k<256)
				Keys[k]=i;
		}

		Keys[VK_NUMPAD0]=DIK_NUMPAD0;
		Keys[VK_NUMPAD1]=DIK_NUMPAD1;
		Keys[VK_NUMPAD2]=DIK_NUMPAD2;
		Keys[VK_NUMPAD3]=DIK_NUMPAD3;
		Keys[VK_NUMPAD4]=DIK_NUMPAD4;
		Keys[VK_NUMPAD5]=DIK_NUMPAD5;
		Keys[VK_NUMPAD6]=DIK_NUMPAD6;
		Keys[VK_NUMPAD7]=DIK_NUMPAD7;
		Keys[VK_NUMPAD8]=DIK_NUMPAD8;
		Keys[VK_NUMPAD9]=DIK_NUMPAD9;
		Keys[VK_MULTIPLY]=DIK_MULTIPLY;
		Keys[VK_ADD]=DIK_ADD;
		Keys[VK_SUBTRACT]=DIK_SUBTRACT;
		Keys[VK_DECIMAL]=DIK_DECIMAL;
		Keys[VK_DIVIDE]=DIK_DIVIDE;
		Keys[VK_RCONTROL]=DIK_RCONTROL;
		Keys[VK_RMENU]=DIK_RMENU;
		Keys[VK_PAUSE]=DIK_PAUSE;
		Keys[VK_HOME]=DIK_HOME;
		Keys[VK_UP]=DIK_UP;
		Keys[VK_PRIOR]=DIK_PRIOR;
		Keys[VK_LEFT]=DIK_LEFT;
		Keys[VK_RIGHT]=DIK_RIGHT;
		Keys[VK_END]=DIK_END;
		Keys[VK_DOWN]=DIK_DOWN;
		Keys[VK_NEXT]=DIK_NEXT;
		Keys[VK_INSERT]=DIK_INSERT;
		Keys[VK_DELETE]=DIK_DELETE;
		Keys[VK_LWIN]=DIK_LWIN;
		Keys[VK_RWIN]=DIK_RWIN;
	}
	if(VK<256)
		return Keys[VK];
	return 0;
}

DWORD WINAPI ThreadProc(
  _In_  LPVOID lpParameter
)
{
	while(1)
	{
		Sleep(100);
		SetEvent((HANDLE)lpParameter);
	}
}

typedef HRESULT __stdcall t_SetEventNotification(IDirectInputDeviceW* _this, HANDLE);
t_SetEventNotification* org_DIK_SetEventNotification=0;
HRESULT __stdcall DIK_SetEventNotification(IDirectInputDeviceW* _this, HANDLE evt)
{
//	LogInfo("Dinput::DIK_SetEventNotification");
//	HRESULT hr=org_DIK_SetEventNotification(_this,evt); 
	CloseHandle(CreateThread(0,0,ThreadProc,evt,0,0));	// instead of notification - make the poling 10 times per second
	return S_OK;
}

void MyGetKeyboardState(BYTE Keys[256],int type)
{
	static DWORD PrevTick=0;
	static int prevtype=-1;
	static BYTE PrevKeys[256];
	DWORD CurTick=GetTickCount();
	if(type==1)				// so that GetDeviceState(2) imediately following GetDeviceData(1) obtains the same data
	{
		GetKeyboardState(PrevKeys);
		PrevTick=CurTick;
	} else
	{
		if(CurTick-PrevTick>40)
		{
			GetKeyboardState(PrevKeys);
			PrevTick=CurTick;
		}
	}
	memcpy(Keys,PrevKeys,256);
	prevtype=type;
}


typedef HRESULT __stdcall t_GetDeviceData(IDirectInputDeviceW* _this,DWORD,LPDIDEVICEOBJECTDATA,LPDWORD,DWORD);
t_GetDeviceData *org_DIK_GetDeviceData=0;
HRESULT __stdcall DIK_GetDeviceData(IDirectInputDeviceW* _this,DWORD cbObjectData,LPDIDEVICEOBJECTDATA rgdod,LPDWORD pdwInOut,DWORD dwFlags)
{
//	LogInfo("Dinput::DIK_GetDeviceData");
//	HRESULT hr=org_DIK_GetDeviceData(_this,cbObjectData,rgdod,pdwInOut,dwFlags);
	DWORD Pos=0;
	static DWORD Seq=1;
	static BYTE prevstate[256];
	BYTE keystate[256];
	static int type=1;
	MyGetKeyboardState(keystate,type);
	int i;
	for(i=0; i<256 && Pos<*pdwInOut; i++)
	{
		if(prevstate[i]==keystate[i] || VKtoDIK(i)==0)
			continue;

		DWORD Val=keystate[i];
		if((dwFlags&DIGDD_PEEK)==0)
			prevstate[i]=keystate[i];

		if(rgdod)
		{
			LPDIDEVICEOBJECTDATA dod=(LPDIDEVICEOBJECTDATA)((Pos*cbObjectData)+(char*)rgdod);

			dod->dwOfs=VKtoDIK(i);
			dod->dwData=Val&0x80;
			dod->dwTimeStamp=GetTickCount();
			dod->dwSequence=Seq++;	
		}
		Pos++;
	}
	*pdwInOut=Pos;
	if(i==256)
	{
		if((dwFlags&DIGDD_PEEK)==0)
			type=1;
		else
			type=0;
		return DI_OK;
	} else
	{
		type=0;
		return DI_BUFFEROVERFLOW;
	}
}

typedef HRESULT __stdcall t_SetCooperativeLevel(IDirectInputDeviceW* _this, HWND,DWORD);
t_SetCooperativeLevel *org_DIK_SetCooperativeLevel=0;
HRESULT __stdcall DIK_SetCooperativeLevel(IDirectInputDeviceW* _this, HWND hwnd,DWORD Flags)
{
	//LogInfo("Dinput::DIK_SetCooperativeLevel");
	HRESULT hr=org_DIK_SetCooperativeLevel(_this,hwnd,Flags);
	return S_OK;	// always OK
}

typedef HRESULT __stdcall t_Acquire(IDirectInputDeviceW* _this);
t_Acquire *org_DIK_Acquire=0;
HRESULT __stdcall DIK_Acquire(IDirectInputDeviceW* _this)
{
	//LogInfo("Dinput::DIK_Acquire");
	HRESULT hr=org_DIK_Acquire(_this);
	return S_OK;	// always OK
}

typedef HRESULT __stdcall t_Unacquire(IDirectInputDeviceW* _this);
t_Unacquire *org_DIK_Unacquire=0;
HRESULT __stdcall DIK_Unacquire(IDirectInputDeviceW* _this)
{
	//LogInfo("Dinput::DIK_Unacquire");
	HRESULT hr=org_DIK_Unacquire(_this);
	return S_OK;	// always OK
}

typedef HRESULT __stdcall t_GetDeviceState(IDirectInputDeviceW* _this, DWORD,LPVOID);
t_GetDeviceState *org_DIK_GetDeviceState=0;
HRESULT __stdcall DIK_GetDeviceState(IDirectInputDeviceW* _this, DWORD Len,LPVOID Buff)
{
	//LogInfo("Dinput::DIK_GetDeviceState");
	HRESULT hr=org_DIK_GetDeviceState(_this,Len,Buff);
	if(FAILED(hr))
	{
		BYTE keystate[256];
		MyGetKeyboardState(keystate,2);
		memset(Buff,0,Len);
		BYTE* Keys=(BYTE*)Buff;
		for(int i=0; i<256; i++)
		{
			UINT k=VKtoDIK(i);			
			if(k>0 && k<Len)
				Keys[k]=keystate[i];
		}
	}
	return S_OK;
}

typedef HRESULT __stdcall t_DI_CreateDevice(IDirectInputW* _this, REFGUID guid,LPDIRECTINPUTDEVICE *out,LPUNKNOWN unk);
t_DI_CreateDevice *org_DI_CreateDevice=0;
HRESULT __stdcall DI_CreateDevice(IDirectInputW* _this, REFGUID guid,LPDIRECTINPUTDEVICE *out,LPUNKNOWN unk)
{
	LogInfo("DirectInput::CreateDevice: Device GUID {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	HRESULT hr=org_DI_CreateDevice(_this,guid,out,unk);
	LPDIRECTINPUTDEVICE did=(LPDIRECTINPUTDEVICE)*out;
	if(SUCCEEDED(hr) && (guid==GUID_SysKeyboardEm || guid==GUID_SysKeyboard || guid==GUID_SysKeyboardEm2))
	{
		LogWarn("DirectInput::CreateDevice: hooked keyboard input");
		org_DIK_SetCooperativeLevel=(t_SetCooperativeLevel*)did->lpVtbl->SetCooperativeLevel;
		did->lpVtbl->SetCooperativeLevel=DIK_SetCooperativeLevel;

		org_DIK_Acquire=(t_Acquire*)did->lpVtbl->Acquire;
		did->lpVtbl->Acquire=DIK_Acquire;

		org_DIK_Unacquire=(t_Unacquire*)did->lpVtbl->Unacquire;
		did->lpVtbl->Unacquire=DIK_Unacquire;

		org_DIK_GetDeviceState=(t_GetDeviceState*)did->lpVtbl->GetDeviceState;
		did->lpVtbl->GetDeviceState=DIK_GetDeviceState;

		org_DIK_GetDeviceData=(t_GetDeviceData*)did->lpVtbl->GetDeviceData;
		did->lpVtbl->GetDeviceData=DIK_GetDeviceData;

		org_DIK_SetEventNotification=(t_SetEventNotification*)did->lpVtbl->SetEventNotification;
		did->lpVtbl->SetEventNotification=DIK_SetEventNotification;
	}
	return hr;
}

