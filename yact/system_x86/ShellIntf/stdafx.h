// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


// The Visual Studio wizard left these ATL includes behind, but nothing in
// this project uses ATL (no CString/CComPtr references). ATL ships only with
// Visual Studio, so drop them to allow building with clang/MinGW.
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
//#include <atlbase.h>
//#include <atlstr.h>

// TODO: reference additional headers your program requires here
