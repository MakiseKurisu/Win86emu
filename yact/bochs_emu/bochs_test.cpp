// bochs_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <new>
using namespace std;
#include "bochs.h"
#include "param_names.h"
#include "cpu/cpu.h"
#include "iodev/iodev.h"
#define EMU_EXPORT __declspec(dllexport)
#include <peldr.h>
#include <x86emul.h>
#include <callbacks.h>
#include <classes.h>
#include <util.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#ifndef _ARM_
//#include <D:\Program Files (x86)\Visual Leak Detector\include\vld.h>
#endif

typedef BX_CPU_C *BX_CPU_C_PTR;
BOCHSAPI BX_CPU_C_PTR *bx_cpu_array = NULL;
BOCHSAPI BX_MEM_C bx_mem;
bx_startup_flags_t bx_startup_flags;
bx_bool bx_user_quit;
Bit8u bx_cpu_count;
bx_pc_system_c bx_pc_system;
bx_debug_t bx_dbg;
bx_devices_c *bx_devicesptr=0;
bx_devices_c bx_devices;
static logfunctions thePluginLog;
logfunctions *pluginlog = &thePluginLog;
bx_gui_c * bx_gui=0;

bx_devices_c::bx_devices_c() {}
bx_devices_c::~bx_devices_c() {}
void bx_devices_c::reset(unsigned int) {}
void bx_devices_c::exit(void) {}
void  bx_gui_c::cleanup(void) {}

BX_MEM_C::BX_MEM_C()
{
}

#define LOG_THIS BX_MEM_THIS
Bit8u* BX_MEM_C::alloc_vector_aligned(Bit32u bytes, Bit32u alignment)
{
  Bit64u test_mask = alignment - 1;
  BX_MEM_THIS actual_vector = new Bit8u [(Bit32u)(bytes + test_mask)];
  if (BX_MEM_THIS actual_vector == 0) {
    BX_PANIC(("alloc_vector_aligned: unable to allocate host RAM !"));
    return 0;
  }
  // round address forward to nearest multiple of alignment.  Alignment
  // MUST BE a power of two for this to work.
  Bit64u masked = ((Bit64u)(BX_MEM_THIS actual_vector + test_mask)) & ~test_mask;
  Bit8u *vector = (Bit8u *) masked;
  // sanity check: no lost bits during pointer conversion
  assert(sizeof(masked) >= sizeof(vector));
  // sanity check: after realignment, everything fits in allocated space
  assert(vector+bytes <= BX_MEM_THIS actual_vector+bytes+test_mask);
  return vector;
}

BX_MEM_C::~BX_MEM_C()
{
  cleanup_memory();
}

void BX_MEM_C::init_memory(Bit64u guest, Bit64u host)
{
}

void BX_MEM_C::allocate_block(Bit32u block)
{
}

Bit64s memory_param_save_handler(void *devptr, bx_param_c *param)
{
  return -1;
}

void memory_param_restore_handler(void *devptr, bx_param_c *param, Bit64s val)
{
}

void BX_MEM_C::register_state()
{
}

void BX_MEM_C::cleanup_memory()
{
}

void BX_MEM_C::load_ROM(const char *path, bx_phy_address romaddress, Bit8u type)
{
}

void BX_MEM_C::load_RAM(const char *path, bx_phy_address ramaddress, Bit8u type)
{

}

#if (BX_DEBUGGER || BX_DISASM || BX_GDBSTUB)
bx_bool BX_MEM_C::dbg_fetch_mem(BX_CPU_C *cpu, bx_phy_address addr, unsigned len, Bit8u *buf)
{
  bx_bool ret = 1;

  memcpy(buf,(void*)addr,len);
  return ret;
}
#endif

void BX_MEM_C::readPhysicalPage(BX_CPU_C *cpu, bx_phy_address addr, unsigned len, void *data)
{
	memcpy(data,(void*)addr,len);
}

void BX_MEM_C::writePhysicalPage(BX_CPU_C *cpu, bx_phy_address addr, unsigned len, void *data)
{
	memcpy((void*)addr,data,len);
}

Bit8u *BX_MEM_C::getHostMemAddr(BX_CPU_C *cpu, bx_phy_address addr, unsigned rw)
{
	return (Bit8u*)addr;
}

