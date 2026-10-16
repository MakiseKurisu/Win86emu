# Phase 11: Wine / Winetricks / MSVC v120 / Compiler Portability Report

## 1. Purpose

Phase 11 extends Phase 10 by actually operating Wine/Winetricks, obtaining an
official Visual Studio 2013 (v120) installer, attempting installation under Wine,
and analyzing `test_seh.exe`, x86 `peldr`, and `Launcher.exe`.

## 2. Environment

| Item | Value |
|------|-------|
| OS | macOS 15.8.1 (Build 24H32) |
| CPU | Intel x86_64 |
| Host Clang | Apple Clang 17.0.0 |
| LLVM-MinGW | 20261006 ucrt-macos-universal, clang 23.1.3 |
| Wine | 11.18 (`/Applications/Wine Devel.app/.../wine`) |
| Winetricks | 20260125 |
| Git HEAD (start) | `1c804d8a7f079654d82e5bbb269d66e037b9316b` |
| Branch | `work/phase6-build-environment` |

## 3. Wine / Winetricks Operations

### 3.1 Wine

```text
wine --version -> wine-11.18
wineboot -u -> exit 0
```

Wine runs directly despite Gatekeeper rejection (bundle unsigned, quarantine
removed earlier). No security setting was changed.

### 3.2 Winetricks

```text
winetricks --version -> 20260125
```

A dedicated 64-bit prefix was created:

```text
$HOME/.wineprefixes/win86emun-msvc-v120
WINEARCH=win64
```

`winetricks list-installed` works against this prefix. `vcrun2013` was installed
successfully in this prefix (exit 0), providing `msvcr120.dll` and `mfc120.dll`
runtime files.

Important constraint: Wine 11.18 in this installation runs in new wow64 mode.
`WINEARCH=win32` is rejected, so only 64-bit prefixes are available. 32-bit
x86 programs still execute through wow64 (verified in Phase 10).

### 3.3 .NET Dependency Attempt

`winetricks -q dotnet45` was attempted in the MSVC prefix. It downloaded
`dotNetFramework` (about 48 MiB) but the .NET 4.5 installer did not complete
successfully under Wine 11.18. The prefix ultimately reports only
`remove_mono internal`, not `dotnet45`.

This is significant because the Visual Studio 2013 installer is a .NET-based
burn bundle and depends on .NET 4.5.1.

## 4. MSVC v120 Official Acquisition

### 4.1 Official Route

Microsoft's documentation states that Visual Studio 2013 downloads require
My.VisualStudio.com, which requires a free Dev Essentials registration or a
Visual Studio Subscription. The public older-downloads page lists Visual Studio
2022/2019/2017, not 2013, as directly downloadable without subscription.

A direct official Microsoft download URL for the Visual Studio 2013 Community
Update 5 ISO was located on `download.microsoft.com`:

```text
URL: https://download.microsoft.com/download/A/A/D/AAD1AA11-FF9A-4B3C-8601-054E89260B78/vs2013.5_ce_enu.iso
File: vs2013.5_ce_enu.iso
Size: 5,115,258,880 bytes
SHA-256: 7de2340d9570895451c5f5d9f2081db860a4f753e7596e31b2742390a6d0e6bc
Type: ISO 9660 CD-ROM filesystem data 'VS2013_5_CE_ENU'
```

The file was downloaded successfully and its size matches the server-reported
`Content-Length`.

### 4.2 Installer Launch

The ISO was mounted read-only at `/Volumes/VS2013_5_CE_ENU`. It contains
`vs_community.exe`.

Attempt 1 (`/Passive /NoRestart`): the process hung in the Wine GUI/runtime and
was terminated. No installation completed.

Attempt 2 (`/Quiet /NoRestart`): the burn bundle started and launched the .NET
Framework 4.5.1 redistributable (`NDP451-KB2858728-x86-x64-AllOS-ENU.exe`).
That sub-installer consumed near 100% CPU and repeatedly respawned with new
pipe IDs. It did not complete. The run was terminated.

After both attempts:

```text
find .../win86emun-msvc-v120/drive_c -iname cl.exe -> no result
```

No Visual Studio 2013, Windows SDK, MFC, or ATL development files were installed.

## 5. MSVC Stage Assessment

| MSVC stage | Result |
|------------|--------|
| 1. Official media available | Success (downloaded ISO) |
| 2. Installer launched on Wine | Partial / failed (installer started, then hung) |
| 3. Complete installation | Failed |
| 4. `cl.exe` executed/version | Not available |
| 5. Minimal x86 program compiled | Not attempted (no compiler) |
| 6. Linking succeeded | Not attempted |
| 7. Win86emun target code compiled | Not attempted |
| 8. MFC development headers/libraries | Not available |
| 9. ATL development headers/libraries | Not available |

Classification: MSVC v120 = **failed installation / unverified compiler**.
The ISO download itself succeeded, but the installer did not complete under
Wine 11.18.

## 6. Target-Specific Analysis

### 6.1 `test_seh.exe`

Reproduction (x86, Clang/LLVM-MinGW):

```bash
i686-w64-mingw32-g++ -std=c++11 -c \
  -DWIN32 -D_DEBUG -D_CONSOLE \
  -I yact/test_seh \
  yact/test_seh/test_seh.cpp
```

Result: failure. Representative errors:

