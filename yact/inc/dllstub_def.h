/* 
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * Macros for OS kernel DLLs (x86 stub code, emulator transfers calls to native code)
 *
 */

#ifndef __DLLSTUB_H
#define __DLLSTUB_H

#define STUB_EXPORT __declspec(dllexport) __stdcall 
#ifdef __cplusplus
#define EXTERN_C extern "C"
#endif

#ifdef _WINDEF_
#error Do not include this file with windows.h
#endif

typedef unsigned int DW;
struct ModuleDef
{
	const char *Dll;
	const char *Func;
};

// functions with "..." does not prepend underscore to names
#define DEFINE_FUNC_(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT _stub_##name(DW p1,...)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD_(name,ord) 	\
DEFINE_FUNC_(name)	\
EXTERN_C DW STUB_EXPORT _stub_Ord_##ord(DW p1,...)		\
{	\
	return _stub_##name(p1);	\
}

#define DEFINE_FUNC0(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(void)		\
{	\
	DW *p=0;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD0(name,ord) 	\
DEFINE_FUNC0(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(void)		\
{	\
	return stub_##name();	\
}

#define DEFINE_FUNC1(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD1(name,ord) 	\
DEFINE_FUNC1(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1)		\
{	\
	return stub_##name(p1);	\
}

#define DEFINE_FUNC2(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD2(name,ord) 	\
DEFINE_FUNC2(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2)		\
{	\
	return stub_##name(p1,p2);	\
}

#define DEFINE_FUNC3(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD3(name,ord) 	\
DEFINE_FUNC3(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3)		\
{	\
	return stub_##name(p1,p2,p3);	\
}

#define DEFINE_FUNC4(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD4(name,ord) 	\
DEFINE_FUNC4(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4)		\
{	\
	return stub_##name(p1,p2,p3,p4);	\
}

#define DEFINE_FUNC5(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD5(name,ord) 	\
DEFINE_FUNC5(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5);	\
}

#define DEFINE_FUNC6(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD6(name,ord) 	\
DEFINE_FUNC6(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6);	\
}

#define DEFINE_FUNC7(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD7(name,ord) 	\
DEFINE_FUNC7(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7);	\
}

#define DEFINE_FUNC8(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD8(name,ord) 	\
DEFINE_FUNC8(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8);	\
}

#define DEFINE_FUNC9(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD9(name,ord) 	\
DEFINE_FUNC9(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9);	\
}

#define DEFINE_FUNC10(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD10(name,ord) 	\
DEFINE_FUNC10(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10);	\
}

#define DEFINE_FUNC11(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD11(name,ord) 	\
DEFINE_FUNC11(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11);	\
}

#define DEFINE_FUNC12(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD12(name,ord) 	\
DEFINE_FUNC12(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12);	\
}

#define DEFINE_FUNC13(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD13(name,ord) 	\
DEFINE_FUNC13(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13);	\
}

#define DEFINE_FUNC14(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD14(name,ord) 	\
DEFINE_FUNC14(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14);	\
}

#define DEFINE_FUNC15(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD15(name,ord) 	\
DEFINE_FUNC15(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15);	\
}

#define DEFINE_FUNC16(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15,DW p16)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD16(name,ord) 	\
DEFINE_FUNC16(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15,DW p16)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16);	\
}

#define DEFINE_FUNC17(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15,DW p16,DW p17)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD17(name,ord) 	\
DEFINE_FUNC17(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15,DW p16,DW p17)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17);	\
}

#define DEFINE_FUNC18(name) 	\
static const ModuleDef str_##name={DLL_NAME,#name};	\
EXTERN_C DW STUB_EXPORT stub_##name(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15,DW p16,DW p17,DW p18)		\
{	\
	DW *p=&p1;	\
	__asm { mov eax,p }	\
	__asm { jmp f1 }	\
	__asm { mov eax,offset str_##name }	\
f1:	__asm { in eax,0xe5 }	\
	__asm { mov p,eax }	\
	return (DW)p;	\
}

#define DEFINE_FUNC_ORD18(name,ord) 	\
DEFINE_FUNC18(name)	\
EXTERN_C DW STUB_EXPORT stub_Ord_##ord(DW p1,DW p2,DW p3,DW p4,DW p5,DW p6,DW p7,DW p8,DW p9,DW p10,DW p11,DW p12,DW p13,DW p14,DW p15,DW p16,DW p17,DW p18)		\
{	\
	return stub_##name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18);	\
}


#endif // __DLLSTUB_H
