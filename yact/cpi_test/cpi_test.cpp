// cpi_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
    HMODULE HM = LoadLibraryA("cpihook.dll");
    if (HM == 0)
        return GetLastError();
    FARPROC hook = GetProcAddress(HM, "CpiMakeHook");
    if (hook == 0)
        return GetLastError();

    hook();
    //	_texecl(L"..\\..\\Win32\\Debug\\regedit.exe",L"REGEDIT.EXE",0);
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    CreateProcessA(0, "U:\\DynamicTranslator\\ARM\\regedit.exe", 0, 0, FALSE, 0, 0, 0, &si, &pi);

    return 0;
}

