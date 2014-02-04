/* 
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * Macros for stub DLLs (native code, function implementation)
 *
 */

#ifndef __DLLYACT_H
#define __DLLYACT_H
#include <callbacks.h>

#define STUB_EXPORT __declspec(dllexport) __fastcall
#define STUB_IMPORT __declspec(dllimport) __stdcall 
#ifdef __cplusplus
#define EXTERN_C extern "C"
#endif

#ifdef _WINDEF_
#error Do not include this file after windows.h
#endif

typedef unsigned int DW;

#define p1 (R[0])
#define p2 (R[1])
#define p3 (R[2])
#define p4 (R[3])
#define p5 (R[4])
#define p6 (R[5])
#define p7 (R[6])
#define p8 (R[7])
#define p9 (R[8])
#define p10 (R[9])
#define p11 (R[10])
#define p12 (R[11])
#define p13 (R[12])
#define p14 (R[13])
#define p15 (R[14])
#define p16 (R[15])
#define p17 (R[16])
#define p18 (R[17])
#define LEAVE(n) { ; }

#define DEFINE_FUNC_(name) 	\
EXTERN_C DW STUB_IMPORT name(void);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
/* Function should be implemented manually" */ \
return 0; \
}

#define DEFINE_FUNC0(name) 	\
EXTERN_C DW STUB_IMPORT name(void);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name();	\
  LEAVE(0);		\
  return r;		\
}

#define DEFINE_FUNC1(name) 	\
EXTERN_C DW STUB_IMPORT name(DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1);	\
  LEAVE(1);		\
  return r;		\
}

#define DEFINE_FUNC2(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2);	\
  LEAVE(2);		\
  return r;		\
}

#define DEFINE_FUNC3(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3);	\
  LEAVE(3);		\
  return r;		\
}

#define DEFINE_FUNC4(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4);	\
  LEAVE(4);		\
  return r;		\
}

#define DEFINE_FUNC5(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5);	\
  LEAVE(5);		\
  return r;		\
}

#define DEFINE_FUNC6(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6);	\
  LEAVE(6);		\
  return r;		\
}

#define DEFINE_FUNC7(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7);	\
  LEAVE(7);		\
  return r;		\
}

#define DEFINE_FUNC8(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8);	\
  LEAVE(8);		\
  return r;		\
}

#define DEFINE_FUNC9(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9);	\
  LEAVE(9);		\
  return r;		\
}

#define DEFINE_FUNC10(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10);	\
  LEAVE(10);		\
  return r;		\
}

#define DEFINE_FUNC11(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11);	\
  LEAVE(11);		\
  return r;		\
}

#define DEFINE_FUNC12(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12);	\
  LEAVE(12);		\
  return r;		\
}

#define DEFINE_FUNC13(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13);	\
  LEAVE(13);		\
  return r;		\
}

#define DEFINE_FUNC14(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14);	\
  LEAVE(14);		\
  return r;		\
}

#define DEFINE_FUNC15(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15);	\
  LEAVE(15);		\
  return r;		\
}

#define DEFINE_FUNC16(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16);	\
  LEAVE(16);		\
  return r;		\
}

#define DEFINE_FUNC17(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17);	\
  LEAVE(17);		\
  return r;		\
}

#define DEFINE_FUNC18(name) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18);	\
  LEAVE(18);		\
  return r;		\
}

#define DEFINE_FUNC_CB1(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{	\
  R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);  \
  DW r=name(p1);	\
  LEAVE(1);		\
  return r;		\
}

#define DEFINE_FUNC_CB2(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2);	\
  LEAVE(2);		\
  return r;		\
}

#define DEFINE_FUNC_CB3(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3);	\
  LEAVE(3);		\
  return r;		\
}

#define DEFINE_FUNC_CB4(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4);	\
  LEAVE(4);		\
  return r;		\
}

#define DEFINE_FUNC_CB5(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5);	\
  LEAVE(5);		\
  return r;		\
}

#define DEFINE_FUNC_CB6(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6);	\
  LEAVE(6);		\
  return r;		\
}

#define DEFINE_FUNC_CB7(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7);	\
  LEAVE(7);		\
  return r;		\
}

#define DEFINE_FUNC_CB8(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8);	\
  LEAVE(8);		\
  return r;		\
}

#define DEFINE_FUNC_CB9(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9);	\
  LEAVE(9);		\
  return r;		\
}

#define DEFINE_FUNC_CB10(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10);	\
  LEAVE(10);		\
  return r;		\
}

#define DEFINE_FUNC_CB11(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11);	\
  LEAVE(11);		\
  return r;		\
}

#define DEFINE_FUNC_CB12(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12);	\
  LEAVE(12);		\
  return r;		\
}

#define DEFINE_FUNC_CB13(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13);	\
  LEAVE(13);		\
  return r;		\
}

#define DEFINE_FUNC_CB14(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14);	\
  LEAVE(14);		\
  return r;		\
}

#define DEFINE_FUNC_CB15(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15);	\
  LEAVE(15);		\
  return r;		\
}

#define DEFINE_FUNC_CB16(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16);	\
  LEAVE(16);		\
  return r;		\
}

#define DEFINE_FUNC_CB17(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17);	\
  LEAVE(17);		\
  return r;		\
}

#define DEFINE_FUNC_CB18(name,cbpos,ncbparams) 	\
EXTERN_C DW STUB_IMPORT name(DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
EXTERN_C DW STUB_EXPORT yact_##name(DW *R)		\
{ R[cbpos-1]=CbCreateNativeStdcallCallback(R[cbpos-1],ncbparams);\
  DW r=name(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18);	\
  LEAVE(18);		\
  return r;		\
}

#endif // __DLLYACT_H
