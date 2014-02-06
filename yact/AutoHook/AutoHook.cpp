// AutoHook.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <tlhelp32.h>
#include <stdio.h>
#include "AutoHook.h"
#include <util.h>

#define MAX_LOADSTRING 100

// I prefere ANSI versions of those funcs
#undef Module32First 
#undef Module32Next 
#undef MODULEENTRY32 
#undef PMODULEENTRY32 
#undef LPMODULEENTRY32 

bool HookProcess(DWORD PID, bool silent)
{
    char Path[MAX_PATH] = "c:\\x86";
    DWORD Ret = GetModuleFileNameA(0, Path, MAX_PATH);
    char *Pp = strrchr(Path, '\\');
    if (Pp)
        *Pp = 0;
    strcat_s(Path, "\\cpihook.dll");

    FARPROC proc = GetProcAddress(LoadLibraryA("kernel32.dll"), "LoadLibraryA");
    DWORD Delta = (DWORD) proc - (0xFFFFFFF0 & (DWORD) LoadLibraryA("kernel32.dll"));
    if (proc == 0 || (int) Delta < 0)
    {
        if (!silent)
            LogErr("Can'd find LoadLibraryA address in kernel32.dll");
        return false;
    }

    HANDLE HP = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, PID);
    DWORD dw = GetLastError();
    if (HP == 0)
    {
        if (!silent)
            LogErr("OpenProcess failed, error %0d", dw);
        return false;
    }

    HANDLE HS = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
    dw = GetLastError();
    if (HS == INVALID_HANDLE_VALUE)
    {
        if (!silent)
            LogErr("CreateToolhelp32Snapshot failed, error %0d", dw);
        CloseHandle(HP);
        return false;
    }

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);
    BOOL Ok = Module32First(HS, &me);
    while (Ok)
    {
        if (_strcmpi(me.szModule, "kernel32.dll") == 0)
            proc = (FARPROC) (me.modBaseAddr + Delta);

        if (_strcmpi(me.szModule, "cpihook.dll") == 0)
        {
            if (!silent)
                LogWarn("Hook already installed in the process");
            CloseHandle(HP);
            CloseHandle(HS);
            return true;			// already loaded
        }

        Ok = Module32Next(HS, &me);
    }

    void *param = VirtualAllocEx(HP, 0, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);	// This memory is leaked
    dw = GetLastError();
    if (param == 0)
    {
        if (!silent)
            LogErr("VirtualAllocEx failed, error %0d", dw);
        CloseHandle(HP);
        CloseHandle(HS);
        return false;
    }
    if (WriteProcessMemory(HP, param, Path, MAX_PATH, &Ret) == 0)
    {
        dw = GetLastError();
        if (!silent)
            LogErr("WriteProcessMemory failed, error %0d", dw);
        VirtualFreeEx(HP, param, 0, MEM_RELEASE);
        CloseHandle(HP);
        CloseHandle(HS);
        return false;
    }

    HANDLE HT = CreateRemoteThread(HP, 0, 0, (LPTHREAD_START_ROUTINE) proc, param, 0, 0);

    if (HT)
    {
        if (WaitForSingleObject(HT, 500) != WAIT_TIMEOUT)
            VirtualFreeEx(HP, param, 0, MEM_RELEASE);

        CloseHandle(HT);

        // check that our DLL is loaded
        me.dwSize = sizeof(MODULEENTRY32);
        BOOL Ok = Module32First(HS, &me);
        while (Ok)
        {
            if (_strcmpi(me.szModule, "cpihook.dll") == 0)
            {
                CloseHandle(HP);
                CloseHandle(HS);
                return true;			// loaded
            }
            Ok = Module32Next(HS, &me);
        }
        if (!silent)
            LogErr("cpihook.dll not loaded");
    }
    else
    {
        dw = GetLastError();
        if (!silent)
            LogErr("CreateRemoteThread failed, error %0d", dw);
    }
    CloseHandle(HP);
    CloseHandle(HS);

    return false;
}

bool UpdateProcesses(bool silent = false)
{
    bool Ret = false;
    HANDLE HS = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (HS == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    BOOL Ok = Process32First(HS, &pe);
    while (Ok)
    {
        //		if(_wcsicmp(pe.szExeFile,L"iexplore.exe")==0)		// iexplorer createthread shim provides too small stack
        //			continue;
        if (!silent)
            LogPrint("Hooking %S", pe.szExeFile);
        bool b = HookProcess(pe.th32ProcessID, silent);
        if (!silent)
        {
            if (b)
                LogPrint("Hooked successfully");
            else
                LogPrint("Hook failed");
        }
        Ret |= b;
        Ok = Process32Next(HS, &pe);
    }
    CloseHandle(HS);
    return Ret;
}

//
//  SetPrivilege enables/disables process token privilege.
//
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    LUID luid;
    BOOL bRet = FALSE;

    if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
    {
        TOKEN_PRIVILEGES tp;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;
        //
        //  Enable the privilege or disable all privileges.
        //
        if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL))
        {
            //
            //  Check to see if you have proper access.
            //  You may get "ERROR_NOT_ALL_ASSIGNED".
            //
            bRet = (GetLastError() == ERROR_SUCCESS);
        }
    }
    return bRet;
}

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING] = L"X86AutoHook";

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    HANDLE hProcess = GetCurrentProcess();
    HANDLE hToken;

    if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
        CloseHandle(hToken);
    }

    UpdateProcesses(
#ifdef _DEBUG
        false
#else
        true
#endif
        );

    HWND hw = FindWindowW(szWindowClass, 0);
    if (hw)
    {
        PostMessage(hw, WM_COMMAND, 0, 0);
        return TRUE;
    }

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //	LoadString(hInstance, IDC_AUTOHOOK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    if (!UpdateProcesses(
#ifdef _DEBUG
        false
#else
        true
#endif
        ))
    {
        MessageBoxA(0, "Unable to install CreateProcess hook DLL", "Error", MB_ICONSTOP);
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AUTOHOOK));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AUTOHOOK));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_AUTOHOOK);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    //   UpdateWindow(hWnd);
    SetTimer(hWnd, 1, 5000, 0);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
        case WM_TIMER:
            UpdateProcesses(true);
            break;
        case WM_COMMAND:
            if (wParam == 0 && lParam == 0)
            {
                UpdateProcesses(true);
                return 0;
            }
            wmId = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR) TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR) TRUE;
            }
            break;
    }
    return (INT_PTR) FALSE;
}
