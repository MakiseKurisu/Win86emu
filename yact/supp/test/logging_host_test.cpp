// logging_host_test.cpp : host-side regression tests for the real
// supp.nt.dll logger implementation (yact/supp/logging.cpp).
//
// The logger is compiled unmodified; the Windows APIs it uses come from
// test/shim. Run through test_logging_host.py, which starts one process per
// mode because the logger caches the level and the file path for the
// process lifetime.

#include "shim/windows.h"
#include <util.h>
#include "../logging.h"

#include <string>
#include <vector>
#include <set>
#include <thread>
#include <fstream>
#include <sstream>
#include <regex>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>

// ---------------------------------------------------------------------------
// Configuration injected through the real CfgGetInt/CfgGetString calls.

static int g_CfgLevel = 2;
static int g_CfgMaxKB = 8192;
static std::string g_CfgLogFile;

extern "C" int CfgGetInt(const char *Setting, int Default)
{
    if (strcmp(Setting, "LogLevel") == 0)
        return g_CfgLevel;
    if (strcmp(Setting, "LogMaxSizeKB") == 0)
        return g_CfgMaxKB;
    return Default;
}

extern "C" bool CfgGetString(const char *Setting, char *Out, int OutLen)
{
    if (strcmp(Setting, "LogFile") != 0 || g_CfgLogFile.empty())
        return false;
    if ((int) g_CfgLogFile.size() + 1 > OutLen)
        return false;
    strcpy(Out, g_CfgLogFile.c_str());
    return true;
}

extern "C" const char *EmGetVersionString(void)
{
    return "host-test";
}

// ---------------------------------------------------------------------------
// Tiny check framework (the runner counts PASS/FAIL lines).

static int g_Pass = 0;
static int g_Fail = 0;

static void Check(bool Ok, const std::string &Name, const std::string &Detail = "")
{
    if (Ok)
    {
        g_Pass++;
        printf("PASS %s\n", Name.c_str());
    }
    else
    {
        g_Fail++;
        printf("FAIL %s%s%s\n", Name.c_str(), Detail.empty() ? "" : ": ", Detail.c_str());
    }
}

// ---------------------------------------------------------------------------
// Helpers

static std::string TempDir(void)
{
    const char *Base = getenv("TMPDIR");
    if (Base == 0 || *Base == 0)
        Base = "/tmp";
    std::string Tmpl = std::string(Base) + "/win86emu_logtest_XXXXXX";
    std::vector<char> Buf(Tmpl.begin(), Tmpl.end());
    Buf.push_back(0);
    char *Dir = mkdtemp(Buf.data());
    return Dir ? std::string(Dir) : std::string();
}

static std::string ReadFile(const std::string &Path)
{
    std::ifstream In(Path.c_str(), std::ios::binary);
    std::stringstream Ss;
    Ss << In.rdbuf();
    return Ss.str();
}

static std::vector<std::string> ReadLines(const std::string &Path)
{
    std::vector<std::string> Lines;
    std::ifstream In(Path.c_str(), std::ios::binary);
    std::string Line;
    while (std::getline(In, Line))
    {
        if (!Line.empty() && Line.back() == '\r')
            Line.pop_back();
        Lines.push_back(Line);
    }
    return Lines;
}

static bool Contains(const std::string &Haystack, const std::string &Needle)
{
    return Haystack.find(Needle) != std::string::npos;
}

static std::string Lower(std::string S)
{
    for (size_t i = 0; i < S.size(); i++)
        S[i] = (char) tolower((unsigned char) S[i]);
    return S;
}

// Full line format: <date> <time> [LEVEL][pid=N tid=N][module] message
static bool MatchLineFormat(const std::string &Line, std::string *Level = 0,
                            std::string *Module = 0, std::string *Message = 0)
{
    static const std::regex Re(
        "^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\\.[0-9]{3} "
        "\\[(ERROR|WARN|INFO|DEBUG|TRACE)\\]\\[pid=[0-9]+ tid=[0-9]+\\]\\[([^\\]]+)\\] (.*)$");
    std::smatch M;
    if (!std::regex_match(Line, M, Re))
        return false;
    if (Level)
        *Level = M[1];
    if (Module)
        *Module = M[2];
    if (Message)
        *Message = M[3];
    return true;
}

static void Setup(const std::string &Dir, const std::string &File, int Level, int MaxKB = 8192)
{
    g_CfgLevel = Level;
    g_CfgMaxKB = MaxKB;
    g_CfgLogFile = Dir + "/" + File;
    ShimSetDefaultModule("logging_host_test");
}

// ---------------------------------------------------------------------------
// Modes

