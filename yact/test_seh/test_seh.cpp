// test_seh.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

/*struct EXCEPTION_REGISTRATION_RECORD {
    struct EXCEPTION_REGISTRATION_RECORD *Next;
    void* Handler;
    };*/

#define MAXIMUM_SUPPORTED_EXTENSION     512
#define SIZE_OF_80387_REGISTERS      80

typedef struct _FLOATING_SAVE_AREA86
{
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

typedef struct _CONTEXT86
{
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

typedef EXCEPTION_DISPOSITION t_eh_handler(
struct _EXCEPTION_RECORD *ExceptionRecord,
    void *EstablisherFrame,
    CONTEXT86 *ContextRecord,
    CONTEXT86 *DispatcherContext);

extern "C" EXCEPTION_DISPOSITION __declspec(naked)
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
            mov     ebp, esp

            push    DispatcherContext
            push    ContextRecord
            push    EstablisherFrame
            push    ExceptionRecord
            mov     ecx, ExceptionRoutine
            call    ecx

            mov     esp, ebp
            pop     ebp
            ret		20
    }
}

extern "C" void __declspec(naked) __stdcall MyRtlpCaptureContext(void *Ctx)
{
    __asm {
        push    ebx
            mov     ebx, [esp + 8]
            mov     dword ptr[ebx + 0B0h], 0
            mov     dword ptr[ebx + 0ACh], 0
            mov     dword ptr[ebx + 0A8h], 0
            mov     dword ptr[ebx + 0A4h], 0
            mov     dword ptr[ebx + 0A0h], 0
            mov     dword ptr[ebx + 9Ch], 0
            mov     word ptr[ebx + 0BCh], cs
            mov     word ptr[ebx + 98h], ds
            mov     word ptr[ebx + 94h], es
            mov     word ptr[ebx + 90h], fs
            mov     word ptr[ebx + 8Ch], gs
            mov     word ptr[ebx + 0C8h], ss
            pushfd
            pop		dword ptr[ebx + 0C0h]
            mov     eax, [ebp + 4]
            mov[ebx + 0B8h], eax
            mov     eax, [ebp + 0]
            mov[ebx + 0B4h], eax
            lea     eax, [ebp + 8]
            mov[ebx + 0C4h], eax
            pop     ebx
            retn    4
    }
}

extern "C" void __declspec(naked) __stdcall MyRtlUnwind(
    EXCEPTION_REGISTRATION_RECORD* TargetFrame,
    PVOID TargetIp,
    PEXCEPTION_RECORD Record,
    PVOID retval)
{
    EXCEPTION_RECORD record;
    CONTEXT86 Ctx;
    DWORD Disp[4];	// DISPATCHER_CONTEXT
    EXCEPTION_REGISTRATION_RECORD* Rec;
    DWORD Ret;

    __asm
    {
        push    ebp
            mov     ebp, esp
            sub     esp, __LOCAL_SIZE
    }

    Ctx.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_SEGMENTS;
    MyRtlpCaptureContext(&Ctx);

    if (!Record)
    {
        record.ExceptionCode = 0xC0000027;
        record.ExceptionFlags = 0;
        record.ExceptionRecord = 0;
        record.ExceptionAddress = (void *) Ctx.Eip;
        record.NumberParameters = 0;
        Record = &record;
    }

    Record->ExceptionFlags |= 2;	// EXCEPTION_UNWINDING

    Rec = (EXCEPTION_REGISTRATION_RECORD*) *(DWORD*) NtCurrentTeb();
    while (Rec != (EXCEPTION_REGISTRATION_RECORD*) -1 && Rec != TargetFrame)
    {
        Ret = ExecuteHandler(Record, Rec, &Ctx, Disp, Rec->Handler);
        Rec = Rec->Next;
        *(DWORD*) NtCurrentTeb() = (DWORD) Rec;
        // We support only ExceptionContinueSearch
        if (Ret != ExceptionContinueSearch)
        {
            ExitProcess(-1);
        }
    }
    // Continue
    __asm {
        mov eax, Ctx.Eax
            mov esi, Ctx.Eip
            mov ebx, Ctx.Ebp
            mov ecx, Ctx.Esp
            mov esp, ecx
            mov ebp, ebx
            jmp esi
    }

    ExitProcess(-1);
}

