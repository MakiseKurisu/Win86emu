
// LauncherDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Launcher.h"
#include "LauncherDlg.h"
#include "afxdialogex.h"
#include <util.h>
#include <imagehlp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static bool HideWindow = false;

// CLauncherDlg dialog
CLauncherDlg::CLauncherDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CLauncherDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLauncherDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EXEFILE, ExeFile);
}

BEGIN_MESSAGE_MAP(CLauncherDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDBROWSE, &CLauncherDlg::OnBnClickedBrowse)
    ON_BN_CLICKED(IDRUN, &CLauncherDlg::OnBnClickedRun)
    ON_WM_SYSCOMMAND()
    ON_COMMAND(ID_TRAY_RUN, &CLauncherDlg::OnTrayRun)
    ON_COMMAND(ID_TRAY_EXIT, &CLauncherDlg::OnTrayExit)
    ON_BN_CLICKED(IDCANCEL, &CLauncherDlg::OnBnClickedCancel)
    ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()

/*
void InitAutoHook()
{
char Path[MAX_PATH]="c:\\x86";
DWORD Ret=GetModuleFileNameA(0,Path,MAX_PATH);
char *Pp=strrchr(Path,'\\');
if(Pp)
*Pp=0;
strcat(Path,"\\cpihook.dll");
HMODULE hm=LoadLibraryA(Path);
if(hm)
{
FARPROC p=GetProcAddress(hm,"CpiInstallGlobalHook");
if(p)
p();
}
}
*/

// CLauncherDlg message handlers

BOOL CLauncherDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // TODO: Add extra initialization here
    char Buff[MAX_PATH] = "";
    CfgGetString("LauncherExe", Buff, MAX_PATH);
    ExeFile.SetWindowText(CStringW(Buff));
    for (int i = 0; i < 15; i++)
    {
        char Reg[64];
        sprintf(Reg, "LauncherExe%d", i);
        Buff[0] = 0;
        CfgGetString(Reg, Buff, MAX_PATH);
        CString Str(Buff);
        if (Str.GetLength() && ExeFile.FindStringExact(-1, Str) == CB_ERR)
            ExeFile.InsertString(0, Str);
    }

    CRect r;
    GetWindowRect(&r);
    int W = r.right - r.left;
    int H = r.bottom - r.top;
    r.left = CfgGetInt("LauncherWndX", r.left);
    r.top = CfgGetInt("LauncherWndY", r.top);
    r.right = r.left + W;
    r.bottom = r.top + H;
    MoveWindow(&r);

    //	if(CfgGetInt("LauncherGlobalHook"))
    //		InitAutoHook();

    if (__argc > 1)
        PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);
    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLauncherDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLauncherDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


void CLauncherDlg::OnBnClickedBrowse()
{
    CStringW Str;
    ExeFile.GetWindowTextW(Str);
    if (Str[0] == '\"')
    {
        int P = Str.Find('\"', 1);
        if (P<1)
            P = Str.GetLength();

        Str = Str.Mid(1, P - 1);
    }
    else
    {
        int P = Str.Find(' ', 1);
        if (P>0)
            Str = Str.Left(P);
    }
    CFileDialog Ofd(TRUE, L".exe", Str, OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, L"Executable Files|*.exe||", this);
    if (Ofd.DoModal() != IDOK)
        return;
    Str = Ofd.GetPathName();
    if (Str.Find(' ') >= 0)
        Str = '\"' + Str + '\"';
    ExeFile.SetWindowTextW(Str);
}

