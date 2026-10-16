# Win86emun Debug Logging

Win86emun writes a single debug log shared by all of its components. The logger
is implemented in `supp.nt.dll` (`yact/supp/logging.cpp`), which every component
already imports (`x86_emu.dll`, `dosbox_emu.dll` + `dosbox_emu1..15.dll`,
`bochs_emu.dll`, `cbacks.nt.dll`, `peldr.nt.dll`, Launcher). No extra DLL and no
change to the DLL load order is needed.

## Where the log goes

Default: `%TEMP%\win86emu.log` (writable on Windows RT for the account that runs
the application).

The existing settings mechanism is used to change it, per user or per machine:

| Registry value | Location | Meaning |
|----------------|----------|---------|
| `LogFile` | `HKCU\Software\x86node\Settings` (fallback: `HKLM\...`) | Full path of the log file. `REG_SZ` or `REG_EXPAND_SZ` (`%TEMP%`-style variables are expanded) |
| `LogLevel` | same | 0..5, REG_DWORD, default 2 |
| `LogMaxSizeKB` | same | Size limit in KB, REG_DWORD, default 8192, 0 disables rotation |

On 64-bit Windows the front end runs as a 32-bit process, so the `HKLM` value is
read from the 32-bit registry view (`HKLM\SOFTWARE\Wow6432Node\x86node\Settings`);
`HKCU` is shared and is checked first. Windows RT (32-bit) has no redirection.
`settings.reg` and `settings64.reg` contain ready-to-import examples.

The Launcher deletes the log file before starting an application and its
"Open log file" action opens `LogGetName()`, so each run starts with a fresh
file.

## Line format

```
2026-10-16 21:03:11.482 [INFO][pid=8123 tid=4412][dosbox_emu.dll] EmuExecute: engine=dosbox_emu index=0 addr=0x00401234 NParams=2 tid=4412
```

Fields: local timestamp with milliseconds, level, process id, thread id, the
module that logged the event (the DLL that called the logger, resolved from the
call site), and the message. Every line ends with `\n`. Messages longer than
about 1 KB are truncated and end with `...`.

The first lines of a file describe the run: logger file/level, version, process
id and the format itself.

## Levels

| LogLevel | Logged |
|----------|--------|
| 0 | nothing (logging disabled) |
| 1 | ERROR |
| 2 | WARN, ERROR (default) |
| 3 | INFO, WARN, ERROR |
| 4 | DEBUG, INFO, WARN, ERROR |
| 5 | TRACE, DEBUG, INFO, WARN, ERROR |

The settings (level, file path, size limit) are read once per process; restart
the application after changing them.

What the levels contain:

- **1-2 (default)**: startup/DLL/engine failures with `GetLastError`, invalid
  callbacks, exceptions caught around callbacks, unhandled exceptions.
- **3**: startup sequence (logger file and level, version, whether the process
  runs under Windows RT x86 emulation, engine initialization result, loaded
  engine DLLs), fallback to the Bochs core.
- **4 (DEBUG)**: one line per emulated call (entry, exit and duration), the
  engine chosen for each call, API hook binding, successful DLL loads. This is
  the level to use when an application stops: the last `emulation start` line
  without a matching `emulation end` line is the call that hung.
- **5 (TRACE)**: reserved for per-instruction output. The DOSBox engine already
  has an EIP dump behind `#ifdef _DEBUG` (`dosbox_emu.cpp`), which now goes
  through `LogTrace`; release builds of the engines do not contain
  per-instruction logging yet.

Use level 4 or 5 only while diagnosing: per-call and per-instruction logging
produces large files and slows emulation down.

## Size limit and rotation

When the file reaches `LogMaxSizeKB` (default 8 MB), it is renamed to
`<file>.1` and a new file is started. Only the newest previous file is kept;
older generations are replaced deliberately (the Launcher deletes the current
file on the next run, so `.1` is the only extra file that can accumulate).

If the rename fails - for example because another process keeps the file open -
the logger keeps appending to the current file (no messages are lost), reports
the failure to the debug output once, and retries after another `LogMaxSizeKB`
of data.

## When no log file appears

1. Check `LogLevel` in `HKCU\Software\x86node\Settings` (0 disables logging).
2. Check the effective path: it is `LogGetName()`; without a `LogFile` setting
   it is `%TEMP%\win86emu.log` of the account that runs the application.
3. Check the debug output (DebugView, a debugger, or `SHIM_ECHO_DEBUG=1` in the
   host tests). When the file cannot be used, every line is written there,
   including the reason and error code
   (`cannot open log file '...' errno=... GetLastError=...`).
   `LogGetFileError()` returns that code (0 = no error) and `LogGetFileOpened()`
   tells whether the file is open.
