# Win86emun Build Guide

## Overview

Win86emun is a usermode emulator that runs legacy x86 (Win32) applications on Windows RT (ARMv7) devices. This guide explains how to build both x86 and ARM32 versions of the emulator.

## Prerequisites

### For x86 Build (Windows 32-bit DLLs)
- MinGW-w64 (`i686-w64-mingw32-g++`), or the LLVM-MinGW toolchain below, which provides the
  same target on Linux and macOS

### For ARM32 Build (Windows RT DLLs)
- LLVM-MinGW toolchain with `armv7-w64-mingw32-g++`
- Download from: https://github.com/mstorsjo/llvm-mingw/releases
- Linux: use a `*-ucrt-ubuntu-*-x86_64.tar.xz` archive
- macOS (Intel and Apple Silicon): use the `*-ucrt-macos-universal.tar.xz` archive
  (verified on Intel with release `20261006`)

### For the Full Solution (`yact.sln`)
- Windows host with Visual Studio 2013 Update 5 (platform toolset `v120`) and the Windows
  SDK 8.0/8.1 ARM build tools. Every project in `yact.sln` uses `PlatformToolset v120`.
  The engine DLLs, `peldr.nt.dll`, `cpihook.dll` and `Launcher.exe` are not part of the
  make build (see "MSVC (`yact.sln`) Build Requirements").

## Quick Start

### Setup LLVM-MinGW (if not already installed)

Linux:

```bash
cd /tmp
wget https://github.com/mstorsjo/llvm-mingw/releases/download/20261006/llvm-mingw-20261006-ucrt-ubuntu-22.04-x86_64.tar.xz
tar -xf llvm-mingw-20261006-ucrt-ubuntu-22.04-x86_64.tar.xz
export PATH=/tmp/llvm-mingw-20261006-ucrt-ubuntu-22.04-x86_64/bin:$PATH
```

macOS (Intel and Apple Silicon; verified on Intel):

```bash
cd /tmp
curl -L -O https://github.com/mstorsjo/llvm-mingw/releases/download/20261006/llvm-mingw-20261006-ucrt-macos-universal.tar.xz
tar -xf llvm-mingw-20261006-ucrt-macos-universal.tar.xz
export PATH=/tmp/llvm-mingw-20261006-ucrt-macos-universal/bin:$PATH

armv7-w64-mingw32-g++ --version   # clang 23.1.3, target armv7-w64-windows-gnu
```

Note: macOS archives of LLVM-MinGW are available as UCRT variants only (there is no msvcrt
archive for macOS). See "Windows RT Runtime Requirements" below.

### Build x86 Version

```bash
cd <repository root>
make -f Makefile.x86 clean
make -f Makefile.x86 -j4          # use -j$(nproc) on Linux
```

Output DLLs will be in `release/` directory:
- `supp.nt.dll` - support DLL, includes the debug logger (x86)
- `x86_emu.dll` - emulator front end (x86)
- `yact_emu.dll` - experimental emulation core (x86); not used at runtime
- `cbacks.nt.dll` - callback layer (x86; links against `supp.nt.dll` and `x86_emu.dll`)

Note: the make build produces the four DLLs above. The engine DLLs that `x86_emu.dll` loads at
runtime (`dosbox_emu.dll` + `dosbox_emu1..15.dll`, `bochs_emu.dll`) and the remaining runtime
files (`peldr.nt.dll`, `cpihook.dll`, `Launcher.exe` and the `*.86.dll` stub DLLs) are built
from the Visual Studio solution (`yact.sln`), which is also what `Setup/Setup.isl` packages.
The Makefiles contain an optional `stubs` target, but it does not build with the MinGW
toolchains: the `*.86.dll` stubs are x86 DLLs written with MSVC inline assembly
(`yact/inc/dllstub_def.h`). A runtime directory assembled from the make output alone cannot
start an emulated application.

The solution contains a single `dosbox_emu` project. The release configuration of `x86_emu.dll`
loads 16 engine files, so create `dosbox_emu1.dll` ... `dosbox_emu15.dll` as copies of the built
`dosbox_emu.dll` before packaging; each copy is loaded as its own module and provides one emulation
instance. The debug configuration loads only `dosbox_emu.dll`.

### Build ARM32 Version

```bash
cd <repository root>
make -f Makefile.arm clean
make -f Makefile.arm -j4          # use -j$(nproc) on Linux
```

Output DLLs will be in `release/` directory:
- `supp.nt.dll` - support DLL, includes the debug logger (ARM32)
- `x86_emu.dll` - emulator front end (ARM32)
- `yact_emu.dll` - experimental emulation core (ARM32); not used at runtime
- `cbacks.nt.dll` - callback layer (ARM32; links against `supp.nt.dll` and `x86_emu.dll`)

The same note as for the x86 build applies: the DOSBox/Bochs engines, the `*.86.dll` stubs and
the remaining runtime files come from `yact.sln`, not from this Makefile.

