# Phase 9: Wine/MSVC Build Experiments

## Environment

| Item | Value |
|------|-------|
| macOS | 15.8.1 (Build 24H32) |
| CPU | x86_64 (Intel) |
| Homebrew | 5.1.15 |
| MacPorts | 2.12.7 |
| Wine | Installing via MacPorts |
| LLVM-MinGW | 20261006 ucrt-macos-universal |
| Apple Clang | 17.0.0 |

## Homebrew Status

- Homebrew 5.1.15 is installed and functional
- `brew install --cask wine-stable` fails: cask disabled (Gatekeeper check failure, 2026-09-01)
- Intel Mac is Tier 3 configuration (limited support)

## MacPorts Installation

- MacPorts 2.12.7 installed from source
- Installation path: `/opt/local`
- Wine installation in progress

## Wine Installation

- Installing via MacPorts: `sudo port install wine-stable`
- Build dependencies: cmake, etc.
- Status: In progress

## MSVC v120 / Visual Studio 2013

- Not yet attempted
- Requires Wine to be installed first
- VS 2013 installer needs to be obtained from Microsoft

## MFC/ATL

- Not yet attempted
- Requires MSVC v120 with MFC/ATL components

## Build Results

### x86 peldr (SEH issue)

**Status:** Build fails with clang/LLVM-MinGW

**Error:**
```
error: assembler label 'L__ehtable$_ZL18PeLdrIsValidHandleP6PeFile' can not be undefined
error: assembler label 'L__ehtable$PeLdrGetProcAddressA' can not be undefined
...
```

**Cause:** clang's i686 SEH code generation produces undefined assembler labels for `__try`/`__except` blocks.

**Possible solutions:**
1. Use MSVC v120 via Wine
2. Disable SEH and use alternative exception handling
3. Separate SEH code into MSVC-specific files

### test_seh.exe (MSVC inline assembly)

**Status:** Build fails with clang/LLVM-MinGW

**Error:**
```
error: expected 'volatile', 'inline', 'goto', or '('
error: non-ASM statement in naked function is not supported
```

**Cause:** MSVC inline assembly (`__asm {}`) and `__declspec(naked)` are not supported by clang.

**Possible solutions:**
1. Use MSVC v120 via Wine
2. Separate assembly into `.asm` files
3. Use GNU inline assembly syntax

### Launcher.exe (MFC)

**Status:** Not yet attempted

**Requirements:**
- MSVC v120 with MFC
- Windows SDK
- ARM32 or x86 target

## Existing Builds (Phase 8)

### x86 Build (79 DLLs)
- Core: supp.nt.dll, x86_emu.dll, yact_emu.dll, cbacks.nt.dll
- Stubs: 74 *.86.dll
- Support: supp.86.dll

### ARM32 Build (95 DLLs)
- Core: supp.nt.dll, x86_emu.dll, yact_emu.dll, cbacks.nt.dll
- Native: peldr.nt.dll, cpihook.nt.dll, dosbox_emu.dll, bochs_emu.dll
- System: 72 *.nt.dll
- Slots: dosbox_emu1..15.dll

## Host Tests

| Test | Checks | Failures | Status |
|------|--------|----------|--------|
| test_logging_host.py | 63 | 0 | PASS |
| test_isa_host.py | 573 | 0 | PASS |
| test_callback_arity.py | - | 0 | PASS |

## Next Steps

1. Complete Wine installation
2. Test Wine with minimal Windows program
3. Attempt VS 2013 installation via Wine
4. Build Launcher.exe with MFC
5. Build test_seh.exe with MSVC v120
6. Build x86 peldr with MSVC v120
7. Create GitHub Release
