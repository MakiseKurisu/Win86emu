#include "dllyact_def.h"
#include <windows.h>
#include <x86emul.h>
#include <callbacks.h>
#include <wininet.h>
#include <util.h>

/*
void CALLBACK InternetStatusCallback(
  _In_  HINTERNET hInternet,
  _In_  DWORD_PTR dwContext,
  _In_  DWORD dwInternetStatus,
  _In_  LPVOID lpvStatusInformation,
  _In_  DWORD dwStatusInformationLength
)
*/

//DEFINE_FUNC2(InternetSetStatusCallback)
EXTERN_C DW STUB_EXPORT yact_InternetSetStatusCallback(DW *R)
{
	return (DW)InternetSetStatusCallbackA((HINTERNET)p1,(INTERNET_STATUS_CALLBACK)CbCreateNativeStdcallCallback(p2,5));
}

//DEFINE_FUNC2(InternetSetStatusCallbackA)
EXTERN_C DW STUB_EXPORT yact_InternetSetStatusCallbackA(DW *R)
{
	return (DW)InternetSetStatusCallbackA((HINTERNET)p1,(INTERNET_STATUS_CALLBACK)CbCreateNativeStdcallCallback(p2,5));
}

//DEFINE_FUNC2(InternetSetStatusCallbackW)
EXTERN_C DW STUB_EXPORT yact_InternetSetStatusCallbackW(DW *R)
{
	return (DW)InternetSetStatusCallbackW((HINTERNET)p1,(INTERNET_STATUS_CALLBACK)CbCreateNativeStdcallCallback(p2,5));
}