Note: both Makefiles write to `release/` and use the same object file names, so build the two
architectures in separate working copies (or run `make clean` in between) if both outputs are
needed at the same time.

### Host Tests

The host tests run on the build machine (no Windows toolchain needed):

```bash
python3 yact/supp/test_logging_host.py      # logger: 63 checks
python3 yact/yact_emu/test_isa_host.py      # ISA regression harness: 573 checks
python3 yact/cbacks/test_callback_arity.py  # callback arity invariants
```

All three exit with status 0 and print `OK`/`0 failures` on success.

### Inspecting the Built DLLs

```bash
file release/*.dll
# ARM32: PE32 executable (DLL) (GUI) ARMv7 Thumb, for MS Windows
# x86:   PE32 executable (DLL) (GUI) Intel 80386, for MS Windows

llvm-readobj --file-headers --coff-exports --coff-imports release/supp.nt.dll
# ARM32: Machine: IMAGE_FILE_MACHINE_ARMNT (0x1C4)
# x86:   Machine: IMAGE_FILE_MACHINE_I386 (0x14C)

objdump -p release/x86_emu.dll        # works with the system objdump on macOS
```

Checks that were verified for the ARM32 build:
- `supp.nt.dll` exports the logger API (`LogErr`, `LogWarn`, `LogInfo`, `LogPrint`, `LogTrace`,
  `LogIsEnabled`, `LogGetName`, `InitDebugLog`, `ShutdownDebugLog`, ...).
- `x86_emu.dll` imports the logger functions from `supp.nt.dll`; `cbacks.nt.dll` imports from
  both `supp.nt.dll` and `x86_emu.dll`.
- All DLLs additionally import `KERNEL32.dll` and the UCRT API sets (`api-ms-win-crt-*`).

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

## Build Host Notes

### macOS (verified: macOS 15.8.1, Intel x86_64)

- Host tools: Xcode Command Line Tools (`clang` 17.0.0, GNU Make 3.81, `file`, `objdump`,
  Python 3.9, git 2.39). No additional host packages are needed for the make builds or the
  host tests.
- The LLVM-MinGW macOS archive is a universal binary (x86_64 + arm64) and runs natively on
  Intel Macs.
- Homebrew: the official installer refuses to run on Intel Macs ("Homebrew on macOS is only
  supported on Apple Silicon processors!"; Homebrew dropped Intel Mac support in 2026), so no
  Homebrew packages are used. MacPorts is the officially suggested alternative on Intel, but
  installing it requires an interactive administrator install. Neither is required: the
  toolchain comes from the official LLVM-MinGW release archive, and optional Python helpers
  (e.g. `pefile`) can be installed into a virtual environment (`python3 -m venv`).

## MSVC (`yact.sln`) Build Requirements

The make build covers `supp.nt.dll`, `x86_emu.dll`, `yact_emu.dll` and `cbacks.nt.dll`. The
remaining runtime components (`dosbox_emu.dll` + `dosbox_emu1..15.dll`, `bochs_emu.dll`,
`peldr.nt.dll`, `cpihook.dll`, `Launcher.exe`, the `*.86.dll` stubs) are built only from
`yact.sln`, which requires a Windows host:

- Visual Studio 2013 Update 5 (platform toolset `v120`, used by all projects)
- Windows SDK 8.0/8.1 with the ARM build tools (the solution contains ARM32 configurations)
- This path was not executed while preparing this guide (no Windows host was available on the
  macOS machine used for verification).

## Windows RT Runtime Requirements

DLLs produced by the LLVM-MinGW `ucrt` toolchain import `ucrtbase.dll` and the
`api-ms-win-crt-*.dll` API sets. On Windows RT 8.0/8.1 these require the Universal C Runtime
update (KB2999226), which Microsoft distributes through Windows Update for Windows RT/RT 8.1.

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
- `FASTCALL` macro redefined (MinGW builds): harmless; `yact_emu.h` redefines the macro to
  `__fastcall`, which matches the MinGW header meaning on x86 and is ignored on ARM.
- Unused-parameter / unused-variable warnings: expected with `-Wall -Wextra`; they do not
  affect the build result.

### Tracked Object Files
- A number of `.o` object files are tracked in git (added by earlier snapshot commits; they are
  already covered by `.gitignore`). Any make build overwrites them, so `git status` shows them
  as modified afterwards. They are build artifacts: run `make clean` before building, and
  `git restore '*.o'` to drop the local changes.

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
    release/yact_emu.dll \
    release/cbacks.nt.dll
```

This only covers the files produced by the make build. A runnable package additionally needs the
engine DLLs built from `yact.sln` (`dosbox_emu.dll` + `dosbox_emu1..15.dll`, `bochs_emu.dll`),
and every DLL must keep its original file name: the front end looks the engine
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
