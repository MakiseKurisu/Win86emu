# Win86emun Application Compatibility List

This document tracks the compatibility status of x86 applications running on Windows RT devices using Win86emun.

## Legend

| Symbol | Status | Description |
|--------|--------|-------------|
| ✓ | Works | Application runs without issues |
| ⚠ | Partial | Application runs with minor issues or limitations |
| ✗ | Broken | Application crashes or fails to start |
| ? | Untested | Not yet tested |

## Tested Applications

### Utilities

| Application | Version | Status | Notes |
|-------------|---------|--------|-------|
| 7-Zip | 19.00 | ✓ | Fully functional, compression/extraction works |
| Notepad++ | 7.9.5 | ✓ | Basic editing works, some plugins may fail |
| WinRAR | 5.90 | ⚠ | Opens files, but some operations may crash |
| Total Commander | 9.22 | ✓ | File management works well |
| PuTTY | 0.73 | ✓ | SSH/Telnet client works |
| Notepad2 | 4.21 | ✓ | Lightweight editor, fully functional |

### Development Tools

| Application | Version | Status | Notes |
|-------------|---------|--------|-------|
| Visual Studio 6.0 | - | ✗ | Too complex, many unimplemented APIs |
| Dev-C++ | 5.11 | ⚠ | IDE opens, compilation may fail |
| UltraEdit | 26.00 | ⚠ | Basic editing works, advanced features limited |

### Multimedia

| Application | Version | Status | Notes |
|-------------|---------|--------|-------|
| VLC Media Player | 3.0.10 | ✗ | SSE instructions not fully implemented |
| Foobar2000 | 1.5.6 | ⚠ | Audio playback works, some codecs fail |
| IrfanView | 4.56 | ✓ | Image viewing works well |

### Games

| Application | Version | Status | Notes |
|-------------|---------|--------|-------|
| Doom (Source Port) | - | ? | Not yet tested |
| Classic Win32 games | - | ✗ | Most require DirectX which is not emulated |

## Known Limitations

### Unimplemented Features

1. **SSE Instructions**: Many multimedia applications require SSE instructions, which no engine in this repository implements.
   - The DOSBox-based engine (`dosbox_emu.dll`) decodes the integer and MMX instruction sets, but it has no XMM registers and no SSE opcodes; its CPUID reports only FPU+TSC (no SSE and no MMX feature bit) and an SSE opcode raises an illegal-instruction exception.
   - The Bochs-based engine (`bochs_emu.dll`) is built with `BX_CPU_LEVEL 5` ("be compatible with no SSE in DosBox core!"), which disables SSE and MMX.
   - The experimental engine (`yact_emu.dll`) contains no SSE handlers and is not part of the installed runtime.
   - The x87 FPU is provided by the DOSBox and Bochs engines; MMX is provided by the DOSBox engine only.
   - Not implemented: SSE, SSE2, SSE3, SSSE3 and later instruction sets.

2. **DirectX**: No DirectX emulation support. Games and applications requiring hardware acceleration will not work.

3. **Privileged Instructions**: Applications attempting to execute privileged CPU instructions will fail.

4. **Hardware-Specific APIs**: Applications that directly access hardware will not work.

### Performance Considerations

- Emulation overhead: Expect 10-50x slowdown compared to native x86 execution
- Memory-intensive applications may experience additional slowdowns
- Applications with tight timing loops may not function correctly

## How to Test an Application

1. Install Win86emun on the Windows RT device (the device must be jailbroken to run x86 code).
   The installer places the emulator files in the Windows directory (`%SystemRoot%`), not next to the
   application. The runtime file set is:

   - `Launcher.exe` - starts an emulated x86 application
   - `x86_emu.dll` - emulator front end; it loads the engine DLLs when it is first used
   - `dosbox_emu.dll` and `dosbox_emu1.dll` ... `dosbox_emu15.dll` - the primary engine (16 slots;
     the release build requires all 16 files)
   - `bochs_emu.dll` - the fallback engine used when every DOSBox slot is busy
   - `cbacks.nt.dll` - callback layer between the emulated code and the host
   - `supp.nt.dll` - shared support code (logging, configuration)
   - the `*.86.dll` stub DLLs and the `*.nt.dll` system DLLs

   These names are fixed. `x86_emu.dll` looks the other modules up by name
   (`LoadLibraryA("cbacks.nt.dll")`, `"dosbox_emu.dll"`, `"bochs_emu.dll"`), so renaming the files
   (for example to `x86_emu_arm.dll`) makes the emulator exit during startup. `yact_emu.dll` is an
   experimental engine; it is neither installed nor loaded.

2. Start the application through the installed launcher instead of copying DLLs next to the
   application executable.

3. Run the application and observe its behavior.

4. Check `%TEMP%\win86emu.log` (or the path set in the `LogFile` registry value) for error messages
   if the application fails.

## Reporting Issues

When reporting compatibility issues, please include:

1. Application name and version
2. Exact error message or crash description
3. Relevant excerpt from `win86emu.log`
4. Whether the application works on native x86 Windows

## Contributing

If you've tested an application not on this list, please submit your findings:

- Application name and version
- Compatibility status (✓ / ⚠ / ✗)
- Any relevant notes or workarounds

## References

- Original compatibility discussion: http://forum.xda-developers.com/showthread.php?t=2095934
- Intel SDM for instruction reference: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
