#include "dllyact_def.h"
#include <windows.h>

HMODULE RichedHMod=0;

EXTERN_C DW STUB_EXPORT yact_Dummy(DW *R)
{
//	RichedHMod=LoadLibraryA("riched20.dll");
	RichedHMod=LoadLibraryA("riched32.dll");
#if 0
	WNDCLASSEXA wc;
	wc.cbSize=sizeof(wc);

	if(GetClassInfoExA(0,"RICHEDIT",&wc)==0)
	{
		if(GetClassInfoExA(0,"RichEdit20A",&wc))
		{
			wc.lpszClassName="RICHEDIT";
			RegisterClassExA(&wc);
		}
	}
#endif
	return 1;
}
