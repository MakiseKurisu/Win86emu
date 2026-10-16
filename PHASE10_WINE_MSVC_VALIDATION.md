# Phase 10: Wine 11.18 / Winetricks / MSVC v120 Validation

## 1. Purpose and Scope

Phase 10 verifies whether the currently working Wine 11.18 stack can support the
remaining Win86emun build items that failed or were untried under Phase 9:

1. Wine 11.18 execution and Winetricks connectivity.
2. Dedicated, non-destructive Wine prefixes.
3. Execution of 32-bit x86 Windows (PE32) programs.
4. Feasibility of MSVC v120 / Visual Studio 2013.
5. MFC and ATL development availability.
6. `test_seh.exe`, x86 `peldr`, and `Launcher.exe` investigation.
7. Preservation of existing build/test artifacts and host tests.

This report is a historical record for Phase 10. Phase 9 findings remain
unchanged in `PHASE9_WINE_MSVC.md`.

## 2. Verified Environment

| Item | Value |
|------|-------|
| Date of verification | 2026-10-17 |
| OS | macOS 15.8.1 (Build 24H32) |
| CPU | Intel x86_64 |
| Host Clang | Apple Clang 17.0.0 |
| Homebrew | 7.0.9 |
| MacPorts | 2.12.7 |
| GitHub CLI | 2.102.0 |
| LLVM-MinGW | 20261006 ucrt-macos-universal, clang 23.1.3 |
| Git branch | `work/phase6-build-environment` |
| Git HEAD | `c8abac78621d366c2a5f7beb2ccad1597b4f955e` |
| Phase 9 tag | `phase9-20261016` (annotated, remote resolves to `968803b0...`) |
| Wine application | `/Applications/Wine Devel.app` |
| Wine | 11.18 |
| Wine server | 596752 bytes at `.../wine/bin/wineserver` |
| Winetricks | 20260125 |
| cabextract | 1.11 |
| p7zip | 17.05 |

Working tree was clean at the start of Phase 10. The repository is at the Phase 9
commit and the remote branch matches.

## 3. Gatekeeper / Code Signing

Results:

```text
xattr "/Applications/Wine Devel.app"          -> no quarantine attribute (removed earlier)
codesign -dv "/Applications/Wine Devel.app"   -> code object is not signed at all
spctl --assess --type exec ...                -> rejected, source=no usable signature
```

The application bundle is not signed and Finder/Gatekeeper rejects it. The Wine
binary inside the bundle runs directly and successfully. No SIP, Gatekeeper, or
system security setting was changed during Phase 10.

## 4. Stage A: Wine 11.18 and Winetricks

### 4.1 Wine

`wine` is not on `PATH`. The installed Wine is invoked by absolute path:

```bash
WINE="/Applications/Wine Devel.app/Contents/Resources/wine/bin/wine"
"$WINE" --version
# -> wine-11.18
```

Direct execution succeeds.

### 4.2 Winetricks

```bash
command -v winetricks   # -> /usr/local/bin/winetricks
winetricks --version     # -> 20260125
```

Winetricks and Wine are bound through environment variables:

```bash
export WINE="/Applications/Wine Devel.app/Contents/Resources/wine/bin/wine"
export WINESERVER="/Applications/Wine Devel.app/Contents/Resources/wine/bin/wineserver"
export WINEPREFIX="$HOME/.wineprefixes/win86emun-test"
```

Version binding test:

```bash
env WINE="$WINE" WINESERVER="$WINESERVER" WINEPREFIX="$WINEPREFIX" winetricks --version
# -> 20260125 ... wine-11.18, WINEARCH=win64
```

### 4.3 Existing Default Prefix

`$HOME/.wine` existed before Phase 10 and was not deleted or recreated. It was
treated as an existing prefix and left intact.

### 4.4 Dedicated Prefix: `win86emun-test`

Prefix: `$HOME/.wineprefixes/win86emun-test`

Observed properties:

```text
drive_c/windows/syswow64 exists
dosdevices/c: -> ../drive_c
dosdevices/z: -> /
```

Therefore the prefix is a 64-bit prefix (Wine wow64 mode). `wineboot -u` completed
with exit code 0, although the MoltenVK duplicate-class and Vulkan/Metal messages
appear; they are non-fatal graphics-stack warnings.

Winetricks reports:

```text
Using winetricks 20260125 ... with wine-11.18 and WINEARCH=win64
warning: 64-bit WINEPREFIX ...
warning: Wine's new wow64 mode ... EXPERIMENTAL
warning: ~/.wineprefixes/win86emun-test/winetricks.log not found
```