static void ModeBasic(const std::string &Dir)
{
    Setup(Dir, "win86emu.log", 5);
    ShimDebugOutputReset();

    InitDebugLog();
    LogErr("basic-token-error");
    LogWarn("basic-token-warn");
    LogInfo("basic-token-info");
    LogPrint("basic-token-debug");
    LogTrace("basic-token-trace");

    std::vector<std::string> Lines = ReadLines(g_CfgLogFile);
    Check(!Lines.empty(), "basic/file-created");

    bool Header = false;
    for (size_t i = 0; i < Lines.size(); i++)
        if (Lines[i] == "=== Win86emun debug log ===")
            Header = true;
    Check(Header, "basic/header");

    const char *Levels[] = {"ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    const char *Tokens[] = {"basic-token-error", "basic-token-warn", "basic-token-info",
                            "basic-token-debug", "basic-token-trace"};
    for (int i = 0; i < 5; i++)
    {
        bool Found = false;
        std::string Where;
        for (size_t j = 0; j < Lines.size(); j++)
        {
            std::string Level, Module, Message;
            if (MatchLineFormat(Lines[j], &Level, &Module, &Message) &&
                Level == Levels[i] && Contains(Message, Tokens[i]))
            {
                Found = true;
                Where = Module;
                break;
            }
        }
        Check(Found, std::string("basic/line-") + Lower(Levels[i]));
        Check(Found && Where == "logging_host_test", std::string("basic/module-") + Lower(Levels[i]),
              "module=" + Where);
    }

    char PidField[64];
    snprintf(PidField, sizeof(PidField), "[pid=%lu tid=", (unsigned long) GetCurrentProcessId());
    bool PidOk = false;
    for (size_t i = 0; i < Lines.size(); i++)
        if (Contains(Lines[i], PidField))
            PidOk = true;
    Check(PidOk, "basic/pid-tid-field");

    Check(Contains(ReadFile(g_CfgLogFile), "logger initialized"), "basic/init-line");
}

static void ModeLevels(const std::string &Dir, int Level)
{
    Setup(Dir, "levels.log", Level);

    InitDebugLog();
    LogErr("lvl-error");
    LogWarn("lvl-warn");
    LogInfo("lvl-info");
    LogPrint("lvl-debug");
    LogTrace("lvl-trace");

    std::string Text = ReadFile(g_CfgLogFile);
    const char *Tokens[] = {"lvl-error", "lvl-warn", "lvl-info", "lvl-debug", "lvl-trace"};
    bool Ok = true;
    std::string Got;
    for (int i = 0; i < 5; i++)
    {
        bool Expect = (Level != 0) && (Level >= i + 1);
        bool Present = Contains(Text, Tokens[i]);
        if (Expect != Present)
        {
            Ok = false;
            Got += std::string(Tokens[i]) + (Present ? "=present " : "=absent ");
        }
    }
    char Name[64];
    snprintf(Name, sizeof(Name), "levels/L%d-output-set", Level);
    Check(Ok, Name, Got);

    bool EnabledOk = true;
    for (int L = 1; L <= 5; L++)
    {
        bool Expect = (Level != 0) && (Level >= L);
        if (LogIsEnabled(L) != Expect)
            EnabledOk = false;
    }
    snprintf(Name, sizeof(Name), "levels/L%d-isenabled", Level);
    Check(EnabledOk, Name);

    if (Level == 0)
        Check(!Contains(Text, "lvl-"), "levels/L0-no-messages");
}

static void ModeThreads(const std::string &Dir)
{
    Setup(Dir, "threads.log", 2);        // WARN: every line is committed
    InitDebugLog();

    const int Threads = 4;
    const int PerThread = 100;
    std::vector<std::thread> Workers;
    for (int T = 0; T < Threads; T++)
    {
        Workers.push_back(std::thread([T]()
        {
            for (int I = 0; I < PerThread; I++)
                LogWarn("thread-token-%d-%d", T, I);
        }));
    }
    for (size_t i = 0; i < Workers.size(); i++)
        Workers[i].join();

    std::vector<std::string> Lines = ReadLines(g_CfgLogFile);
    int Found = 0;
    int Bad = 0;
    std::set<std::string> Tokens;
    for (size_t i = 0; i < Lines.size(); i++)
    {
        if (!Contains(Lines[i], "thread-token-"))
            continue;
        Found++;
        std::string Message;
        if (!MatchLineFormat(Lines[i], 0, 0, &Message))
            Bad++;
        else
            Tokens.insert(Message);
    }
    Check(Found == Threads * PerThread, "threads/all-lines-present",
          "found=" + std::to_string(Found));
    Check(Bad == 0, "threads/no-interleaved-or-truncated-lines",
          "bad=" + std::to_string(Bad));
    Check((int) Tokens.size() == Threads * PerThread, "threads/all-tokens-unique",
          "unique=" + std::to_string(Tokens.size()));
}