/*
 * One needs to provide both a read_handler and a write_handler.
 * XXX: maybe we should check for overlapping memory handlers
 */
  bx_bool
BX_MEM_C::registerMemoryHandlers(void *param, memory_handler_t read_handler,
		memory_handler_t write_handler, bx_phy_address begin_addr, bx_phy_address end_addr)
{
  return 1;
}

bx_bool
BX_MEM_C::unregisterMemoryHandlers(memory_handler_t read_handler, memory_handler_t write_handler,
		bx_phy_address begin_addr, bx_phy_address end_addr)
{
  bx_bool ret = 1;
  return ret;
}

typedef DWORD __fastcall func(DWORD*);

DWORD GetHookAddress(char* Dll, char*FuncName)
{
	char DllPath[1024];
	char Buff[1024];
	strcpy(DllPath,PeLdrGetSystemDirectoryA());
	int tmp=strlen(DllPath);
	strcpy(DllPath+tmp-2,"NT\\");
	strcat(DllPath,Dll);
	if((DWORD)FuncName>65545)
	{
#ifdef _ARM_
		sprintf(Buff,"yact_%s",FuncName);
#else
		sprintf(Buff,"@yact_%s@4",FuncName);
#endif
	} else
	{
#ifdef _ARM_
		sprintf(Buff,"yact_Ord%",(int)FuncName);
#else
		sprintf(Buff,"@yact_Ord%d4",(int)FuncName);
#endif
	}
	HMODULE H=LoadLibraryA(DllPath);
	if(H==0)
	{
#ifdef _DEBUG
		printf("Error %d loading '%s'\n",GetLastError(),Dll);
#endif
		LogErr("Error %d loading '%s'\n",GetLastError(),Dll);
	}
#ifdef _DEBUG
	LogPrint("Binding %s from %s\n",Buff,Dll);
#endif
	DWORD R=(DWORD)GetProcAddress(H,Buff);
	return R;
}

unsigned int ProcessCallback(unsigned int reg_eax, unsigned int reg_eip)
{
	DWORD *Param=(DWORD*)reg_eax;
	DWORD Func=*(DWORD*)(reg_eip-4);

	if((0x80000000&Func)==0)
	{
		Func=0x80000000|(DWORD)GetHookAddress(((char**)Func)[0],((char**)Func)[1]);
#if 1 //def _DEBUG
		if(0x80000000==Func)
		{
			Func=*(DWORD*)(reg_eip-4);
			char *Dll=((char**)Func)[0];
			char *Name=((char**)Func)[1];
#ifdef _DEBUG
			printf("Func %s not found in %s\n",Name,Dll);
#endif
			LogErr("Func %s not found in %s\n",Name,Dll);
			exit(0);
		}
#endif
		*(DWORD*)(reg_eip-4)=Func;
	}

	if(0x80000000==Func)
#ifndef _DEBUG
			goto Skip;
#else
			exit(0);
#endif
	int tmp=0;
	__try {
		tmp=((func*)(0x7fffffff&Func))(Param);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
#ifdef _DEBUG
		LogInfo("Exception in callback!");
#endif
	}
Skip:
	reg_eax=tmp;
	return reg_eax;
}

Bit32u bx_devices_c::inp(Bit16u addr, unsigned io_len)
{
	return 0;
}

void   bx_devices_c::outp(Bit16u addr, Bit32u value, unsigned io_len)
{
}

int bx_atexit(void)
{
	return 0;
}

int bx_begin_simulation (int argc, char *argv[])
{
	return 0;
}

void bx_gui_c::update_drive_status_buttons(void)
{
}

#define RETURN(val,end)							      \
    do { if (endptr != NULL) *endptr = (char *) (end);		      \
	 return val; } while (0)

