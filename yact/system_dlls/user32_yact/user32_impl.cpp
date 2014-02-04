#include "dllyact_def.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <x86emul.h>
#define NO_COMMCTRL_DA
#include <commctrl.h>
#include <callbacks.h>
#include <map>
#include <util.h>
#include "peldr.h"
#include <classes.h>
#include <richedit.h>
#include <string>

using namespace std;
map<string,DWORD> OrigWndProcs;

static BOOL CALLBACK PreProcessMessage(	// preview message and modify its params if needed
    HWND &hwnd, UINT &uMsg, WPARAM &wParam, LPARAM &lParam, DWORD *Ret);

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
	char Buff[256];
	GetClassNameA(hwnd,Buff,256);
#ifdef _DEBUG
	if(OrigWndProcs[Buff]&0x80000000)
		__debugbreak();
#endif
	DWORD ret;
	if(PreProcessMessage(hwnd,uMsg,wParam,lParam,&ret))
		return ret;

	return EmuExecute(OrigWndProcs[Buff],4,(DWORD)hwnd,(DWORD)uMsg,(DWORD)wParam,(DWORD)lParam);
}

EXTERN_C DW STUB_EXPORT yact_RegisterClassW(DW *R)
{
	char Buff[1024];
	WNDCLASSW *w=(WNDCLASSW*)p1;
	sprintf_s(Buff,"%S",w->lpszClassName);

//	w->hInstance=GetModuleHandle(0);
	OrigWndProcs[Buff]=(DWORD)w->lpfnWndProc;
	w->lpfnWndProc=WindowProc;

	ATOM r=RegisterClassW(w);

	DWORD W=GetLastError();

	LEAVE(1);
	return (DW)r;
}

EXTERN_C DW STUB_EXPORT yact_RegisterClassA(DW *R)
{
	WNDCLASSA *w=(WNDCLASSA*)p1;

//	w->hInstance=GetModuleHandle(0);
	OrigWndProcs[w->lpszClassName]=(DWORD)w->lpfnWndProc;
	w->lpfnWndProc=WindowProc;

	ATOM r=RegisterClassA(w);

	DWORD W=GetLastError();

	LEAVE(1);
	return (DW)r;
}

static BOOL IsDialog(HWND hwnd)
{
	char Buff[9];	
	if(RealGetWindowClassA(hwnd,Buff,8)<8)
	{
		Buff[8]=0;
		return strcmp("#32770",Buff)==0;
	}
	return false;
}

//DEFINE_FUNC2(GetWindowLongA)
EXTERN_C DW STUB_EXPORT yact_GetWindowLongA(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	DWORD r=0;

	if(index==GWL_WNDPROC || (IsDialog(hwnd) && index==DWL_DLGPROC))
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)
		{
			r=0x80000000|GetWindowLongA(hwnd,index);	// I set high bit to indicate native wndproc
		}
	} else
	{
		r=GetWindowLongA(hwnd,index);
	}

	LEAVE(2);
	return (DW)r;
}

//DEFINE_FUNC2(GetWindowLongW)
EXTERN_C DW STUB_EXPORT yact_GetWindowLongW(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	DWORD r=0;

	if(index==GWL_WNDPROC || (IsDialog(hwnd) && index==DWL_DLGPROC))
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)
		{
			r=0x80000000|GetWindowLongW(hwnd,index);
		}
	} else
	{
		r=GetWindowLongW(hwnd,index);
	}

	LEAVE(2);
	return (DW)r;
}

//DEFINE_FUNC3(SetWindowLongA)
EXTERN_C DW STUB_EXPORT yact_SetWindowLongA(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	LONG newlong=p3;
	DWORD r=0;

	if(index==GWL_WNDPROC || (IsDialog(hwnd) && index==DWL_DLGPROC))
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)	// subclassing a "native" control
		{
			r=0x80000000|SetWindowLongA(hwnd,index,(DWORD)WindowProc);
		}
		if(newlong&0x80000000)	// restoring a "native" control wndproc
		{
			SetWindowLongA(hwnd,index,0x7fffffff&(DWORD)newlong);
			newlong=0;
		}
		OrigWndProcs[Buff]=newlong;
	} else
	{
		r=SetWindowLongA(hwnd,index,newlong);
	}

	LEAVE(3);
	return (DW)r;
}