static void ModeFallback(const std::string &Dir)
{
    // Parent directory does not exist yet: the logger must report the failure
    // and keep working through the debug output.
    Setup(Dir, "missing_dir/sub/win86emu.log", 3);
    ShimDebugOutputReset();

    InitDebugLog();
    LogInfo("fallback-token-1");

    Check(!LogGetFileOpened(), "fallback/file-not-open");
    Check(LogGetFileError() != 0, "fallback/error-recorded");
    std::string Out = ShimDebugOutputGet();
    Check(Contains(Out, "cannot open log file"), "fallback/reason-in-debug-output");
    Check(Contains(Out, "fallback-token-1"), "fallback/message-in-debug-output");

    // Create the directory and retry: InitDebugLog must open the file now.
    std::string Sub = Dir + "/missing_dir/sub";
    mkdir((Dir + "/missing_dir").c_str(), 0700);
    mkdir(Sub.c_str(), 0700);
    InitDebugLog();
    Check(LogGetFileOpened(), "fallback/retry-opens-file");
    LogInfo("fallback-token-2");
    Check(Contains(ReadFile(g_CfgLogFile), "fallback-token-2"), "fallback/message-after-retry");
}

static void ModeRotation(const std::string &Dir)
{
    Setup(Dir, "rotate.log", 3, 1);      // 1 KB: rotations happen with small lines
    InitDebugLog();

    for (int I = 0; I < 12; I++)
        LogInfo("rotate-token-%02d %s", I, std::string(60, 'x').c_str());

    std::string Main = ReadFile(g_CfgLogFile);
    std::string Backup = ReadFile(g_CfgLogFile + ".1");
    Check(!Backup.empty(), "rotation/backup-created");
    Check(Contains(Main, "rotate-token-11"), "rotation/last-in-main");
    Check(Contains(Backup, "rotate-token-"), "rotation/earlier-in-backup");

    std::set<std::string> MainTokens;
    std::set<std::string> BackupTokens;
    int Bad = 0;
    std::vector<std::string> MainLines = ReadLines(g_CfgLogFile);
    for (size_t i = 0; i < MainLines.size(); i++)
    {
        std::string Message;
        if (MatchLineFormat(MainLines[i], 0, 0, &Message) && Contains(Message, "rotate-token-"))
            MainTokens.insert(Message);
        else if (Contains(MainLines[i], "rotate-token-"))
            Bad++;
    }
    std::vector<std::string> BackupLines = ReadLines(g_CfgLogFile + ".1");
    for (size_t i = 0; i < BackupLines.size(); i++)
    {
        std::string Message;
        if (MatchLineFormat(BackupLines[i], 0, 0, &Message) && Contains(Message, "rotate-token-"))
            BackupTokens.insert(Message);
        else if (Contains(BackupLines[i], "rotate-token-"))
            Bad++;
    }
    Check(Bad == 0, "rotation/lines-intact", "bad=" + std::to_string(Bad));

    bool Disjoint = true;
    for (std::set<std::string>::iterator It = MainTokens.begin(); It != MainTokens.end(); ++It)
        if (BackupTokens.count(*It))
            Disjoint = false;
    Check(Disjoint, "rotation/tokens-not-duplicated");
    Check(Main.size() < 3 * 1024, "rotation/main-size-capped",
          "size=" + std::to_string(Main.size()));
}

static void ModePreinit(const std::string &Dir)
{
    Setup(Dir, "preinit.log", 3);

    // Log before any explicit initialization: the logger must initialize
    // itself and record the message.
    LogErr("preinit-token");
    InitDebugLog();

    std::string Text = ReadFile(g_CfgLogFile);
    Check(Contains(Text, "preinit-token"), "preinit/error-before-init-logged");
    Check(Contains(Text, "logger initialized"), "preinit/init-result-logged");
    Check(Contains(Text, "=== Win86emun debug log ==="), "preinit/header");
    Check(LogGetFileOpened(), "preinit/file-open");
    Check(Contains(LogGetName(), "preinit.log"), "preinit/getname-path");
}