```text
test_seh.cpp:73:11: error: expected 'volatile', 'inline', 'goto', or '('
test_seh.cpp:92:11: error: expected 'volatile', 'inline', 'goto', or '('
test_seh.cpp:126:5: error: non-ASM statement in naked function is not supported
```

Source-level causes:

- `__declspec(naked)` functions contain non-assembly statements, especially
  `MyRtlUnwind`.
- MSVC inline assembly (`__asm { ... }`) is used throughout.
- MSVC-specific constructs such as `__LOCAL_SIZE` and SAL annotations (`__in`)
  appear.
- The test hooks `RtlUnwind` and `RaiseException` by overwriting the first bytes
  of the system functions, so register/stack/exception-unwinding behavior must be
  preserved exactly.

A textual syntax replacement is not acceptable. Any port must be validated
against the intended SEH unwinding behavior.

### 6.2 x86 `peldr`

Reproduction (x86, Clang/LLVM-MinGW, flags matching the successful ARM build):

```bash
i686-w64-mingw32-g++ -std=c++17 -O2 -fms-extensions \
  -DUNICODE -D_UNICODE -DWIN32 -DNDEBUG \
  -D__in= -D__out= \
  -D'min(a,b)=...' -D'max(a,b)=...' -D'PE_EXPORT=...' \
  -I yact/peldr -I yact/inc -I yact \
  -c yact/peldr/peldr.cpp
```

Result: failure. Representative errors:

```text
error: assembler label 'L__ehtable$PeLdrIsValidHandleP6PeFile' can not be undefined
error: assembler label 'L__ehtable$PeLdrGetProcAddressA' can not be undefined
error: assembler label 'L__ehtable$ValidatorThreadProc...' can not be undefined
```

The same source builds successfully for ARM32 with the same `-fms-extensions`
flag, confirming that this is an x86 SEH code-generation issue rather than a
generic syntax problem.

The failures occur in functions containing `__try` / `__except` blocks. The
compiler accepts a minimal isolated `__try`/`__except` function, so the
breakage depends on the surrounding function complexity and flags.

### 6.3 `Launcher.exe`

Project settings:

```text
PlatformToolset: v120
UseOfMfc: Dynamic / Static
CharacterSet: Unicode
SubSystem: Windows
AdditionalDependencies: imagehlp.lib
```

Source dependencies:

- `afxwin.h`, `afxdialogex.h`
- `CDialogEx`, `CWinApp`, message maps
- MFC resources and dialog templates

Result: **not buildable** in the current environment because MFC development
headers and libraries are absent. Runtime `mfc120.dll` from `vcrun2013` does not
satisfy build requirements.

## 7. Host Tests (Regression)

```text
CXX=/usr/bin/clang++ python3 yact/supp/test_logging_host.py -> 63 checks, 0 failures, exit 0
CXX=/usr/bin/clang++ python3 yact/yact_emu/test_isa_host.py -> 573 checks, 0 failures, exit 0
python3 yact/cbacks/test_callback_arity.py -> OK, exit 0
git diff --check -> no issues
```

No host test was modified.

## 8. PE Machine Verification

Current `release/` contains 95 DLLs, all `IMAGE_FILE_MACHINE_ARMNT (0x01C4)`.
The x86 build (79 DLLs, `0x014C`) was not regenerated in Phase 11; its prior
verification remains unchanged.

## 9. License / Redistribution Notes

The downloaded VS 2013 ISO is stored outside the repository in the user's
Downloads directory. It was not added to the repository or to any release
artifact. Wine prefixes and installers must remain excluded from release
packages.

## 10. Remaining Issues

1. Visual Studio 2013 installation did not complete under Wine 11.18 because the
   bundled .NET 4.5.1 installer hung/looped.
2. `cl.exe`, `link.exe`, Windows SDK, MFC, and ATL development files are still
   unavailable.
3. `test_seh.cpp` requires MSVC-specific assembly and cannot be built with
   Clang/LLVM-MinGW.
4. x86 `peldr` cannot be built with Clang/LLVM-MinGW due to i686 SEH code
   generation.
5. `Launcher.exe` cannot be built without MFC development files.
6. No Windows RT hardware was available for runtime verification.

## 11. Recommended Next Steps

1. If MSVC v120 is essential, try a Windows VM or a Microsoft-hosted build
   environment instead of Wine for the installer.
2. Keep the make-based ARM32/x86 builds as the supported path.
3. For `test_seh`, design a validated GNU/external-assembly port separately;
   do not perform mechanical text substitution.
4. For x86 `peldr`, either maintain it as MSVC-only or isolate the SEH blocks
   and prove ABI equivalence before replacing them.
5. Preserve Phase 9/10/11 reports as historical records.

## 12. Phase 11 Status Summary

| Target | Result |
|--------|--------|
| Wine 11.18 | Success |
| Winetricks | Success |
| Dedicated Wine prefix | Success (64-bit only) |
| MSVC media download | Success (ISO downloaded and SHA-256 verified) |
| MSVC installer on Wine | Failed (hung during .NET 4.5.1) |
| `cl.exe` / `link.exe` | Not available |
| Minimal x86 compile | Not attempted |
| MFC development files | Not available |
| ATL development files | Not available |
| `test_seh` (Clang/MinGW) | Failed |
| x86 `peldr` (Clang/MinGW) | Failed |
| `Launcher.exe` | Not buildable |
| Host tests | Success |
| Windows RT hardware | Not available |