Bit64u bx_strtoull (const char *nptr, char **endptr, int baseignore)
{
  int negative;			/* The sign of the number.  */
  int exponent;			/* Exponent of the number.  */

  /* Numbers starting `0X' or `0x' have to be processed with base 16.  */
  int base = 10;

  /* Number of bits currently in result value.  */
  int bits;

  /* Running pointer after the last character processed in the string.  */
  const char *cp, *tp;
  /* Start of significant part of the number.  */
  const char *startp, *start_of_digits;
  /* Total number of digit and number of digits in integer part.  */
  int dig_no;
  /* Contains the last character read.  */
  char c;

  Bit64s n = 0;
  char const *p;

  /* Prepare number representation.  */
  exponent = 0;
  negative = 0;
  bits = 0;

  /* Parse string to get maximal legal prefix.  We need the number of
     characters of the integer part, the fractional part and the exponent.  */
  cp = nptr - 1;
  /* Ignore leading white space.  */
  do
    c = *++cp;
  while (isspace (c));

  /* Get sign of the result.  */
  if (c == '-')
  {
     negative = 1;
     c = *++cp;
  }
  else if (c == '+')
    c = *++cp;

  if (c < '0' || c > '9')
  {
     /* It is really a text we do not recognize.  */
     RETURN (0, nptr);
  }

  /* First look whether we are faced with a hexadecimal number.  */
  if (c == '0' && tolower (cp[1]) == 'x')
  {
     /* Okay, it is a hexa-decimal number.  Remember this and skip
        the characters.  BTW: hexadecimal numbers must not be
        grouped.  */
     base = 16;
     cp += 2;
     c = *cp;
  }

  /* Record the start of the digits, in case we will check their grouping.  */
  start_of_digits = startp = cp;

  /* Ignore leading zeroes.  This helps us to avoid useless computations.  */
  while (c == '0')
    c = *++cp;

  /* If no other digit but a '0' is found the result is 0.0.
     Return current read pointer.  */
  if ((c < '0' || c > '9')
      && (base == 16 && (c < tolower ('a') || c > tolower ('f')))
      && (base == 16 && (cp == start_of_digits || tolower (c) != 'p'))
      && (base != 16 && tolower (c) != 'e'))
    {
      tp = start_of_digits;
      /* If TP is at the start of the digits, there was no correctly
	 grouped prefix of the string; so no number found.  */
      RETURN (0, tp == start_of_digits ? (base == 16 ? cp - 1 : nptr) : tp);
    }

  /* Remember first significant digit and read following characters until the
     decimal point, exponent character or any non-FP number character.  */
  startp = cp;
  dig_no = 0;
  while (1)
  {
      if ((c >= '0' && c <= '9')
	  || (base == 16 && tolower (c) >= 'a' && tolower (c) <= 'f'))
	++dig_no;
      else
	break;
      c = *++cp;
  }

  /* The whole string is parsed.  Store the address of the next character.  */
  if (endptr)
    *endptr = (char *) cp;

  if (dig_no == 0)
    return 0;

  for (p=start_of_digits; p!=cp; p++) {
    n = n * (Bit64s)base;
    c = tolower (*p);
    c = (c >= 'a') ? (10+c-'a') : c-'0';
    n = n + (Bit64s)c;
    //printf ("after shifting in digit %c, n is %lld\n", *p, n);
  }
  return negative? -n : n;
}

void bx_init_options(void);

void __cdecl newhandler( )
{
   LogErr("Out of Memory or corrupt heap in new()\n");
   throw bad_alloc( );
   return;
}

EMU_EXPORT BOOL EmuInitialize(void)
{
	static bool Initialized=false;
	if(Initialized)
		return true;
	Initialized=true;

#if 0 //def _DEBUG
   _CrtSetDbgFlag (
      _CRTDBG_ALLOC_MEM_DF |
      _CRTDBG_LEAK_CHECK_DF);
   _CrtSetReportMode ( _CRT_ERROR,
      _CRTDBG_MODE_DEBUG);
#endif 
	bx_cpu_count=1;
	bx_init_siminterface();
	bx_init_options();

	SIM->set_init_done(1);

	set_new_handler(newhandler);

	return TRUE;
}

/*
extern "C" EMU_EXPORT void L_Lock()
{
}

extern "C" EMU_EXPORT void L_Unlock() 
{
}
*/

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

struct __EXCEPTION_REGISTRATION_RECORD {
	struct __EXCEPTION_REGISTRATION_RECORD *Next;
    void* Handler;
};

