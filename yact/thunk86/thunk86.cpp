// thunk86.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "thunk86.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shellapi.h>

int _tmain(int argc, _TCHAR* argv[])
{
    //	ShowCursor(TRUE);
    //	HCURSOR OldCur=SetCursor(LoadCursor(NULL, IDC_WAIT));

    char Path[MAX_PATH];

    DWORD Ret = GetModuleFileNameA(0, Path, MAX_PATH);
    char *Pp = strrchr(Path, '\\');
    if (Pp)
        *Pp = 0;

    if (argc == 0)
        strcat_s(Path, "\\peloader.exe");
    else
        strcat_s(Path, "\\peloaderc.exe");

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    int Count = 0;

    char FakeCmdLine1[32768];
    char* FakeCmdLine = new char[32768];
    strcpy_s(FakeCmdLine, 32768, GetCommandLineA());

    char *Param;
    if (FakeCmdLine[0] == '\"')
    {
        FakeCmdLine++;
        Param = strchr(FakeCmdLine, '\"');
        if (Param)
        {
            *Param = 0;
            Param++;
        }
    }
    else
        Param = strchr(FakeCmdLine, ' ');

    while (Param && *Param == ' ')
    {
        *Param = 0;
        Param++;
    }

    char *Exe = strrchr(FakeCmdLine, '\\');
    if (Exe)
        Exe++;
    else
        Exe = FakeCmdLine;

    if (Param)
        sprintf_s(FakeCmdLine1, "%s %s", Exe, Param);
    else
        sprintf_s(FakeCmdLine1, "%s", Exe);

Loop:
    Count++;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessA(Path, FakeCmdLine1, 0, 0, TRUE, 0, 0, 0, &si, &pi) == 0)
    {
        LPTSTR MessageBuffer;
        DWORD dwBufferLength;
        DWORD Err = GetLastError();
        dwBufferLength = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            Err,
            0x409,
            (LPTSTR) &MessageBuffer,
            0,
            NULL
            );
        //		SetCursor(OldCur);
        //		ShowCursor(FALSE);
        MessageBox(0, MessageBuffer, "Error launching emulation engine", MB_ICONSTOP);
        return -1;
    }
    Ret = 0;
    if (argc == 0)
    {
        Ret = WaitForInputIdle(pi.hProcess, INFINITE);
        if (Ret)
        {
            MessageBoxW(0, L"Error waiting for program to initialize, log may provide more info", L"Error", MB_ICONSTOP);
            goto Done;
        }
        MSG msg;
        PeekMessage(&msg, 0, 0, 0, 0);	// release the caller that waits on us for be idle
    }

    Ret = WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD Ex = 0;
    GetExitCodeProcess(pi.hProcess, &Ex);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (Ex == 0xBAD00000 && Count < 20)
        goto Loop;
    if ((Ex & 0xFFFFFFF0) == 0xBAD00000)
    {
        switch (Ex & 0xf)
        {
            case 0:
                MessageBoxW(0, L"Can't reserve memory (file is too big or corrupted)", L"Error", MB_ICONSTOP);
                break;
            case 1:
                MessageBoxW(0, L"Can't open emulated EXE file", L"Error", MB_ICONSTOP);
                break;
            case 2:
                MessageBoxW(0, L"Bad file format (not an x86 PE)", L"Error", MB_ICONSTOP);
                break;
            case 3:
                MessageBoxW(0, L"Can't load peldr.nt.dll", L"Error", MB_ICONSTOP);
                break;
            case 4:
                MessageBoxW(0, L"Error loading program, see log for details", L"Error", MB_ICONSTOP);
                //			ShellExecuteA(0,"open",LogGetName(),0,0,SW_SHOWNORMAL);
                break;
            default:
                MessageBoxW(0, L"Unknown error running emulated program", L"Error", MB_ICONSTOP);
        }
    }
Done:
    //	SetCursor(OldCur);
    //	ShowCursor(FALSE);
    return Ret;
}


int CALLBACK WinMain(
    _In_  HINSTANCE hInstance,
    _In_  HINSTANCE hPrevInstance,
    _In_  LPSTR lpCmdLine,
    _In_  int nCmdShow
    )
{
    return _tmain(0, 0);
}
