#include "dllyact_def.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <Winternl.h>

//DEFINE_FUNC1(NetpNtStatusToApiStatus)
EXTERN_C DW STUB_EXPORT yact_NetpNtStatusToApiStatus(DW *R)
{
	return RtlNtStatusToDosError(p1);
}