LPEXCEPTION_POINTERS SaveExceptionInfo(LPEXCEPTION_POINTERS Exc)
{
	static EXCEPTION_RECORD ER;
	static CONTEXT Ct;
	static EXCEPTION_POINTERS Exp={&ER,(CONTEXT*)&Ct};

	memcpy(&ER,Exc->ExceptionRecord,sizeof(ER));
	memcpy(&Ct,Exc->ContextRecord,sizeof(Ct));
	return &Exp;
}

CList CpuList;
CList StackList;
/*
EXCEPTION_DISPOSITION eh_handler_ (
            struct _EXCEPTION_RECORD *ExceptionRecord,
            void *EstablisherFrame,
            struct _CONTEXT *ContextRecord,
            struct _DISPATCHER_CONTEXT *DispatcherContext);
*/
static void ExceptExec(BX_CPU_C_PTR CPU)
{
		CPU->cpu_loop(0);
		return;
	BOCHSAPI BX_CPU_C_PTR *bx_cpu_array = &CPU;
	LPEXCEPTION_POINTERS Exc=0;

	__try 
	{
		CPU->cpu_loop(0);
	}
	__except((Exc=SaveExceptionInfo(GetExceptionInformation()))?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_EXECUTION)
	{
		DWORD *TEB=(DWORD*)PeLdrGetCurrentTeb(); 
		puts("Exception!");
		__EXCEPTION_REGISTRATION_RECORD* Handler=(__EXCEPTION_REGISTRATION_RECORD*)TEB[0];
		CONTEXT86 Ctx;
		void* ExcAddress=(void*)BX_CPU(0)->gen_reg[BX_32BIT_REG_EIP].dword.erx;

		Ctx.Eip=(DWORD)ExcAddress;
		Ctx.Esp=BX_CPU(0)->gen_reg[BX_32BIT_REG_ESP].dword.erx;
		Ctx.Ebp=BX_CPU(0)->gen_reg[BX_32BIT_REG_EBP].dword.erx;

		while(Handler && (DWORD)Handler!=-1 && !CbIsReturnToHost((DWORD)Handler->Handler))
		{
			EXCEPTION_RECORD Er;

			Er.ExceptionCode=Exc->ExceptionRecord->ExceptionCode;
			Er.ExceptionFlags=Exc->ExceptionRecord->ExceptionFlags;
			Er.ExceptionRecord=(EXCEPTION_RECORD*)-1;
			Er.ExceptionAddress=ExcAddress;
			Er.NumberParameters=Exc->ExceptionRecord->NumberParameters;
			memcpy(&Er.ExceptionInformation[0],&Exc->ExceptionRecord->ExceptionInformation[0],EXCEPTION_MAXIMUM_PARAMETERS*sizeof(DWORD));

			printf("Calling exception handler %08X\n",Handler->Handler);
			DWORD Ret=EmuExecute((DWORD)Handler->Handler,4,Exc->ExceptionRecord,Handler,&Ctx,&Ctx);
			printf("Handler returned %08X\n",Ret);
			if(Ret!=ExceptionContinueSearch)
			{
/*				__EXCEPTION_REGISTRATION_RECORD* Rec=(__EXCEPTION_REGISTRATION_RECORD*)TEB[0];
				while(Rec!=Handler)
				{
					EXCEPTION_RECORD ErUw;

					puts("SEH: Unwinding...");

					ErUw.ExceptionCode=0xC0000027;	// Unwind
					ErUw.ExceptionFlags=2;			// Unwind
					ErUw.ExceptionRecord=(EXCEPTION_RECORD*)-1;
					ErUw.ExceptionAddress=ExcAddress;
					ErUw.NumberParameters=0;
					
					Rec=Rec->Next;
					DWORD Ret=EmuExecute((DWORD)Rec->Handler,4,&ErUw,Rec,&Ctx,&Ctx);
				}*/
				BX_CPU(0)->gen_reg[BX_32BIT_REG_EIP].dword.erx = (DWORD)Ctx.Eip;
				BX_CPU(0)->gen_reg[BX_32BIT_REG_ESP].dword.erx=Ctx.Esp;
				BX_CPU(0)->gen_reg[BX_32BIT_REG_EBP].dword.erx=Ctx.Ebp;

				puts("Resuming exception...");
				return;
			}

			Handler=Handler->Next;
		}

		puts("Unhandled exception...");
//		ExitProcess(Exc->ExceptionRecord->ExceptionCode);
//		EmuExecute((DWORD)Exc->Handler,0);
		BX_CPU(0)->gen_reg[BX_32BIT_REG_EIP].dword.erx = (DWORD)Handler->Handler;
	}
}

