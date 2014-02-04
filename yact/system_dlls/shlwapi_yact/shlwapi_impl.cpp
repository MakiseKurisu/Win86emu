#include "dllyact_def.h"
#include <windows.h>
#include <util.h>
#include <shlwapi.h>
#include "peldr.h"
#include "callbacks.h"
#include "x86emul.h"

//DEFINE_FUNC_CB4(SHCreateThread,1,1)
EXTERN_C DW STUB_EXPORT yact_SHCreateThread(DW *R)		
{	
  DWORD f1=CbCreateNativeStdcallCallback(p1,1);  
  DWORD f2=CbCreateNativeStdcallCallback(p4,1);  
  DW r=SHCreateThread((LPTHREAD_START_ROUTINE)f1,(void*)p2,p3,(LPTHREAD_START_ROUTINE)f2);	
  LEAVE(1);		
  return r;		
}
