/*
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * Macros for DirectX and other COM stub DLLs (native code, function implementation)
 *
 */

#ifndef __DLLCOMOBJECT_H
#define __DLLCOMOBJECT_H

#define COMSTUB_EXPORT __fastcall
#define COMFUNC_EXPORT __stdcall
#ifdef __cplusplus
#define EXTERN_C extern "C"
#endif

#ifdef _WINDEF_
#error Do not include this file after windows.h
#endif

#include <windows.h>
#include <string.h>
#include <stddef.h>
#include "callbacks.h"

typedef unsigned int DW;

template <class IVtbl> class ICallbackUnknown
{
private:
    ICallbackUnknown()
    {
    }
    ICallbackUnknown(const ICallbackUnknown&)
    {
    }
    typedef unsigned int COMFUNC_EXPORT t_RefFunc(ICallbackUnknown<IVtbl> *This);
public:
    IVtbl *Vtbl;
    ICallbackUnknown<IVtbl> *IOriginal;
    void *Data;		// may be used to store some data. Data should be freed by caller
    static unsigned int _AddRef(ICallbackUnknown<IVtbl> *This)
    {
        t_RefFunc *Func = (t_RefFunc*) ((void**) This->IOriginal->Vtbl)[1];
        return Func(This->IOriginal);
    }
    static DW COMSTUB_EXPORT AddRef(DW *R)
    {
        return _AddRef((ICallbackUnknown<IVtbl>*)p1);
    }
    static unsigned int _Release(ICallbackUnknown<IVtbl> *This)
    {
        t_RefFunc *Func = (t_RefFunc*) ((void**) This->IOriginal->Vtbl)[2];
        unsigned int Ret = Func(This->IOriginal);
        if (Ret <= 0)
            delete This;
        return Ret;
    }
    static DW COMSTUB_EXPORT Release(DW *R)
    {
        return _Release((ICallbackUnknown<IVtbl>*)p1);
    }
    static unsigned int _QueryInterface(ICallbackUnknown<IVtbl> *This, void *riid, ICallbackUnknown<IVtbl>**Obj)
    {
        *Obj = This;	// dirty hack in the case if QueryInterface is not specially implemented
        _AddRef(This);
#ifdef _DEBUG
        __debugbreak();
#endif
        return 0;   // S_OK
    }
    static DW COMSTUB_EXPORT QueryInterface(DW *R)
    {
        return _QueryInterface((ICallbackUnknown<IVtbl>*)p1, (void*) p2, (ICallbackUnknown<IVtbl>**)p3);
    }
    ICallbackUnknown(void *_IOriginal) : IOriginal((ICallbackUnknown<IVtbl>*)_IOriginal)
    {
        Data = 0;//((ICallbackUnknown<IVtbl>*)_IOriginal)->Data;
        Vtbl = (IVtbl*) malloc(sizeof(IVtbl));
        memset(Vtbl, 0, sizeof(IVtbl));
        *((DWORD*) Vtbl + 0) = CbCreateX86StdcallCallback(ICallbackUnknown::QueryInterface, 3);
        *((DWORD*) Vtbl + 1) = CbCreateX86StdcallCallback(ICallbackUnknown::AddRef, 1);
        *((DWORD*) Vtbl + 2) = CbCreateX86StdcallCallback(ICallbackUnknown::Release, 1);
    }
    ~ICallbackUnknown()
    {
        for (int i = 0; i < sizeof(IVtbl) / sizeof(void*); i++)
            if (*((DWORD*) Vtbl + i))
            {
                //				if(i==2)	// This function is called from Release, so we can't free it
                //					CbCallAtThreadExit(CbFreeCallback,(void*)*((DWORD*)Vtbl+i));	
                //				else
                //					CbFreeCallback(*((DWORD*)Vtbl+i));
                *((DWORD*) Vtbl + i) = 0;
            }
        free(Vtbl);
    }
};