`winetricks list-installed` works and accesses the referenced prefix. This is the
minimum operation that proves the prefix wiring.

### 4.5 Important Architecture Constraint

Wine 11.18 in this installation runs in new wow64 mode. An attempt to initialize a
32-bit-only prefix with `WINEARCH=win32` failed immediately:

```text
wine: WINEARCH is set to 'win32' but this is not supported in wow64 mode.
```

Consequence: only 64-bit prefixes are available. 32-bit Windows programs still run
through Wine's wow64 layer, and this was verified separately in Stage B. Any
future MSVC installation must account for 64-bit-only prefixes.

### 4.6 Winetricks Runtime Test: `vcrun2013`

A separate 64-bit prefix was created:

```text
$HOME/.wineprefixes/win86emun-vcrun2013
```

Command:

```bash
env WINE="$WINE" WINESERVER="$WINESERVER" WINEPREFIX="$WINEPREFIX" winetricks -q vcrun2013
```

Result: **success**. Exit code 0. Winetricks downloaded
`vcredist_x64.exe` (about 7.0 MiB) from `download.microsoft.com` and installed it.
After installation `winetricks list-installed` reports `vcrun2013`, and the prefix
contains:

```text
drive_c/windows/system32/msvcr120.dll
drive_c/windows/system32/mfc120.dll
```

This verifies real Wine/Winetricks installation behavior, not only version output.

## 5. Stage B: 32-Bit x86 PE32 Execution

A minimal console program was built with LLVM-MinGW:

```bash
i686-w64-mingw32-gcc -O2 -o /tmp/win86_test/hello2.exe hello2.c
```

Result:

```text
PE32 executable (console) Intel 80386
Machine: IMAGE_FILE_MACHINE_I386 (0x014C)
```

Execution in the dedicated 64-bit Wine prefix:

```bash
WINEDEBUG=-all "$WINE" /tmp/win86_test/hello2.exe
# stdout: Win86emun Stage B: hello from Wine x86, exit 42
# exit code: 42
```

Result: **success** for PE32 x86 console execution. Wine's wow64 layer loaded and
executed the 32-bit binary.

A GUI `MessageBoxA` variant was also built successfully as a PE32, but it was not
executed unattended because it blocks on a dialog. Console execution is the
verified result.

## 6. Stage C: MSVC v120 / Visual Studio 2013 Feasibility

### 6.1 Fixed Findings

No MSVC v120 installation media or installer was present on the host. Searches of
the repository, Wine prefixes, common download folders, and the system did not find
`cl.exe` or Visual Studio 2013 installation files.

Official availability notes (verified 2026-10-17):

- Visual Studio 2013 Update 5 release notes state that the version is no longer
  supported and redirect current downloads to the latest Visual Studio.
- Microsoft provides Visual Studio 2013 downloads only through `my.visualstudio.com`,
  which requires a free Dev Essentials registration or a Visual Studio
  subscription.
- Microsoft Download Center contains TFS Server/ISO and vcredist packages, not the
  Visual Studio IDE/compiler installer as a directly available unauthenticated
  package.
- The Internet Archive contains a VS 2013 Community ISO, but that is not a
  publisher-provided download and was not used because the project rule is to use
  official distribution and license terms.
- `vcredist_x86`/`vcredist_x64` are runtime packages only. They do not contain
  `cl.exe`, `link.exe`, MFC/ATL headers, or import libraries.

### 6.2 MSVC Stage Assessment

| MSVC stage | Result |
|------------|--------|
| 1. Official media available without account/subscription | **Not verified/unavailable through normal unattended access** |
| 2. Installer launched on Wine | **Not attempted** (no installer available) |
| 3. Complete installation | **Not attempted** |
| 4. `cl.exe` executed and version obtained | **Not attempted** |
| 5. Minimal x86 program compiled | **Not attempted** |
| 6. Linking succeeded | **Not attempted** |
| 7. Win86emun target code compiled | **Not attempted** |

Classification for MSVC v120 in the summary table: **not attempted / unverified**.

No statement is made that MSVC v120 was installed, that `cl.exe` works, or that
any Win86emun target was built with it.

## 7. Stage D: Winetricks vs MSVC / MFC / ATL

The following separation was maintained throughout:

| Term | Actual availability |
|------|---------------------|
| `vcrun2013` | 32/64-bit runtime DLLs for Visual C++ 2013 (MFC 120, ATL 120, CRT). Installed successfully on Wine 11.18. |
| MSVC v120 | Compiler and linker toolset. Not installed and not verified. |
| MFC | Runtime DLLs can be installed via `vcrun2013`; development headers (`afxwin.h`, etc.) and import libraries are absent. |
| ATL | No development headers present. The repository has no live ATL code path; the only reference is a commented-out include block in `yact/system_x86/ShellIntf/stdafx.h`. |
| Windows SDK | No SDK installation present on host or in the Wine prefixes. |

