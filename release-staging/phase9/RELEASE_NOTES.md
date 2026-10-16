# Win86emun Phase 9 — Wine/MSVC Build Experiments

## Release Overview

This release contains the verified build artifacts from Phase 8, with Phase 9 experiments for Wine/MSVC v120/MFC/ATL build environment.

**Status:** Experimental (Windows RT device testing not performed)

## Build Information

| Item | Value |
|------|-------|
| macOS | 15.8.1 (Build 24H32) |
| CPU | x86_64 (Intel) |
| Apple Clang | 17.0.0 |
| LLVM-MinGW | 20261006 ucrt-macos-universal |
| Homebrew | 5.1.15 |
| MacPorts | 2.12.7 |
| Wine | Installing via MacPorts |

## Architecture Information

### x86 (79 DLLs)
- **Machine ID:** I386 (0x14C)
- **Format:** PE32
- **Contents:** Core emulator DLLs + 74 system DLL stubs

### ARM32 (95 DLLs)
- **Machine ID:** ARMNT (0x1C4)
- **Format:** PE32
- **Contents:** Core emulator DLLs + peldr.nt + cpihook.nt + dosbox_emu + bochs_emu + 72 system DLLs + 15 dosbox slots

## Test Status

| Test | Status | Details |
|------|--------|---------|
| x86 Build | PASS | 79 DLLs, all I386 |
| ARM32 Build | PASS | 95 DLLs, all ARMNT |
| Host Tests | PASS | 63 + 573 + callback checks, 0 failures |
| Wine Tests | Pending | Installation in progress |
| MSVC v120 | Pending | Requires Wine |
| MFC/ATL | Pending | Requires MSVC v120 |
| Windows RT Device | NOT PERFORMED | No physical device available |

## Phase 9 Experiments

### x86 peldr (SEH Issue)
- **Status:** Build fails with clang/LLVM-MinGW
- **Error:** Undefined assembler labels for SEH (`__try`/`__except`)
- **Next:** Attempt with MSVC v120 via Wine

### test_seh.exe (MSVC Inline Assembly)
- **Status:** Build fails with clang/LLVM-MinGW
- **Error:** MSVC inline assembly not supported by clang
- **Next:** Attempt with MSVC v120 via Wine

### Launcher.exe (MFC)
- **Status:** Not yet attempted
- **Requirements:** MSVC v120 with MFC
- **Next:** Attempt after Wine + MSVC v120 installation

## Attached Files

| File | Architecture | Contents |
|------|--------------|----------|
| Win86emun-phase9-x86-dlls.zip | x86 (I386) | 79 DLLs |
| Win86emun-phase9-arm32-dlls.zip | ARM32 (ARMNT) | 95 DLLs |
| SHA256SUMS.txt | - | SHA-256 checksums |

## Important Limitations

1. **Windows RT device testing has NOT been performed.** These binaries are verified only for PE format and architecture, not for actual Windows RT execution.
2. **Wine/MSVC v120/MFC/ATL experiments are in progress.** Results will be documented separately.
3. **This is an experimental pre-release.** Do not use for production.

## License

This project is based on the original Win86emu by mamaich. See the original thread for license details.