extern "C" void __declspec(dllexport) __stdcall MyRaiseException(
    __in          DWORD dwExceptionCode,
    __in          DWORD dwExceptionFlags,
    __in          DWORD nNumberOfArguments,
    __in          const ULONG_PTR* lpArguments
    )
{
    DWORD *TEB = (DWORD*) NtCurrentTeb();
    EXCEPTION_REGISTRATION_RECORD* Handler;
    CONTEXT86 Ctx;
    DWORD Disp[4];
    DWORD Ret;
    if (nNumberOfArguments > EXCEPTION_MAXIMUM_PARAMETERS) nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;
    void* ExcAddress = MyRaiseException;

    MyRtlpCaptureContext(&Ctx);

    Ctx.Esp += 12;
    Ctx.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_SEGMENTS;

    Handler = (EXCEPTION_REGISTRATION_RECORD*) TEB[0];

    EXCEPTION_RECORD Er;
    memset(&Er, 0, sizeof(Er));

    Er.ExceptionCode = dwExceptionCode;
    Er.ExceptionFlags = dwExceptionFlags;
    Er.ExceptionRecord = 0;
    Er.ExceptionAddress = ExcAddress;
    Er.NumberParameters = nNumberOfArguments;
    if (lpArguments && nNumberOfArguments > 0)
        memcpy(&Er.ExceptionInformation[0], &lpArguments[0], nNumberOfArguments*sizeof(ULONG_PTR));

    while (Handler && (DWORD) Handler != -1 /*&& !CbIsReturnToHost((DWORD)Handler->Handler)*/)
    {
        Ret = ExecuteHandler(&Er, Handler, &Ctx, Disp, Handler->Handler);
        if (Ret != ExceptionContinueSearch)
        {
            ExitProcess(-1);
            return;
        }
        Handler = Handler->Next;
    }

    ExitProcess(-1);
}

float t = 0;

class TestClass
{
public:
    const char *Str;
    TestClass(const char *a) :Str(a)
    {
        printf("%s constructor\n", a);
    }
    TestClass(const TestClass &a) :Str(a.Str)
    {
        printf("%s copy constructor\n", a);
    }
    ~TestClass()
    {
        printf("%s destructor\n", Str);
    }
} Global("Global");

void ExcFunc()
{
    TestClass Local("In Excfunc");
    throw 123.0 / t;
}

int _tmain(int argc, _TCHAR* argv[])
{
    DWORD Tmp = 0;
    HMODULE HK = LoadLibraryA("kernel32.dll");
    BYTE *RU = (BYTE*) GetProcAddress(HK, "RtlUnwind");
    VirtualProtect(RU, 16, PAGE_EXECUTE_READWRITE, &Tmp);
    RU[0] = 0x68;
    *(DWORD*) (RU + 1) = (DWORD) MyRtlUnwind;
    RU[5] = 0xC3;

    BYTE *RE = (BYTE*) GetProcAddress(HK, "RaiseException");
    VirtualProtect(RE, 16, PAGE_EXECUTE_READWRITE, &Tmp);
    RE[0] = 0x68;
    *(DWORD*) (RE + 1) = (DWORD) MyRaiseException;
    RE[5] = 0xC3;

    TestClass Lm("Local in main");
    try
    {
        TestClass Lm("Local in try");
        ExcFunc();
    }
    catch (const char *ptr)
    {
        printf("Got exception: %s\n", ptr);
    }
    catch (const TestClass &ptr)
    {
        printf("Got exception: %s\n", ptr.Str);
    }
    catch (...)
    {
        printf("Got ... exception\n");
    }
    getchar();
    return 0;
}