void* AllocStack(int Size)
{
	DWORD Tmp;
	void *Ptr=VirtualAlloc(0,Size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if(Ptr==0)
		return 0;

#ifdef _DEBUG
	memset(Ptr,0xbe,Size);
	VirtualProtect(Ptr,PeLdrGetPageSize(),PAGE_READWRITE|PAGE_GUARD,&Tmp);
#endif
	return Ptr;
}

void FreeStack(void *Ptr)
{
	VirtualFree(Ptr,0,MEM_RELEASE);
}

void ReuseBX_CPU(void *Ptr)
{
	BX_CPU_C_PTR tmp=(BX_CPU_C_PTR)Ptr;
	CpuList.AddHead(tmp);
}

void ReuseStack(void *Ptr)
{
	StackList.AddHead(Ptr);
}

unsigned int GetFsBase()
{
	return (DWORD)PeLdrGetCurrentTeb();
}

EMU_EXPORT DWORD EmuExecute(DWORD Addr, int NParams,...)
{
	static int id=0;
	BX_CPU_C_PTR tmp;
	BOCHSAPI BX_CPU_C_PTR *bx_cpu_array = &tmp;

	BX_CPU(0)=(BX_CPU_C_PTR)CpuList.GetHead();
	if(BX_CPU(0)==0)
	{
		BX_CPU(0)=new BX_CPU_C(id++);
		BX_CPU(0)->reset(BX_RESET_SOFTWARE);

		// CS deltas
		BX_CPU(0)->sregs[BX_SEG_REG_CS].cache.u.segment.base = 0x00000000;
		BX_CPU(0)->sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xFFFFFFFF;
		BX_CPU(0)->sregs[BX_SEG_REG_CS].cache.u.segment.g   = 1; // page granularity
		BX_CPU(0)->sregs[BX_SEG_REG_CS].cache.u.segment.d_b = 1; // 32bit

		// DS deltas
		BX_CPU(0)->sregs[BX_SEG_REG_DS].cache.u.segment.base = 0x00000000;
		BX_CPU(0)->sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled = 0xFFFFFFFF;
		BX_CPU(0)->sregs[BX_SEG_REG_DS].cache.u.segment.g   = 1; // page granularity
		BX_CPU(0)->sregs[BX_SEG_REG_DS].cache.u.segment.d_b = 1; // 32bit

		// ES deltas
		BX_CPU(0)->sregs[BX_SEG_REG_ES].cache.u.segment.base = 0x00000000;
		BX_CPU(0)->sregs[BX_SEG_REG_ES].cache.u.segment.limit_scaled = 0xFFFFFFFF;
		BX_CPU(0)->sregs[BX_SEG_REG_ES].cache.u.segment.g   = 1; // page granularity
		BX_CPU(0)->sregs[BX_SEG_REG_ES].cache.u.segment.d_b = 1; // 32bit

		// FS deltas
		BX_CPU(0)->sregs[BX_SEG_REG_FS].cache.u.segment.base = (DWORD)PeLdrGetCurrentTeb();
		BX_CPU(0)->sregs[BX_SEG_REG_FS].cache.u.segment.limit_scaled = 0xFFFFFFFF;
		BX_CPU(0)->sregs[BX_SEG_REG_FS].cache.u.segment.g   = 1; // page granularity
		BX_CPU(0)->sregs[BX_SEG_REG_FS].cache.u.segment.d_b = 1; // 32bit

		// GS deltas
		BX_CPU(0)->sregs[BX_SEG_REG_GS].cache.u.segment.base = 0x00000000;
		BX_CPU(0)->sregs[BX_SEG_REG_GS].cache.u.segment.limit_scaled = 0xFFFFFFFF;
		BX_CPU(0)->sregs[BX_SEG_REG_GS].cache.u.segment.g   = 1; // page granularity
		BX_CPU(0)->sregs[BX_SEG_REG_GS].cache.u.segment.d_b = 1; // 32bit

		// SS deltas
		BX_CPU(0)->sregs[BX_SEG_REG_SS].cache.u.segment.base = 0x00000000;
		BX_CPU(0)->sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled = 0xFFFFFFFF;
		BX_CPU(0)->sregs[BX_SEG_REG_SS].cache.u.segment.g   = 1; // page granularity
		BX_CPU(0)->sregs[BX_SEG_REG_SS].cache.u.segment.d_b = 1; // 32bit

		// CR0 deltas
		BX_CPU(0)->cr0.set_PE(1); // protected mode
		BX_CPU(0)->cr4.set_OSFXSR(0);	// no SSE
		BX_CPU(0)->set_IF(1);
		BX_CPU(0)->the_i387.cwd=GetFPUCW();	
		BX_CPU(0)->the_i387.swd=0x122;
		BX_CPU(0)->the_i387.twd=0xffff;
		
		BX_CPU(0)->handleCpuModeChange();
		//BX_CPU(0)->handleSseModeChange();
		BX_CPU(0)->trace=0;
	}
	CbCallAtThreadExit(ReuseBX_CPU,BX_CPU(0));

	void *Stack=StackList.GetHead();
	if(Stack==0)
		Stack=AllocStack(EMU_STACK_SIZE);
	CbCallAtThreadExit(ReuseStack,Stack);

	DWORD my_reg_esp = BX_CPU(0)->gen_reg[BX_32BIT_REG_ESP].dword.erx = EMU_STACK_SIZE-128+(DWORD)Stack;
	BX_CPU(0)->gen_reg[BX_32BIT_REG_EIP].dword.erx = (DWORD)Addr;

	va_list args;
	va_start(args,NParams);

	*(DWORD*)my_reg_esp=CbReturnToHost();

	for(int i=0; i<NParams; i++)
		((DWORD*)my_reg_esp)[i+1]=va_arg(args,int);

	DWORD *TEB=(DWORD*)PeLdrGetCurrentTeb();
	TEB[1]=my_reg_esp;	// Fill stack top and size for Borland
	TEB[2]=EMU_STACK_SIZE;

	while (1) {
/*		printf(" EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x\n",
			  BX_CPU(0)->gen_reg[BX_32BIT_REG_EAX].dword.erx, BX_CPU(0)->gen_reg[BX_32BIT_REG_EBX].dword.erx, BX_CPU(0)->gen_reg[BX_32BIT_REG_ECX].dword.erx, BX_CPU(0)->gen_reg[BX_32BIT_REG_EDX].dword.erx);
		printf(" ESP=%08x  EBP=%08x  ESI=%08x  EDI=%08x\n",
			  BX_CPU(0)->gen_reg[BX_32BIT_REG_ESP].dword.erx, BX_CPU(0)->gen_reg[BX_32BIT_REG_EBP].dword.erx, BX_CPU(0)->gen_reg[BX_32BIT_REG_ESI].dword.erx, BX_CPU(0)->gen_reg[BX_32BIT_REG_EDI].dword.erx);
		printf(" EIP=%08x (%08x)\n", BX_CPU(0)->gen_reg[BX_32BIT_REG_EIP].dword.erx,
			(unsigned) BX_CPU(0)->prev_rip);		*/
		BX_CPU(0)->activity_state=0;
		ExceptExec(BX_CPU(0));
		if (bx_pc_system.kill_bochs_request)
			break;
		if(CbIsReturnToHost(BX_CPU(0)->gen_reg[BX_32BIT_REG_EIP].dword.erx))
			break;
	}

	DWORD ret=BX_CPU(0)->gen_reg[BX_32BIT_REG_EAX].dword.erx;

	CbRemoveCallAtThreadExit(ReuseStack,Stack);
	ReuseStack(Stack);

	CbRemoveCallAtThreadExit(ReuseBX_CPU,BX_CPU(0));
	ReuseBX_CPU(BX_CPU(0));

	return ret;
}