Runtime presence does not imply build capability. `vcrun2013` was not counted as
MSVC v120 success.

## 8. Stage E: Target-Specific Results

### 8.1 `test_seh.exe`

Reproduction command (x86, Clang/LLVM-MinGW):

```bash
i686-w64-mingw32-g++ -std=c++11 -c \
  -DWIN32 -D_DEBUG -D_CONSOLE \
  -I yact/test_seh \
  yact/test_seh/test_seh.cpp
```

Result: **failure**. Representative errors:

```text
test_seh.cpp:73:11: error: expected 'volatile', 'inline', 'goto', or '('
test_seh.cpp:92:11: error: expected 'volatile', 'inline', 'goto', or '('
test_seh.cpp:126:5: error: non-ASM statement in naked function is not supported
```

Cause confirmed at code level: the file uses MSVC `__asm {}` blocks and
`__declspec(naked)` functions. The root non-portable constructs are:

- `__declspec(naked)` functions with non-empty non-assembly bodies such as
  `MyRtlUnwind`.
- MSVC inline assembly `__asm { ... }`, including `__LOCAL_SIZE`, named register
  operands, and MSVC-style memory operands.
- An `ExecuteHandler` declaration whose last parameter is a function pointer but
  whose call site passes an `ExceptionRoutine` value.

No MSVC build was attempted. No source changes were made in Phase 10 because the
correct conversion requires ABI and SEH-register/frame semantics review rather
than mechanical replacement.

### 8.2 x86 `peldr`

Reproduction command (x86, Clang/LLVM-MinGW, same style of flags used for the
successful ARM build):

```bash
i686-w64-mingw32-g++ -std=c++17 -O2 -fms-extensions \
  -DUNICODE -D_UNICODE -DWIN32 -DNDEBUG \
  -D__in= -D__out= \
  -D'min(a,b)=...' -D'max(a,b)=...' -D'PE_EXPORT=...' \
  -I yact/peldr -I yact/inc -I yact \
  -c yact/peldr/peldr.cpp
```

Result: **failure**. Representative errors:

```text
error: assembler label 'L__ehtable$PeLdrIsValidHandleP6PeFile' can not be undefined
error: assembler label 'L__ehtable$PeLdrGetProcAddressA' can not be undefined
error: assembler label 'L__ehtable$ValidatorThreadProc...' can not be undefined
```

Confirmed: the failure occurs in x86 SEH code generation for `__try` / `__except`
blocks. The ARM32 build of the same source succeeds, confirming that this is
target/compiler-specific rather than a generic source syntax error.

### 8.3 `Launcher.exe`

Result: **not attempted**.

Reason: `Launcher.cpp` and `LauncherDlg.cpp` require MFC (`CWinApp`, `CDialogEx`,
message maps, `afxdialogex.h`). Neither MFC development headers nor MFC import
libraries are installed. The project file requires `PlatformToolset v120`.

Runtime MFC DLLs from `vcrun2013` are not sufficient to compile or link this
project.

### 8.4 ATL Minimal Test

Result: **not necessary as a standalone build**.

No project currently depends on ATL. The only ATL mention in the repository is
commented out because the wizard-generated includes were never used. Therefore no
ATL development library is currently on the critical path; only Visual
Studio-provided ATL headers would be needed if future code introduced ATL
components.

## 9. Host Tests (Pre/Post Comparison)

Commands:

```bash
CXX=/usr/bin/clang++ python3 yact/supp/test_logging_host.py
CXX=/usr/bin/clang++ python3 yact/yact_emu/test_isa_host.py
python3 yact/cbacks/test_callback_arity.py
```

Results:

| Test | Checks / assertion output | Failures | Exit code | Result |
|------|---------------------------|----------|-----------|--------|
| `test_logging_host.py` | 63 checks | 0 | 0 | PASS |
| `test_isa_host.py` | 573 checks | 0 | 0 | PASS |
| `test_callback_arity.py` | script prints OK | 0 | 0 | PASS |

Note: when LLVM-MinGW is prepended to `PATH`, `test_logging_host.py` defaults
to `clang++` from LLVM-MinGW and produces a Windows PE binary that cannot run on
macOS. Use `CXX=/usr/bin/clang++` for host tests. This is an execution
environment caution, not a product failure.

No host test was weakened or modified.