4. If the file exists but has no message lines, the current level filtered
   everything out - raise `LogLevel`.

## Crashes and exceptions

- Exceptions that the engines catch around callbacks are logged with their code
  and callback address (`Exception 0x........ in callback 0x........`).
- `x86_emu.dll` installs a process-wide unhandled exception filter from
  `EmuInitialize` (`LogInstallUnhandledExceptionFilter`). It records the
  exception code, the faulting address as `module+offset`, and for access
  violations whether it was a read or a write plus the accessed address, then
  flushes the file. The filter never suppresses the exception, so the default
  handling of the process is unchanged.
- The filter is a single process-wide setting, so a later installation replaces
  it. The existing emulator threads install `YactUnhandledExceptionFilter`
  (`kernel32_yact/kernel32_impl.cpp`) when they start; that filter logs through
  `LogWarn` and terminates the thread, exactly as before this logging work. The
  logger filter therefore covers crashes before an emulator thread starts (for
  example during DLL/engine loading) and threads that do not run the emulator
  thread procedure; both filters report into the same log file.
- Limits: a filter cannot report crashes that never reach it (stack corruption,
  termination from another process, or when a debugger handles the exception
  first), and it cannot write if the logger lock is held by another thread at
  crash time (the line is then only sent to the debug output). Crashes inside
  the emulated x86 application are reported only if they reach the emulator as
  exceptions.

## Design notes

- One `CRITICAL_SECTION` in `supp.nt.dll` protects the file handle, path, size
  and the module-name cache; every line is written with a single `_write`, so
  lines from different threads do not interleave. The file is opened in append
  mode with sharing enabled, so several processes (Launcher, emulator) can
  append to the same file; the `pid` field separates them.
- ERROR and WARN lines are flushed to disk immediately (`_commit`); other levels
  stay in the OS cache and are flushed on shutdown. This keeps the important
  events even after a crash without slowing down DEBUG/TRACE logging. ERROR and
  WARN lines are also always sent to `OutputDebugStringA`, so a debugger sees
  them even while the file is being written.
- Logger failures never stop emulation. If the log file cannot be opened or
  written, the error is recorded (debug output, `LogGetFileError`) and the
  messages continue through `OutputDebugStringA`.
- A thread-local flag prevents recursive logging; `DllMain` does not open the
  file (no file I/O under the loader lock), it only reports the load to the
  debug output. If a compiler ignores the thread-local attribute (see the ARM
  note in BUILD.md), the flag becomes an ordinary global: a concurrent line can
  then be routed to the debug output instead of the file while another thread is
  writing, but no line is lost and lines do not interleave.
- Two processes rotating at the same time can race: the rename of the loser
  fails and that process keeps appending to the current file, the same fallback
  as when another process keeps the file open.
- The logger records only what the callers pass to it; no passwords or
  credentials are collected by the logger itself.

## Windows RT verification status

- The logger uses Vista+ APIs (`InitOnceExecuteOnce`, `GetModuleHandleExA`,
  `MoveFileExA`) that are available on Windows RT, and it builds with the same
  toolset as the other DLLs (`supp.vcxproj`, v120) and with the MinGW
  `Makefile`/`Makefile.arm` targets that already compile `logging.cpp`.
- Host tests run the real `logging.cpp` against a small Windows API shim:
  file creation and format, level filtering (0..5), multi-thread line
  integrity, unavailable path (failure reason, debug output fallback, retry
  after `InitDebugLog`), rotation with backup, logging before initialization,
  unhandled-exception line formatting, the compatibility API, and a compile
  check of `supp.cpp`. 63 checks, run with:

  ```bash
  python3 yact/supp/test_logging_host.py
  ```

- Not yet verified on Windows RT ARMv7 hardware: the effective path and write
  permissions under `%TEMP%`, rotation while another process keeps the file
  open, and the unhandled exception filter in a real crash. These need a device
  run.

## API

Level helpers (exported by `supp.nt.dll`, declared in `yact/inc/util.h`):
`LogErr`, `LogWarn`, `LogInfo`, `LogPrint` (DEBUG), `LogTrace` (TRACE),
`LogIsEnabled(level)` and `LogGetName()` (effective log file path).

Utility/compatibility API (declared in `yact/supp/logging.h`): `InitDebugLog`,
`ShutdownDebugLog`, `WriteDebugLog` (DEBUG), `LogDllLoadFailure` (ERROR),
`LogEmulationStart`/`LogEmulationEnd` (DEBUG, adds the duration),
`LogInstallUnhandledExceptionFilter`, `LogGetFileError`, `LogGetFileOpened`.