//DEFINE_FUNC3(SetWindowLongW)
EXTERN_C DW STUB_EXPORT yact_SetWindowLongW(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	LONG newlong=p3;
	DWORD r=0;

	if(index==GWL_WNDPROC || (IsDialog(hwnd) && index==DWL_DLGPROC))
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)	// subclassing a "native" control
		{
			r=0x80000000|SetWindowLongW(hwnd,index,(DWORD)WindowProc);
		}
		if(newlong&0x80000000)	// restoring a "native" control wndproc
		{
			SetWindowLongW(hwnd,index,0x7fffffff&(DWORD)newlong);
			newlong=0;
		}
		OrigWndProcs[Buff]=newlong;
	} else
	{
		r=SetWindowLongW(hwnd,index,newlong);
	}

	LEAVE(3);
	return (DW)r;
}

//DEFINE_FUNC2(GetClassLongA)
EXTERN_C DW STUB_EXPORT yact_GetClassLongA(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	DWORD r=0;

	if(index==GCL_WNDPROC)
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)
		{
			r=0x80000000|GetClassLongA(hwnd,index);	// I set high bit to indicate native wndproc
		}
	} else
	{
		r=GetClassLongA(hwnd,index);
	}

	LEAVE(2);
	return (DW)r;
}
//DEFINE_FUNC2(GetClassLongW)
EXTERN_C DW STUB_EXPORT yact_GetClassLongW(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	DWORD r=0;

	if(index==GCL_WNDPROC)
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)
		{
			r=0x80000000|GetClassLongW(hwnd,index);	// I set high bit to indicate native wndproc
		}
	} else
	{
		r=GetClassLongW(hwnd,index);
	}

	LEAVE(2);
	return (DW)r;
}
//DEFINE_FUNC3(SetClassLongA)
EXTERN_C DW STUB_EXPORT yact_SetClassLongA(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	LONG newlong=p3;
	DWORD r=0;

	if(index==GCL_WNDPROC)
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)	// subclassing a "native" control
		{
			r=0x80000000|SetClassLongA(hwnd,index,(DWORD)WindowProc);
		}
		if(newlong&0x80000000)	// restoring a "native" control wndproc
		{
			SetClassLongA(hwnd,index,0x7fffffff&(DWORD)newlong);
			newlong=0;
		}
		OrigWndProcs[Buff]=newlong;
	} else
	{
		r=SetClassLongA(hwnd,index,newlong);
	}

	LEAVE(3);
	return (DW)r;
}
//DEFINE_FUNC3(SetClassLongW)
EXTERN_C DW STUB_EXPORT yact_SetClassLongW(DW *R)
{
	HWND hwnd=(HWND)p1;
	int index=p2;
	LONG newlong=p3;
	DWORD r=0;

	if(index==GCL_WNDPROC)
	{
		char Buff[1024];
		GetClassNameA(hwnd,Buff,1024);
		r=OrigWndProcs[Buff];
		if(r==0)	// subclassing a "native" control
		{
			r=0x80000000|SetClassLongW(hwnd,index,(DWORD)WindowProc);
		}
		if(newlong&0x80000000)	// restoring a "native" control wndproc
		{
			SetClassLongW(hwnd,index,0x7fffffff&(DWORD)newlong);
			newlong=0;
		}
		OrigWndProcs[Buff]=newlong;
	} else
	{
		r=SetClassLongW(hwnd,index,newlong);
	}

	LEAVE(3);
	return (DW)r;
}
//DEFINE_FUNC3(GetClassInfoA)
EXTERN_C DW STUB_EXPORT yact_GetClassInfoA(DW *R)
{
	HINSTANCE hInst=(HINSTANCE)p1;
	LPCSTR Class=(LPCSTR)p2;
	LPWNDCLASSA wca=(LPWNDCLASSA)p3;
	BOOL Ret=GetClassInfoA(hInst,Class,wca);
	if(Ret)
	{
		DWORD wndp=OrigWndProcs[Class];
		if(wndp==0)
		{
			wndp=0x80000000|(DWORD)wca->lpfnWndProc;	// I set high bit to indicate native wndproc
		}
		wca->lpfnWndProc=(WNDPROC)wndp;
	}
	return Ret;
}
//DEFINE_FUNC3(GetClassInfoExA)
EXTERN_C DW STUB_EXPORT yact_GetClassInfoExA(DW *R)
{
	HINSTANCE hInst=(HINSTANCE)p1;
	LPCSTR Class=(LPCSTR)p2;
	LPWNDCLASSEXA wca=(LPWNDCLASSEXA)p3;
	BOOL Ret=GetClassInfoExA(hInst,Class,wca);
	if(Ret)
	{
		DWORD wndp=OrigWndProcs[Class];
		if(wndp==0)
		{
			wndp=0x80000000|(DWORD)wca->lpfnWndProc;	// I set high bit to indicate native wndproc
		}
		wca->lpfnWndProc=(WNDPROC)wndp;
	}
	return Ret;
}
//DEFINE_FUNC3(GetClassInfoW)
EXTERN_C DW STUB_EXPORT yact_GetClassInfoW(DW *R)
{
	HINSTANCE hInst=(HINSTANCE)p1;
	LPCWSTR Class=(LPCWSTR)p2;
	LPWNDCLASSW wca=(LPWNDCLASSW)p3;
	BOOL Ret=GetClassInfoW(hInst,Class,wca);
	if(Ret)
	{
		char Tmp[128];
		sprintf_s(Tmp,"%S",Class);
		DWORD wndp=OrigWndProcs[Tmp];
		if(wndp==0)
		{
			wndp=0x80000000|(DWORD)wca->lpfnWndProc;	// I set high bit to indicate native wndproc
		}
		wca->lpfnWndProc=(WNDPROC)wndp;
	}
	return Ret;
}
//DEFINE_FUNC3(GetClassInfoExW)
EXTERN_C DW STUB_EXPORT yact_GetClassInfoExW(DW *R)
{
	HINSTANCE hInst=(HINSTANCE)p1;
	LPCWSTR Class=(LPCWSTR)p2;
	LPWNDCLASSEXW wca=(LPWNDCLASSEXW)p3;
	BOOL Ret=GetClassInfoExW(hInst,Class,wca);
	if(Ret)
	{
		char Tmp[128];
		sprintf_s(Tmp,"%S",Class);
		DWORD wndp=OrigWndProcs[Tmp];
		if(wndp==0)
		{
			wndp=0x80000000|(DWORD)wca->lpfnWndProc;	// I set high bit to indicate native wndproc
		}
		wca->lpfnWndProc=(WNDPROC)wndp;
	}
	return Ret;
}

