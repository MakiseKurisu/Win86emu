#include "dllstub_def.h"

static const char DLL_NAME[]="dinput.nt.dll";

DEFINE_FUNC_ORD4(DirectInputCreateA,1)
DEFINE_FUNC_ORD5(DirectInputCreateEx,2)
DEFINE_FUNC_ORD4(DirectInputCreateW,3)
DEFINE_FUNC_ORD0(DllCanUnloadNow,4)
DEFINE_FUNC_ORD3(DllGetClassObject,5)
DEFINE_FUNC_ORD0(DllRegisterServer,6)
DEFINE_FUNC_ORD0(DllUnregisterServer,7)

