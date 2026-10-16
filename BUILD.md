# Win86emun Build Guide

## Overview

Win86emun is a usermode emulator that runs legacy x86 (Win32) applications on Windows RT (ARMv7) devices. This guide explains how to build both x86 and ARM32 versions of the emulator.

## Prerequisites

### For x86 Build (Windows 32-bit DLLs)
- Debian Linux with MinGW-w64 installed
- Package: `mingw-w64` (`i686-w64-mingw32-g++`)

### For ARM32 Build (Windows RT DLLs)
- Debian Linux with LLVM-MinGW toolchain
- Download from: https://github.com/mstorsjo/llvm-mingw/releases
- Required compiler: `armv7-w64-mingw32-g++`

## Quick Start

### Setup LLVM-MinGW for ARM32 (if not already installed)

```bash
cd /tmp
wget https://github.com/mstorsjo/llvm-mingw/releases/download/20230614/llvm-mingw-20230614-ucrt-ubuntu-18.04-x86_64.tar.xz
tar -xf llvm-mingw-20230614-ucrt-ubuntu-18.04-x86_64.tar.xz
export PATH=/tmp/llvm-mingw-20230614-ucrt-ubuntu-18.04-x86_64/bin:$PATH
```

### Build x86 Version

```bash
cd /workspace
cp Makefile.x86 Makefile
make clean
make -j$(nproc)
```

Output DLLs will be in `release/` directory:
- `yact_emu.dll` - experimental emulation core (x86); not used at runtime
- `x86_emu.dll` - emulator front end (x86)
- `supp.nt.dll` - support DLL (x86)

Note: the make build only produces the three DLLs above (and the `*.86.dll` stub DLLs when they are
requested explicitly). The engine DLLs that `x86_emu.dll` loads at runtime
(`dosbox_emu.dll` + `dosbox_emu1..15.dll`, `bochs_emu.dll`, `cbacks.nt.dll`) are built from the
Visual Studio solution (`yact.sln`), which is also what `Setup/Setup.isl` packages. A runtime
directory assembled from the make output alone cannot start an emulated application.

The solution contains a single `dosbox_emu` project. The release configuration of `x86_emu.dll`
loads 16 engine files, so create `dosbox_emu1.dll` ... `dosbox_emu15.dll` as copies of the built
`dosbox_emu.dll` before packaging; each copy is loaded as its own module and provides one emulation
instance. The debug configuration loads only `dosbox_emu.dll`.

### Build ARM32 Version

```bash
cd /workspace
cp Makefile.arm Makefile
make clean
make -j$(nproc)
```

Output DLLs will be in `release/` directory:
- `yact_emu.dll` - experimental emulation core (ARM32); not used at runtime
- `x86_emu.dll` - emulator front end (ARM32)
- `supp.nt.dll` - support DLL (ARM32)

The same note as for the x86 build applies: the DOSBox/Bochs engines and `cbacks.nt.dll` come from
`yact.sln`, not from this Makefile.

## Makefile Variants

| Makefile | Target Architecture | Cross Compiler |
|----------|---------------------|----------------|
| `Makefile.x86` | x86 (32-bit Windows) | `i686-w64-mingw32` |
| `Makefile.arm` | ARM32 (Windows RT) | `armv7-w64-mingw32` |

## Compiler Flags

### ARM32 Specific Flags
- `-march=armv7-a`: Target ARMv7-A architecture (Windows RT devices)
- `-mthumb`: Use Thumb-2 instruction set for better code density
- `-mno-unaligned-access`: Prevent unaligned memory access faults on ARM

## Deployment to Windows RT Device

1. Install the Win86emun package built from `yact.sln` with the installer. The emulator files are
   placed in the Windows directory (`%SystemRoot%`), not next to the application:

   - `x86_emu.dll` - emulator front end; it loads the engines on first use
   - `dosbox_emu.dll` + `dosbox_emu1.dll` ... `dosbox_emu15.dll` - primary engine (release build
     requires all 16 files)
   - `bochs_emu.dll` - fallback engine
   - `cbacks.nt.dll` - callback layer
   - `supp.nt.dll` - support DLL
   - `Launcher.exe`, `cpihook.dll`, `peldr.nt.dll`, the `*.86.dll` stub DLLs and the `*.nt.dll`
     system DLLs

   The names are fixed: `x86_emu.dll` loads `"cbacks.nt.dll"`, `"dosbox_emu.dll"` and
   `"bochs_emu.dll"` with `LoadLibraryA()`, so a renamed file (for example `x86_emu_arm.dll`) is not
   found and the process exits during startup. `yact_emu.dll` is an experimental engine; it is not
   installed and never loaded.

2. Start the application through `Launcher.exe` rather than copying DLLs next to the application
   executable.

3. **Important**: Your Windows RT device must be jailbroken to run x86 applications.

## Known Issues

### Warnings During Build
- `__declspec(thread)` attribute ignored on ARM: This is expected and does not affect functionality.
- `FASTCALL` macro redefined: This is harmless.

### Runtime Issues
- Some x86 applications may not work because SSE/SSE2 instructions are not implemented by any
  engine (`dosbox_emu.dll` has no XMM registers, `bochs_emu.dll` is built with `BX_CPU_LEVEL 5`).
  MMX is implemented by the DOSBox engine only.
- Applications requiring privileged instructions will fail.

### Debug Logging
- Win86emun writes `%TEMP%\win86emu.log` through `supp.nt.dll` (levels, rotation, per-call
  tracing, unhandled exceptions). See [LOGGING.md](./LOGGING.md) for the format, the registry
  settings (`LogLevel`, `LogFile`, `LogMaxSizeKB`) and the host tests.

## Testing

Tested applications (on Windows RT):
- 7-Zip: ✓ Works
- Notepad++: ✓ Works
- WinRAR: ⚠ Partial support

See [COMPATIBILITY.md](./COMPATIBILITY.md) for a detailed compatibility list.

## GitHub Release

To create a release with both x86 and ARM binaries:

```bash
git tag -a v1.0.0 -m "Release v1.0.0 (ARM + x86)"
git push origin v1.0.0

gh release create v1.0.0 \
    --title "Win86emu v1.0.0" \
    --notes "ARM32 (Windows RT) and x86 binaries. Jailbreak required for ARM." \
    release/x86_emu.dll \
    release/supp.nt.dll \
    release/yact_emu.dll
```

This only covers the files produced by the make build. A runnable package additionally needs the
engine DLLs built from `yact.sln` (`dosbox_emu.dll` + `dosbox_emu1..15.dll`, `bochs_emu.dll`,
`cbacks.nt.dll`), and every DLL must keep its original file name: the front end looks the engine
DLLs up by name, so the renames performed by `release_and_test.sh` (`yact_emu.dll` ->
`yact_emu_arm.dll`, `x86_emu.dll` -> `x86_emu_arm.dll`, `supp.nt.dll` -> `supp_arm.dll`) are only
usable for archiving the artifacts, not for running the emulator.

## License

This project is based on the original Win86emu by mamaich, which was abandoned. See the original thread for license details.

## References

- Original Project: https://sourceforge.net/projects/win86emu/
- Fork Repository: https://github.com/satouhinata1/Win86emun
- LLVM-MinGW: https://github.com/mstorsjo/llvm-mingw
- Intel SDM: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