//DEFINE_FUNC5(CallWindowProcA)
EXTERN_C DW STUB_EXPORT yact_CallWindowProcA(DW *R)
{
	DWORD prev=p1;
	HWND hwnd=(HWND)p2;
	UINT msg=p3;
	WPARAM wp=p4;
	LPARAM lp=p5;
	DWORD r;
	if(prev&0x80000000)// || IsNativeWndProc(prev))
	{
		if((0xffff0000&prev)==0xffff0000)	// special case
			r=CallWindowProcA((WNDPROC)(prev),hwnd,msg,wp,lp);
		else
			r=CallWindowProcA((WNDPROC)(0x7fffffff&prev),hwnd,msg,wp,lp);
	}
	else
		r=EmuExecute(prev,4,(DWORD)hwnd,(DWORD)msg,(DWORD)wp,(DWORD)lp);

	LEAVE(5);
	return (DW)r;
}

//DEFINE_FUNC5(CallWindowProcW)
EXTERN_C DW STUB_EXPORT yact_CallWindowProcW(DW *R)
{
	DWORD prev=p1;
	HWND hwnd=(HWND)p2;
	UINT msg=p3;
	WPARAM wp=p4;
	LPARAM lp=p5;
	DWORD r;
	if(prev&0x80000000)// || IsNativeWndProc(prev))
	{
		if((0xffff0000&prev)==0xffff0000)	// special case
			r=CallWindowProcW((WNDPROC)(prev),hwnd,msg,wp,lp);
		else
			r=CallWindowProcW((WNDPROC)(0x7fffffff&prev),hwnd,msg,wp,lp);
	}
	else
		r=EmuExecute(prev,4,(DWORD)hwnd,(DWORD)msg,(DWORD)wp,(DWORD)lp);

	LEAVE(5);
	return (DW)r;
}