bool IsConsole(LPCWSTR Exe)
{
    HANDLE H = CreateFileW(Exe, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    HANDLE HM = CreateFileMapping(H, 0, PAGE_READONLY, 0, 0, 0);

    bool IsConsole = false;

    __try
    {
        if (HM == 0)
            return false;
        void *Base = MapViewOfFile(HM, FILE_MAP_READ, 0, 0, 0);
        if (Base == 0)
        {
            DWORD Tmp = GetLastError();
            CloseHandle(HM);
            CloseHandle(H);
            SetLastError(Tmp);
            return false;
        }

        IMAGE_NT_HEADERS *NT = ImageNtHeader(Base);

        IsConsole = (NT->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
        UnmapViewOfFile(Base);
        CloseHandle(HM);
        CloseHandle(H);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return IsConsole;
}

void CLauncherDlg::OnBnClickedRun()
{
    CRect r;
    GetWindowRect(&r);
    CString Str;
    ExeFile.GetWindowTextW(Str);
    if (ExeFile.FindStringExact(-1, Str) == CB_ERR)
        ExeFile.AddString(Str);
    CfgSetString("LauncherExe", (LPCSTR) CStringA(Str));
    for (int i = 0; i < 15 && i < ExeFile.GetCount(); i++)
    {
        char Reg[64];
        CString Str;
        sprintf(Reg, "LauncherExe%d", i);
        ExeFile.GetLBText(i, Str);
        CfgSetString(Reg, (LPCSTR) CStringA(Str));
    }
    CfgSetInt("LauncherWndX", r.left);
    CfgSetInt("LauncherWndY", r.top);
    SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);

    DeleteFileA(LogGetName());

    int P;
    CString Tmp = L"";
    while (Str[0] == ' ')
        Str = Str.Right(Str.GetLength() - 1);

    if (Str[0] == '\"')
    {
        P = Str.Find('\"', 1);
        if (P > 0)
            P++;
    }
    else
        P = Str.Find(' '); // params
    if (P >= 0)
    {
        Tmp = Str.Right(Str.GetLength() - P - 1);
        Str = Str.Left(P);
    }
    while (Tmp[0] == ' ')
        Tmp = Tmp.Right(Tmp.GetLength() - 1);

    CString Cwd = Str;
    P = Cwd.ReverseFind('\\');
    if (P >= 0)
        Cwd = Cwd.Left(P);
    if (Cwd[0] == '\"')
        Cwd = Cwd.Right(Cwd.GetLength() - 1);
    SetCurrentDirectory(Cwd);

    SetEnvironmentVariable(L"EMU_PROGRAM", Str);
    char Path[MAX_PATH] = "c:\\x86";
    char ProgParam[MAX_PATH];
    DWORD Ret = GetModuleFileNameA(0, Path, MAX_PATH);
    char *Pp = strrchr(Path, '\\');
    if (Pp)
        *Pp = 0;
    if (IsConsole(Str))
        sprintf(ProgParam, "%s\\peloaderc.exe %s", Path, (const char*) CStringA(Tmp));
    else
        sprintf(ProgParam, "%s\\peloader.exe %s", Path, (const char*) CStringA(Tmp));
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    int Count = 0;
Loop:
    Count++;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessA(0, ProgParam, 0, 0, FALSE, 0, 0, 0, &si, &pi) == 0)
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
        MessageBoxW(MessageBuffer, L"Error launching emulation engine", MB_ICONSTOP);
        return;
    }
    Ret = WaitForSingleObject(pi.hProcess, 3000);
    if (Ret == WAIT_OBJECT_0)
    {
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
                    MessageBoxW(L"Can't reserve memory (file is too big or invalid format)", L"Error", MB_ICONSTOP);
                    break;
                case 1:
                    MessageBoxW(L"Can't open emulated EXE file", L"Error", MB_ICONSTOP);
                    break;
                case 2:
                    MessageBoxW(L"Bad file format (not an x86 PE)", L"Error", MB_ICONSTOP);
                    break;
                case 3:
                    MessageBoxW(L"Can't load peldr.nt.dll", L"Error", MB_ICONSTOP);
                    break;
                case 4:
                    ShellExecuteA(0, "open", LogGetName(), 0, 0, SW_SHOWNORMAL);
                    break;
                default:
                    MessageBoxW(L"Unknown error running emulated program", L"Error", MB_ICONSTOP);
            }
        }
        else
        {
            char Buff[1000];
            sprintf(Buff, "Program terminated with exit code %d", Ex);
            MessageBoxA(m_hWnd, Buff, "Error", MB_ICONSTOP);
        }
        return;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}


void CLauncherDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    switch (nID & 0xFFF0)
    {
        case SC_MINIMIZE:
            if (lParam)
                ShowWindow(SW_HIDE);
            else
                SetForegroundWindow();
            return;
        default:
            CDialogEx::OnSysCommand(nID, lParam);
            return;
    }
}

void CLauncherDlg::OnTrayRun()
{
    HideWindow = false;
    ShowWindow(SW_RESTORE);
    SetForegroundWindow();
}

void CLauncherDlg::OnTrayExit()
{
    PostQuitMessage(0);
}

void CLauncherDlg::OnBnClickedCancel()
{
    PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);
}

void CLauncherDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if (HideWindow)
        lpwndpos->flags &= ~SWP_SHOWWINDOW;
    CDialogEx::OnWindowPosChanging(lpwndpos);
}
