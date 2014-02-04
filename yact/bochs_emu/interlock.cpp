#include <windows.h>

#ifdef _ARM_
extern "C" __declspec(naked) void volatile _InterlockedIncrement8(unsigned char *ptr)	
{
/*
F3BF 8F5B            dmb         ish  
0002                 mov         r2,r0  
E8D2 0F4F            LDREXB.W        R0, [R2]
3001                 adds        r0,#1  
E8C2 0F43            STREXB.W        R3, R0, [R2]
2B00                 cmp         r3,#0  
D1F8                 bne         yact_InterlockedIncrement+6h (69A91366h)  
F3BF 8F5B            dmb         ish  
4770				 BX			  lr
*/
	__emit(0xF3BF); 
	__emit(0x8F5B);
	__emit(0x0002);
	__emit(0xE8D2);
	__emit(0x0F4F);
	__emit(0x3001);
	__emit(0xE8C2);
	__emit(0x0F43);
	__emit(0x2B00);
	__emit(0xD1F8);
	__emit(0xF3BF);
	__emit(0x8F5B);
	__emit(0x4770);
}

extern "C" __declspec(naked) void volatile _InterlockedDecrement8(unsigned char *ptr)	
{
/*
F3BF 8F5B            dmb         ish  
0002                 mov         r2,r0  
E8D2 0F4F            LDREXB.W        R0, [R2]
3801                 subs        r0,#1  
E8C2 0F43            STREXB.W        R3, R0, [R2]
2B00                 cmp         r3,#0  
D1F8                 bne         yact_InterlockedIncrement+6h (69A91366h)  
F3BF 8F5B            dmb         ish  
4770				 BX			  lr
*/
	__emit(0xF3BF); 
	__emit(0x8F5B);
	__emit(0x0002);
	__emit(0xE8D2);
	__emit(0x0F4F);
	__emit(0x3801);
	__emit(0xE8C2);
	__emit(0x0F43);
	__emit(0x2B00);
	__emit(0xD1F8);
	__emit(0xF3BF);
	__emit(0x8F5B);
	__emit(0x4770);
}
#else
extern "C" __declspec(naked) void volatile _InterlockedIncrement8(unsigned char *_ptr)	
{
	__asm {
		push ebp
		mov ebp,esp
		mov eax,_ptr
		lock inc byte ptr [eax]
		movzx eax,byte ptr [eax]
		pop ebp
		ret
	}
}

extern "C" __declspec(naked) void volatile _InterlockedDecrement8(unsigned char *_ptr)	
{
	__asm {
		push ebp
		mov ebp,esp
		mov eax,_ptr
		lock dec byte ptr [eax]
		movzx eax,byte ptr [eax]
		pop ebp
		ret
	}
}
#endif