EXTERN_C DW STUB_EXPORT yact_CreateWindowExW(DW *R)
{	
	HMENU M=(HMENU)p10;
	if(M==0)
	{
		LPCWSTR Class=(LPCWSTR)p2;
		WNDCLASSW WC;
		if(GetClassInfoW((HINSTANCE)p11,Class,&WC))
		{
			M=LoadMenu((HINSTANCE)p11,WC.lpszMenuName);
		}
	}

	HWND r=CreateWindowExW(p1,(LPCWSTR)p2,(LPCWSTR)p3,p4,p5,p6,p7,p8,(HWND)p9,
		M,(HINSTANCE)p11,(LPVOID)p12);

	DWORD W=GetLastError();

	LEAVE(12);
	return (DW)r;
}

EXTERN_C DW STUB_EXPORT yact_CreateWindowExA(DW *R)
{	
	HMENU M=(HMENU)p10;
	if(M==0)
	{
		LPCSTR Class=(LPCSTR)p2;
		WNDCLASSA WC;
		if(GetClassInfoA((HINSTANCE)p11,Class,&WC))
		{
			M=LoadMenuA((HINSTANCE)p11,WC.lpszMenuName);
		}
	}

	HWND r=CreateWindowExA(p1,(LPCSTR)p2,(LPCSTR)p3,p4,p5,p6,p7,p8,(HWND)p9,
		M,(HINSTANCE)p11,(LPVOID)p12);

	DWORD W=GetLastError();

	LEAVE(12);
	return (DW)r;
}

EXTERN_C DW STUB_EXPORT yact_wsprintfA(DW *R)
{
	char *out=(char*)p1;
	va_list ap;
	ap=(va_list)(1+&p2);
	int t=vsprintf(out,(char*)p2,ap);
	LEAVE(0);
	return t;
}

EXTERN_C DW STUB_EXPORT yact_wsprintfW(DW *R)
{
	wchar_t*out=(wchar_t*)p1;
	va_list ap;
	ap=(va_list)(1+&p2);
	int t=vswprintf(out,(wchar_t*)p2,ap);
	LEAVE(0);
	return t;
}

//DEFINE_FUNC4(SetTimer)
EXTERN_C DW STUB_EXPORT yact_SetTimer(DW *R)
{
	int t=0;
	if(p4==0)
		t=SetTimer((HWND)p1,p2,p3,NULL);
	else
	{
		t=SetTimer((HWND)p1,p2,p3,(TIMERPROC)CbCreateNativeStdcallCallback(p4,4));
	}
	LEAVE(4);
	return t;
}

EXTERN_C DW STUB_EXPORT yact_RegisterClassExW(DW *R)
{
	char Buff[1024];
	WNDCLASSEXW *w=(WNDCLASSEXW*)p1;
	sprintf_s(Buff,"%S",w->lpszClassName);

//	w->hInstance=GetModuleHandle(0);
	OrigWndProcs[Buff]=(DWORD)w->lpfnWndProc;
	w->lpfnWndProc=WindowProc;

	ATOM r=RegisterClassExW(w);

	DWORD W=GetLastError();

	LEAVE(1);
	return (DW)r;
}

EXTERN_C DW STUB_EXPORT yact_RegisterClassExA(DW *R)
{
	WNDCLASSEXA *w=(WNDCLASSEXA*)p1;

//	w->hInstance=GetModuleHandle(0);
	OrigWndProcs[w->lpszClassName]=(DWORD)w->lpfnWndProc;
	w->lpfnWndProc=WindowProc;

	ATOM r=RegisterClassExA(w);

	DWORD W=GetLastError();

	LEAVE(1);
	return (DW)r;
}

typedef LRESULT WINAPI t_SendMessage(
  _In_  HWND hWnd,
  _In_  UINT Msg,
  _In_  WPARAM wParam,
  _In_  LPARAM lParam
);

