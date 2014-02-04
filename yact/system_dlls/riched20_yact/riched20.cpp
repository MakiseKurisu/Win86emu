#include "dllyact_def.h"
#include <windows.h>

HMODULE RichedHMod=0;

/* TODO: 
DEFINE_FUNC3(CreateTextServices)
*/

typedef DW t_Proc4Param(DW,DW,DW,DW);

EXTERN_C DW STUB_EXPORT yact_REExtendedRegisterClass(DW *R)
{
	FARPROC fp=GetProcAddress(RichedHMod,"REExtendedRegisterClass");
	DW Ret=0;
	if(fp)
		Ret=fp();
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_RichEdit10ANSIWndProc(DW *R)
{
	t_Proc4Param *fp=(t_Proc4Param*)GetProcAddress(RichedHMod,"RichEdit10ANSIWndProc");
	DW Ret=0;
	if(fp)
		Ret=fp(p1,p2,p3,p4);
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_RichEditANSIWndProc(DW *R)
{
	t_Proc4Param *fp=(t_Proc4Param*)GetProcAddress(RichedHMod,"RichEditANSIWndProc");
	DW Ret=0;
	if(fp)
		Ret=fp(p1,p2,p3,p4);
	return Ret;
}

EXTERN_C DW STUB_EXPORT yact_Dummy(DW *R)
{
	RichedHMod=LoadLibraryA("riched20.dll");
	return 1;
}