## 10. PE Machine Verification

Current contents of `release/`:

```text
95 DLLs, all PE32 ARMv7 Thumb
Machine: IMAGE_FILE_MACHINE_ARMNT (0x01C4)
```

Sampled exports/dependencies were not rebuilt during Phase 10. The repository’s
Phase 9 counts are preserved:

| Build | DLL count | Machine | Notes |
|-------|-----------|---------|-------|
| x86 | 79 | I386 (`0x014C`) | Phase 8/9 verification; count and machine values retained |
| ARM32 | 95 | ARMNT (`0x01C4`) | Current `release/` contents verified as ARMNT |

The current `release/` directory is not a mixed-architecture build. PE headers were
verified, but this only proves file format, not runtime execution on Windows RT.

## 11. License and Redistribution Notes

Phase 10 added no Microsoft compiler, SDK, MFC, or ATL development files to the
repository or to release artifacts. The following were used as environment/runtime
elements only:

- Wine 11.18 (already installed by the user).
- Winetricks 20260125 (Homebrew).
- `vcredist_x64.exe` downloaded by Winetricks into a private prefix cache
  (`$HOME/.cache/winetricks`) and installed into a dedicated prefix.

The new `win86emun-vcrun2013` prefix and Winetricks cache must not be included in
any future release package. Release packaging must continue to contain only the
project's own DLLs/scripts/docs.

No new GitHub Release was created in Phase 10, as requested.

## 12. Failures With Confirmed vs. Inferred Causes

Confirmed:

- Wine 11.18 direct execution works.
- Wine 11.18 is 64-bit-only in this wow64 mode; `WINEARCH=win32` is rejected.
- Winetricks binds to the selected Wine executable and prefix.
- `vcrun2013` installs successfully on a 64-bit prefix.
- 32-bit PE32 x86 console programs execute under Wine 11.18.
- `test_seh.cpp` fails under Clang/LLVM-MinGW because of MSVC inline assembly and
  naked functions.
- x86 `peldr.cpp` fails under Clang/LLVM-MinGW because of i686 SEH code
  generation (`L__ehtable$...`).
- Host tests pass with 63 + 573 + callback arity success.

Inferred or environment-limited, not conclusively proven:

- Whether a future MSVC v120 installer would launch on Wine 11.18 (no installer
  was available to test).
- Whether the Wine MoltenVK duplicate-class warning affects application behavior
  (no failure was observed, but it remains an environment caveat).
- Whether Windows RT devices would execute the verified ARM32 PE files (no RT
  device was available).

## 13. Remaining Issues

1. Obtain official Visual Studio 2013 / v120 media under a Microsoft subscription
   if compiler-grade MSVC is required.
2. If MSVC v120 becomes available, rebuild `test_seh.exe`, x86 `peldr`, and
   `Launcher.exe` in 64-bit Wine prefixes (32-bit-only prefixes are unavailable).
3. Decide whether `test_seh.cpp` needs a genuine GNU-assembly or separate-source
   port, not a textual substitution.
4. Either port x86 `peldr` SEH to a non-clang-safe form or restrict it to
   MSVC-only builds as already documented in BUILD.md.
5. Maintain the distinction between runtime DLL availability and development
   headers/libraries; `vcrun2013` being installed does not advance MFC/ATL
   development status.
6. Avoid adding Wine prefixes, caches, installers, or absolute local paths to any
   release artifact.

## 14. Recommended Next Steps

Priority order:

1. Document the confirmed host/Wine verification results above and keep Phase 9
   unchanged.
2. Continue the make-based ARM32/x86 builds as the primary supported path.
3. Only pursue MSVC v120 if a legally obtained installer and a Windows/Microsoft
   subscription path are available.
4. Before any future source port of SEH code, prepare a targeted ABI test in a
   VM or on a Windows host.
5. Create a GitHub Release only after license/redistribution review of any new
   artifacts.

## 15. Phase 10 Status Summary

| Target | Result |
|--------|--------|
| Wine 11.18 | Success |
| Winetricks connectivity | Success |
| Dedicated Wine prefix | Success (64-bit only) |
| x86 PE32 execution | Success |
| Winetricks runtime (`vcrun2013`) | Success |
| MSVC v120 compiler | Not attempted / unverified |
| MFC development headers/libraries | Not available |
| ATL development headers/libraries | Not available |
| `test_seh.exe` (Clang/MinGW) | Failed |
| x86 `peldr` (Clang/MinGW) | Failed |
| `Launcher.exe` | Not attempted |
| Host tests | Success |
| Windows RT hardware | Not available / not claimed |