BOOL IsWindowClass(HWND hWnd, const char* Class)
{
	char Buff[64]="";
	RealGetWindowClassA(hWnd,Buff,64);		// ignore all subclassing
	return strcmp(Class,Buff)==0;
}

BOOL IsRichEdit(HWND hWnd)
{
	char Buff[64]="";
	RealGetWindowClassA(hWnd,Buff,64);
	return strcmp(RICHEDIT_CLASSA,Buff)==0 || strcmp(RICHEDIT_CLASS10A,Buff)==0 || strcmp("RichEdit20W",Buff)==0 ||
		strcmp("RICHEDIT50W",Buff)==0 || strcmp("RICHEDIT60W",Buff)==0;
}

// preview message and modify its params if needed. For example we allow an emulated prog to subclass a native ListView embedded into a native dialog
// called before the x86 window proc
static BOOL CALLBACK PreProcessMessage(	
    HWND &hWnd,
    UINT &Msg,
    WPARAM &wParam,
    LPARAM &lParam,
	DWORD *Ret)
{
	switch(Msg)
	{
	case EM_SETAUTOCORRECTPROC:
	case EM_STREAMIN:
	case EM_STREAMOUT:
	case EM_SETWORDBREAKPROCEX:
	case EM_SETWORDBREAKPROC:
	case EM_SETHYPHENATEINFO:	// TODO:
		if(IsRichEdit(hWnd))
		{
			*Ret=0;
			return TRUE;		
		}
		break;
	case EM_GETOLEINTERFACE:
		if(IsRichEdit(hWnd))
		{
			*Ret=0;
			*(DWORD*)lParam=0;
			return TRUE;		
		}
		break;
	case EM_SETOLECALLBACK:
		if(IsRichEdit(hWnd))
		{
			*Ret=0;
			return TRUE;		
		}
		break;
	case LVM_SORTITEMS:
		if(IsWindowClass(hWnd,WC_LISTVIEWA))
		{
			lParam=CbCreateX86StdcallCallback((void*)lParam,3);	
			return FALSE;		
		}
		break;
	case LVM_SORTITEMSEX:
		if(IsWindowClass(hWnd,WC_LISTVIEWA))
		{
			lParam=CbCreateX86StdcallCallback((void*)lParam,3);	
			return FALSE;		
		}
		break;
	case LVM_SORTGROUPS:
		if(IsWindowClass(hWnd,WC_LISTVIEWA))
		{
			lParam=CbCreateX86StdcallCallback((void*)lParam,3);	
			return FALSE;		
		}
		break;
	}
	
	return FALSE;
}