static void ModeDefaultPath(const std::string &Dir)
{
    std::string Temp = Dir + "/";
    setenv("TEMP", Temp.c_str(), 1);
    g_CfgLevel = 3;
    g_CfgMaxKB = 8192;
    g_CfgLogFile.clear();
    ShimSetDefaultModule("logging_host_test");

    InitDebugLog();
    std::string Path = LogGetName();
    Check(Path == Dir + "/win86emu.log", "defaultpath/temp-join", "path=" + Path);
    LogInfo("defaultpath-token");
    Check(Contains(ReadFile(Path), "defaultpath-token"), "defaultpath/written");
}

static void ModeException(const std::string &Dir)
{
    Setup(Dir, "exception.log", 1);
    InitDebugLog();

    LogInstallUnhandledExceptionFilter();
    SHIM_EXCEPTION_FILTER Filter = ShimGetExceptionFilter();
    Check(Filter != 0, "exception/filter-installed");
    if (Filter == 0)
        return;

    ShimRegisterModule(0x400000, 0x401000, "faulting_module.dll");

    EXCEPTION_RECORD Record;
    memset(&Record, 0, sizeof(Record));
    Record.ExceptionCode = 0xC0000005;
    Record.ExceptionAddress = (PVOID) (uintptr_t) 0x400123;
    Record.NumberParameters = 2;
    Record.ExceptionInformation[0] = 0;              // read
    Record.ExceptionInformation[1] = 0xDEADBEEF;

    EXCEPTION_POINTERS Info;
    Info.ExceptionRecord = &Record;
    Info.ContextRecord = 0;

    LONG Result = Filter(&Info);
    Check(Result == EXCEPTION_CONTINUE_SEARCH, "exception/filter-continues-search");

    std::string Text = Lower(ReadFile(g_CfgLogFile));
    Check(Contains(Text, "unhandled exception 0xc0000005"), "exception/code-logged");
    Check(Contains(Text, "faulting_module.dll+0x123"), "exception/module-offset-logged");
    Check(Contains(Text, "access=read") && Contains(Text, "deadbeef"),
          "exception/access-details-logged");
}

static void ModeCompat(const std::string &Dir)
{
    Setup(Dir, "compat.log", 5);
    ShimDebugOutputReset();
    InitDebugLog();

    WriteDebugLog("compat-write-token");
    LogDllLoadFailure("missing.dll", 126);
    LogEmulationStart(0x401000, 2);
    LogEmulationEnd(0x42);

    std::string Text = ReadFile(g_CfgLogFile);
    Check(Contains(Text, "[DEBUG]") && Contains(Text, "compat-write-token"),
          "compat/writedebuglog-is-debug");
    Check(Contains(Text, "failed to load DLL 'missing.dll' (GetLastError=126)"),
          "compat/dllloadfailure");
    Check(Contains(Text, "emulation start: addr=0x00401000 NParams=2"),
          "compat/emulation-start");
    Check(Contains(Text, "emulation end: ret=0x00000042 duration="),
          "compat/emulation-end-duration");

    ShutdownDebugLog();
    Check(!LogGetFileOpened(), "compat/shutdown-closes-file");
    Check(Contains(ReadFile(g_CfgLogFile), "logger shutdown"), "compat/shutdown-line");

    ShimDebugOutputReset();
    LogErr("after-shutdown-token");
    Check(Contains(ShimDebugOutputGet(), "after-shutdown-token"),
          "compat/post-shutdown-goes-to-debug-output");
    Check(!Contains(ReadFile(g_CfgLogFile), "after-shutdown-token"),
          "compat/post-shutdown-not-reopened");
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <mode> [arg]\n", argv[0]);
        return 2;
    }
    std::string Mode = argv[1];
    std::string Dir = TempDir();
    if (Dir.empty())
    {
        fprintf(stderr, "cannot create temp dir\n");
        return 2;
    }

    if (Mode == "basic")
        ModeBasic(Dir);
    else if (Mode == "levels")
    {
        if (argc < 3)
            return 2;
        ModeLevels(Dir, atoi(argv[2]));
    }
    else if (Mode == "threads")
        ModeThreads(Dir);
    else if (Mode == "fallback")
        ModeFallback(Dir);
    else if (Mode == "rotation")
        ModeRotation(Dir);
    else if (Mode == "preinit")
        ModePreinit(Dir);
    else if (Mode == "defaultpath")
        ModeDefaultPath(Dir);
    else if (Mode == "exception")
        ModeException(Dir);
    else if (Mode == "compat")
        ModeCompat(Dir);
    else
    {
        fprintf(stderr, "unknown mode %s\n", Mode.c_str());
        return 2;
    }

    printf("SUMMARY %s pass=%d fail=%d\n", Mode.c_str(), g_Pass, g_Fail);
    return g_Fail == 0 ? 0 : 1;
}
