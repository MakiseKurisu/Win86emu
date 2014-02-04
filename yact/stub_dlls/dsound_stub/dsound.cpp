#include "dllstub_def.h"

static const char DLL_NAME[]="dsound.nt.dll";

DEFINE_FUNC_ORD3(DirectSoundCaptureCreate,6)
DEFINE_FUNC_ORD3(DirectSoundCaptureCreate8,12)
DEFINE_FUNC_ORD2(DirectSoundCaptureEnumerateA,7)
DEFINE_FUNC_ORD2(DirectSoundCaptureEnumerateW,8)
DEFINE_FUNC_ORD3(DirectSoundCreate,1)
DEFINE_FUNC_ORD3(DirectSoundCreate8,11)
DEFINE_FUNC_ORD2(DirectSoundEnumerateA,2)
DEFINE_FUNC_ORD2(DirectSoundEnumerateW,3)
DEFINE_FUNC_ORD10(DirectSoundFullDuplexCreate,10)
DEFINE_FUNC_ORD2(GetDeviceID,9)