// it may possible that for a subclassed window CbCreateNativeStdcallCallback() creates a native callback that later would be converted back by CbCreateX86StdcallCallback in PreProcessMessage. Maybe optimize this later.
// Called before Send/PostMessage
BOOL SendCustomMessage(t_SendMessage SendMessageProc,HWND hWnd,UINT Msg, WPARAM wParam,LPARAM lParam,DWORD *Ret)
{	
	switch(Msg)
	{
	case EM_STREAMIN:
	case EM_STREAMOUT:
		if(IsRichEdit(hWnd))
		{
			EDITSTREAM *es=(EDITSTREAM*)lParam;
			if(es->pfnCallback)
				es->pfnCallback=(EDITSTREAMCALLBACK)CbCreateNativeStdcallCallback((DW)es->pfnCallback,4);
			*Ret=SendMessageProc(hWnd,Msg,wParam,lParam);	
			return TRUE;		
		}
		break;
	case EM_SETAUTOCORRECTPROC:
		if(IsRichEdit(hWnd))
		{
			*Ret=SendMessageProc(hWnd,Msg,CbCreateNativeStdcallCallback(wParam,5),lParam);	
			return TRUE;		
		}
		break;
	case EM_SETWORDBREAKPROC:
	case EM_SETWORDBREAKPROCEX:
		if(IsRichEdit(hWnd))
		{
			*Ret=SendMessageProc(hWnd,Msg,wParam,CbCreateNativeStdcallCallback(lParam,4));	
			return TRUE;		
		}
		break;
	case EM_SETHYPHENATEINFO:
		if(IsRichEdit(hWnd))
		{
			HYPHENATEINFO *hi=(HYPHENATEINFO*)wParam;
			if(hi->pfnHyphenate)
				hi->pfnHyphenate=(void (WINAPI*)(WCHAR*, LANGID, long, HYPHRESULT*))CbCreateNativeStdcallCallback((DW)hi->pfnHyphenate,4);
			*Ret=SendMessageProc(hWnd,Msg,wParam,lParam);	
			return TRUE;		
		}
		break;
	case EM_GETOLEINTERFACE:		// TODO:
		if(IsRichEdit(hWnd))
		{
			*Ret=0;
			*(DWORD*)lParam=0;
			return TRUE;		
		}
		break;
	case EM_SETOLECALLBACK:			// TODO:
		if(IsRichEdit(hWnd))
		{
			*Ret=0;
			return TRUE;		
		}
		break;
	case LVM_SORTITEMS:
		if(IsWindowClass(hWnd,WC_LISTVIEWA))
		{
			*Ret=SendMessageProc(hWnd,Msg,wParam,CbCreateNativeStdcallCallback(lParam,3));	
			return TRUE;		
		}
		break;
	case LVM_SORTITEMSEX:
		if(IsWindowClass(hWnd,WC_LISTVIEWA))
		{
			*Ret=SendMessageProc(hWnd,Msg,wParam,CbCreateNativeStdcallCallback(lParam,3));	
			return TRUE;		
		}
		break;
	case LVM_SORTGROUPS:
		if(IsWindowClass(hWnd,WC_LISTVIEWA))
		{
			*Ret=SendMessageProc(hWnd,Msg,wParam,CbCreateNativeStdcallCallback(lParam,3));	
			return TRUE;		
		}
		break;
	}
	
	return FALSE;
}

//DEFINE_FUNC4(SendMessageA)
EXTERN_C DW STUB_EXPORT yact_SendMessageA(DW *R)
{
	DWORD r=0;
	if(!SendCustomMessage(SendMessageA,(HWND)p1,(UINT)p2,(WPARAM)p3,(LPARAM)p4,&r))
		r=SendMessageA((HWND)p1,(UINT)p2,(WPARAM)p3,(LPARAM)p4);
	return (DW)r;
}

//DEFINE_FUNC4(SendMessageW)
EXTERN_C DW STUB_EXPORT yact_SendMessageW(DW *R)
{
	DWORD r=0;
	HWND HW=(HWND)p1;
	UINT Msg=p2;
	if(!SendCustomMessage(SendMessageW,HW,Msg,(WPARAM)p3,(LPARAM)p4,&r))
		r=SendMessageW(HW,Msg,(WPARAM)p3,(LPARAM)p4);
	return (DW)r;
}

//DEFINE_FUNC4(PostMessageA)
EXTERN_C DW STUB_EXPORT yact_PostMessageA(DW *R)
{
	DWORD r=0;
	if(!SendCustomMessage((t_SendMessage*)PostMessageA,(HWND)p1,(UINT)p2,(WPARAM)p3,(LPARAM)p4,&r))
		r=PostMessageA((HWND)p1,(UINT)p2,(WPARAM)p3,(LPARAM)p4);
	return (DW)r;
}

//DEFINE_FUNC4(PostMessageW)
EXTERN_C DW STUB_EXPORT yact_PostMessageW(DW *R)
{
	DWORD r=0;
	if(!SendCustomMessage((t_SendMessage*)PostMessageW,(HWND)p1,(UINT)p2,(WPARAM)p3,(LPARAM)p4,&r))
		r=PostMessageW((HWND)p1,(UINT)p2,(WPARAM)p3,(LPARAM)p4);
	return (DW)r;
}

//DEFINE_FUNC5(SendDlgItemMessageA)
EXTERN_C DW STUB_EXPORT yact_SendDlgItemMessageA(DW *R)
{
	DWORD r=0;
	HWND DlgI=GetDlgItem((HWND)p1,p2);
	if(DlgI==0)
		return 0;
	if(!SendCustomMessage(SendMessageA,DlgI,(UINT)p3,(WPARAM)p4,(LPARAM)p5,&r))
		r=SendMessageA(DlgI,(UINT)p3,(WPARAM)p4,(LPARAM)p5);
	return (DW)r;
}

