#include "dllstub_def.h"
#include <windows.h>
#include <stdio.h>
#include <util.h>
#include <intrin.h>

//DEFINE_FUNC4(RtlUnwind)

/*struct EXCEPTION_REGISTRATION_RECORD {
	struct EXCEPTION_REGISTRATION_RECORD *Next;
    void* Handler;
};*/

#define MAXIMUM_SUPPORTED_EXTENSION     512
#define SIZE_OF_80387_REGISTERS      80

typedef struct _FLOATING_SAVE_AREA86 {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   TagWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    UCHAR   RegisterArea[SIZE_OF_80387_REGISTERS];
    ULONG   Cr0NpxState;
} FLOATING_SAVE_AREA86;

typedef struct _CONTEXT86 {
    ULONG ContextFlags;
    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;
    FLOATING_SAVE_AREA86 FloatSave;
    ULONG   SegGs;
    ULONG   SegFs;
    ULONG   SegEs;
    ULONG   SegDs;
    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;
    ULONG   Ebp;
    ULONG   Eip;
    ULONG   SegCs;              // MUST BE SANITIZED
    ULONG   EFlags;             // MUST BE SANITIZED
    ULONG   Esp;
    ULONG   SegSs;
    UCHAR   ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT86;

typedef EXCEPTION_DISPOSITION t_eh_handler (
            struct _EXCEPTION_RECORD *ExceptionRecord,
            void *EstablisherFrame,
            CONTEXT86 *ContextRecord,
            CONTEXT86 *DispatcherContext);

extern "C" EXCEPTION_DISPOSITION __declspec(naked) __stdcall
ExecuteHandler(
   IN PEXCEPTION_RECORD ExceptionRecord,
   IN PVOID EstablisherFrame,
   IN OUT CONTEXT86* ContextRecord,
   IN OUT PVOID DispatcherContext,
   IN void* ExceptionRoutine
   )
{
	__asm {
		push    ebp
		mov     ebp,esp

		push    DispatcherContext
		push    ContextRecord
		push    EstablisherFrame
		push    ExceptionRecord
		mov     ecx,ExceptionRoutine
		call    ecx

		mov     esp,ebp
		pop     ebp
		ret		20
	}
}

#define READDW(addr) (*(DWORD*)(addr))

extern "C" void __declspec(naked) __declspec(dllexport) __stdcall stub_RtlUnwind(
	EXCEPTION_REGISTRATION_RECORD* TargetFrame, 
	PVOID TargetIp,
	PEXCEPTION_RECORD Record, 
	PVOID retval)
{
	DWORD _Esp, _Ebp;
    EXCEPTION_RECORD record;
	CONTEXT86 Ctx;
	DWORD Disp[4];	// DISPATCHER_CONTEXT
	EXCEPTION_REGISTRATION_RECORD* Rec;
	DWORD Ret;

	__asm {
		mov		eax,esp
		mov		ecx,ebp
		push	ebp
		mov		ebp,esp
		sub		esp,__LOCAL_SIZE
		mov		_Esp,eax
		mov		_Ebp,ecx
	}	// For unwind to work - we need to save exact ESP and EBP values of a caller

	Ctx.Esp=_Esp+16+4;	// 4 params + retaddr
	Ctx.Ebp=_Ebp;
	Ctx.Eip=READDW(_Esp);
	
	LogInfo("RtlUnwind %08X,%08X,%08X,%08X called from EIP=%08X ESP=%08X\n",TargetFrame,TargetIp,Record,retval,Ctx.Eip,Ctx.Ebp);

	Ctx.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_SEGMENTS;
	Ctx.EFlags=__readeflags();
	Ctx.SegCs=Ctx.SegDs=Ctx.SegEs=Ctx.SegFs=Ctx.SegGs=Ctx.SegSs=0;
	Ctx.Eax=Ctx.Ebx=Ctx.Ecx=Ctx.Edx=Ctx.Esi=Ctx.Edi=0;
	Ctx.Eax=(DWORD) retval;

    if (!Record)
    {
        record.ExceptionCode    = 0xC0000027;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = 0;
		record.ExceptionAddress = _ReturnAddress();
        record.NumberParameters = 0;
        Record = &record;
    }
	LogInfo("RtlUnwind exception %08X at %08X\n",Record->ExceptionCode,Record->ExceptionAddress);

    Record->ExceptionFlags |= 2;	// EXCEPTION_UNWINDING

	Rec=(EXCEPTION_REGISTRATION_RECORD*)*(DWORD*)NtCurrentTeb();
	while(Rec!=(EXCEPTION_REGISTRATION_RECORD*)-1 && Rec!=TargetFrame)
	{
		LogInfo("RtlUnwind executing handler: %08X\n",Rec->Handler);
		Ret=ExecuteHandler(Record,Rec,&Ctx,Disp,Rec->Handler);
		Rec=Rec->Next;
		*(DWORD*)NtCurrentTeb()=(DWORD)Rec;
		// We support only ExceptionContinueSearch
		if(Ret!=ExceptionContinueSearch)
		{
			LogErr("RtlUnwind: unhandled");
			ExitProcess(-1);
		}
	}
	LogErr("RtlUnwind: continuing execution, EIP=%08x, ESP=%08X",Ctx.Eip,Ctx.Esp);
	// Continue
	__asm {
		mov eax,Ctx.Eax
		mov esi,Ctx.Eip
		mov ebx,Ctx.Ebp
		mov ecx,Ctx.Esp
		mov esp,ecx
		mov ebp,ebx
		jmp esi
	}
}

extern "C" void __declspec(dllexport) __stdcall stub_RaiseException(
  __in          DWORD dwExceptionCode,
  __in          DWORD dwExceptionFlags,
  __in          DWORD nNumberOfArguments,
  __in          const ULONG_PTR* lpArguments
)
{
	DWORD *TEB;
	EXCEPTION_REGISTRATION_RECORD* Handler;
	CONTEXT86 Ctx;
	DWORD Disp[4];
	DWORD Ret;

	if(CompatGetInt("NoRaiseException"))
	{
		LogWarn("Ignoring RaiseException: %08X,%08X,%08X EIP=%08X\n",dwExceptionCode,dwExceptionFlags,nNumberOfArguments,_ReturnAddress());
		return;
	}

	TEB=(DWORD*)NtCurrentTeb();
	LogInfo("RaiseException: %08X,%08X,%08X,%08X TEB=%08X EIP=%08X\n",dwExceptionCode,dwExceptionFlags,nNumberOfArguments,lpArguments,TEB,_ReturnAddress());
	
	if (nNumberOfArguments > EXCEPTION_MAXIMUM_PARAMETERS) nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;

	Ctx.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_SEGMENTS;
	__asm {
		mov Ctx.Esp,esp
		mov Ctx.Ebp,ebp
	}	// for RaiseException we don't need the exact register values
	Ctx.EFlags=__readeflags();
	Ctx.SegCs=Ctx.SegDs=Ctx.SegEs=Ctx.SegFs=Ctx.SegGs=Ctx.SegSs=0;
	Ctx.Eax=Ctx.Ebx=Ctx.Ecx=Ctx.Edx=Ctx.Esi=Ctx.Edi=0;
	Ctx.Eax=dwExceptionCode;
	Ctx.Eip=(DWORD)_ReturnAddress();
	void* ExcAddress=(void*)Ctx.Eip;

	Handler=(EXCEPTION_REGISTRATION_RECORD*)TEB[0];

	EXCEPTION_RECORD Er;
	memset(&Er,0,sizeof(Er));

	Er.ExceptionCode=dwExceptionCode;
	Er.ExceptionFlags=dwExceptionFlags;
	Er.ExceptionRecord=0;
	Er.ExceptionAddress=ExcAddress;
	Er.NumberParameters=nNumberOfArguments;
	if(lpArguments && nNumberOfArguments>0)
		memcpy(&Er.ExceptionInformation[0],&lpArguments[0],nNumberOfArguments*sizeof(ULONG_PTR));

	while(Handler && (DWORD)Handler!=-1 /*&& !CbIsReturnToHost((DWORD)Handler->Handler)*/)
	{
		LogInfo("RaiseException executing handler: %08X\n",Handler->Handler);
		Ret=ExecuteHandler(&Er,Handler,&Ctx,Disp,Handler->Handler);
		LogInfo("RaiseException handler returned: %08X\n",Ret);
		if(Ret==ExceptionContinueExecution)
		{
			if((DWORD)_ReturnAddress()!=Ctx.Eip)
				LogInfo("FIXME: Continuing execution EIP=%08X, ESP=%08X, but we just return to caller (EIP=%08X)\n",Ctx.Eip,Ctx.Esp,_ReturnAddress());
			else
				LogInfo("Continuing execution after RaiseException");
			return;			
		}
		if(Ret!=ExceptionContinueSearch)
		{
			LogInfo("Unsupported exception handler return value, terminating program\n");
			ExitProcess(-1);
			return;
		}
		Handler=Handler->Next;
	}

	LogErr("RaiseException: unhandled");
	ExitProcess(-1);
}

extern "C" BOOL __declspec(dllexport) __stdcall stub_IsEmulatorPresent(void)
{
	return TRUE;
}

extern "C" DWORD __declspec(dllexport) __stdcall stub_EncodePointer(DWORD Ptr)
{
	return Ptr^0xC0DEDA7A;
}

extern "C" DWORD __declspec(dllexport) __stdcall stub_DecodePointer(DWORD Ptr)
{
	return Ptr^0xC0DEDA7A;
}

