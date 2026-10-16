# Win86emun Project Classification

## Summary

| Category | Count | Description |
|----------|-------|-------------|
| Core engine | 8 | supp, x86_emu, yact_emu, cbacks, peldr, cpihook, dosbox_emu, bochs_emu |
| Stub DLLs | 74 | *.86.dll stubs for emulated x86 environment |
| System DLLs | 72 | *.nt.dll replacements for ARM32 |
| Other tools | 17 | Launcher, test tools, utilities |
| **Total** | **171** | |

## Build Status (verified 2026-10-16, macOS 15.8.1, LLVM-MinGW 20261006)

### A: Builds with LLVM-MinGW (verified)

| Project | Output | Arch | Method |
|---------|--------|------|--------|
| supp | supp.nt.dll | ARM32 | `make -f Makefile.arm` |
| x86_emu | x86_emu.dll | ARM32 | `make -f Makefile.arm` |
| yact_emu | yact_emu.dll | ARM32 | `make -f Makefile.arm` |
| cbacks | cbacks.nt.dll | ARM32 | `make -f Makefile.arm` |
| peldr | peldr.nt.dll | ARM32 | `make -f Makefile.arm native` |
| cpihook | cpihook.nt.dll | ARM32 | `make -f Makefile.arm native` |
| dosbox_emu | dosbox_emu.dll + 15 copies | ARM32 | `make -f Makefile.arm native dosbox-slots` |
| bochs_emu | bochs_emu.dll | ARM32 | `make -f Makefile.arm native` |
| 72 system_dlls | *.nt.dll | ARM32 | `make -f Makefile.arm native` |
| supp (x86) | supp.nt.dll | x86 | `make -f Makefile.x86` |
| x86_emu (x86) | x86_emu.dll | x86 | `make -f Makefile.x86` |
| yact_emu (x86) | yact_emu.dll | x86 | `make -f Makefile.x86` |
| cbacks (x86) | cbacks.nt.dll | x86 | `make -f Makefile.x86` |
| supp.86 | supp.86.dll | x86 | `make -f Makefile.x86 stubs` |
| 74 stub_dlls | *.86.dll | x86 | `make -f Makefile.x86 stubs` |

### B: Requires minimal changes (not verified)

None identified - all make-buildable projects fall into category A.

### C: Missing Windows SDK/CRT/imports (not buildable on macOS)

| Project | Missing | Notes |
|---------|---------|-------|
| Launcher | MFC | Requires Visual Studio with MFC |
| test_seh | MSVC inline asm | Uses naked functions with __asm |
| imphook | x86 peldr | clang i686 SEH fails on peldr.cpp |
| yactemu_test | x86 peldr | Same as above |

### D: MSVC-specific code changes needed

None identified for the make-buildable projects.

### E: Unresolved

None for the make-buildable projects.

### U: Not investigated

| Project | Notes |
|---------|-------|
| AutoHook | Not part of make build |
| EXE_Stub | Not part of make build |
| MakeLinks | Utility, not part of make build |
| cpi_test | Test tool, not part of make build |
| dll_stub_gen | Code generator, not part of make build |
| dll_yact_gen | Code generator, not part of make build |
| libemu_test | Test tool, not part of make build |
| peldr_test | Test tool, not part of make build |
| peloaderc | Test tool, not part of make build |
| supp_stub | Not part of make build |
| thunk86 | Test tool, not part of make build |
| thunk86c | Test tool, not part of make build |
| ShellIntf | Not part of make build |

## PlatformToolset Distribution

- v120 (Visual Studio 2013): 650 configurations
- v110 (Visual Studio 2010): 14 configurations

## ARM32 Configurations

161 projects have ARM32 configurations in the solution.