//DEFINE_FUNC5(SendDlgItemMessageW)
EXTERN_C DW STUB_EXPORT yact_SendDlgItemMessageW(DW *R)
{
	DWORD r=0;
	HWND DlgI=GetDlgItem((HWND)p1,p2);
	if(DlgI==0)
		return 0;
	if(!SendCustomMessage(SendMessageW,DlgI,(UINT)p3,(WPARAM)p4,(LPARAM)p5,&r))
		r=SendMessageW(DlgI,(UINT)p3,(WPARAM)p4,(LPARAM)p5);
	return (DW)r;
}

//DEFINE_FUNC5(DialogBoxParamW)
EXTERN_C DW STUB_EXPORT yact_DialogBoxParamW(DW *R)
{
	return DialogBoxParamW((HINSTANCE)p1,(LPCWSTR)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

//DEFINE_FUNC5(DialogBoxParamA)
EXTERN_C DW STUB_EXPORT yact_DialogBoxParamA(DW *R)
{
	return DialogBoxParamA((HINSTANCE)p1,(LPCSTR)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

EXTERN_C DW STUB_EXPORT yact_CreateDialogParamW(DW *R)
{
	return (DWORD)CreateDialogParamW((HINSTANCE)p1,(LPCWSTR)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

//DEFINE_FUNC5(DialogBoxParamA)
EXTERN_C DW STUB_EXPORT yact_CreateDialogParamA(DW *R)
{
	return (DWORD)CreateDialogParamA((HINSTANCE)p1,(LPCSTR)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

EXTERN_C DW STUB_EXPORT yact_CreateDialogIndirectParamA(DW *R)
{
	return (DWORD)CreateDialogIndirectParamA((HINSTANCE)p1,(LPCDLGTEMPLATEA)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

EXTERN_C DW STUB_EXPORT yact_CreateDialogIndirectParamW(DW *R)
{
	return (DWORD)CreateDialogIndirectParamW((HINSTANCE)p1,(LPCDLGTEMPLATEW)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

EXTERN_C DW STUB_EXPORT yact_DialogBoxIndirectParamA(DW *R)
{
	return (DWORD)DialogBoxIndirectParamA((HINSTANCE)p1,(LPCDLGTEMPLATEA)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

EXTERN_C DW STUB_EXPORT yact_DialogBoxIndirectParamW(DW *R)
{
	return (DWORD)DialogBoxIndirectParamW((HINSTANCE)p1,(LPCDLGTEMPLATEW)p2,(HWND)p3,(DLGPROC)CbCreateNativeStdcallCallback(p4,4),p5);
}

#if 0
//DEFINE_FUNC4(MessageBoxA)
EXTERN_C DW STUB_EXPORT yact_MessageBoxA(DW *R)
{
	HWND Parent=(HWND)p1;
	DWORD Pid=0;
	GetWindowThreadProcessId(Parent,&Pid);	
	if(Pid!=GetCurrentProcessId())  // don't allow to use the foreign window as parent
		Parent=0;	
	if(!IsWindow(Parent) || !IsWindowVisible(Parent))
		Parent=0;

//	Parent=0;
	DWORD Style=GetWindowLong(Parent,GWL_STYLE);
	DWORD ExStyle=GetWindowLong(Parent,GWL_EXSTYLE);
	WINDOWPLACEMENT wp;
	wp.length=sizeof(wp);
	GetWindowPlacement(Parent,&wp);
	RECT wr;
	GetWindowRect(Parent,&wr);

	DWORD Ret=MessageBoxA(Parent,(LPCSTR)p2,(LPCSTR)p3,p4);
	return Ret;
}
#endif

DW FixResHandle(DW Handle)
{
	if(Handle==0)
	{
		PE_HANDLE Pe=PeLdrFindModuleA(0);
		Handle = (DW)PeLdrGetModuleBase(Pe);
	}
	return Handle;
}