#define DEFINE_CALLBACK(prefix,name,nparams) 	\
    *(DWORD*)(&prefix->Vtbl->name)=CbCreateX86StdcallCallback(prefix##_##name,nparams);

#define DEFINE_VER_CALLBACK(ver,prefix,name,nparams) 	\
    *(DWORD*)(&prefix->Vtbl->name)=CbCreateX86StdcallCallback(prefix##ver##_##name,nparams);

#define DEFINE_CALLBACK1(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,1)
#define DEFINE_CALLBACK2(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,2)
#define DEFINE_CALLBACK3(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,3)
#define DEFINE_CALLBACK4(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,4)
#define DEFINE_CALLBACK5(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,5)
#define DEFINE_CALLBACK6(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,6)
#define DEFINE_CALLBACK7(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,7)
#define DEFINE_CALLBACK8(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,8)
#define DEFINE_CALLBACK9(prefix,type,name) 	DEFINE_CALLBACK(prefix,name,9)
#define DEFINE_CALLBACK10(prefix,type,name) DEFINE_CALLBACK(prefix,name,10)

#define DEFINE_VER_CALLBACK1(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,1)
#define DEFINE_VER_CALLBACK2(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,2)
#define DEFINE_VER_CALLBACK3(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,3)
#define DEFINE_VER_CALLBACK4(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,4)
#define DEFINE_VER_CALLBACK5(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,5)
#define DEFINE_VER_CALLBACK6(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,6)
#define DEFINE_VER_CALLBACK7(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,7)
#define DEFINE_VER_CALLBACK8(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,8)
#define DEFINE_VER_CALLBACK9(ver,prefix,type,name) 	DEFINE_VER_CALLBACK(ver,prefix,name,9)
#define DEFINE_VER_CALLBACK10(ver,prefix,type,name) DEFINE_VER_CALLBACK(ver,prefix,name,10)

#define PROTO_COMFUNC1(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc1(void*);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC2(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc2(void*,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC3(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc3(void*,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC4(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc4(void*,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC5(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc5(void*,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC6(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc6(void*,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC7(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc7(void*,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC8(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc8(void*,DW,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC9(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc9(void*,DW,DW,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);

#define PROTO_COMFUNC10(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc10(void*,DW,DW,DW,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R);


#define DEFINE_COMFUNC1(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc1(void*);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc1*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal); 	\
}

#define DEFINE_COMFUNC2(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc2(void*,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc2*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1]); 	\
}

#define DEFINE_COMFUNC3(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc3(void*,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc3*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2]); 	\
}

#define DEFINE_COMFUNC4(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc4(void*,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc4*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3]); 	\
}

#define DEFINE_COMFUNC5(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc5(void*,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc5*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3],R[4]); 	\
}

#define DEFINE_COMFUNC6(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc6(void*,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc6*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3],R[4],R[5]); 	\
}

#define DEFINE_COMFUNC7(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc7(void*,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc7*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3],R[4],R[5],R[6]); 	\
}

#define DEFINE_COMFUNC8(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc8(void*,DW,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc8*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3],R[4],R[5],R[6],R[7]); 	\
}

#define DEFINE_COMFUNC9(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc9(void*,DW,DW,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc9*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3],R[4],R[5],R[6],R[7],R[8]); 	\
}

#define DEFINE_COMFUNC10(prefix,type,name) 	\
    typedef DW COMFUNC_EXPORT t_ComFunc10(void*,DW,DW,DW,DW,DW,DW,DW,DW);	\
    DW COMSTUB_EXPORT prefix##_##name(DW *R)		\
{	\
    ICallbackUnknown<type> *Tmp=(ICallbackUnknown<type>*)R[0];	\
    return ((t_ComFunc10*)(Tmp->IOriginal->Vtbl->name))(Tmp->IOriginal,R[1],R[2],R[3],R[4],R[5],R[6],R[7],R[8],R[9]); 	\
}

#endif // __DLLYACT_H
