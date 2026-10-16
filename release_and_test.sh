#!/bin/bash
# Win86emun Build and Release Script
# Builds both x86 and ARM32 versions and prepares for GitHub release

set -e

WORKSPACE="/workspace"
RELEASE_DIR="$WORKSPACE/release"
LLVM_MINGW_PATH="/tmp/llvm-mingw-20260826-ucrt-ubuntu-22.04-x86_64/bin"

export PATH="$LLVM_MINGW_PATH:$PATH"

echo "=== Win86emun Build Script ==="
echo ""

# Clean release directory
echo "Cleaning release directory..."
rm -f "$RELEASE_DIR"/*.dll

# Build x86 version
echo ""
echo "=== Building x86 (32-bit Windows) version ==="
cd "$WORKSPACE"
cp Makefile.x86 Makefile
make clean
make -j$(nproc)

# Rename x86 DLLs
cd "$RELEASE_DIR"
mv yact_emu.dll yact_emu_x86.dll 2>/dev/null || true
mv x86_emu.dll x86_emu_x86.dll 2>/dev/null || true
mv supp.nt.dll supp_x86.dll 2>/dev/null || true
echo "✓ x86 build complete"

# Build ARM32 version
echo ""
echo "=== Building ARM32 (Windows RT) version ==="
cd "$WORKSPACE"
cp Makefile.arm Makefile
make clean
make -j$(nproc)

# Rename ARM DLLs
cd "$RELEASE_DIR"
mv yact_emu.dll yact_emu_arm.dll 2>/dev/null || true
mv x86_emu.dll x86_emu_arm.dll 2>/dev/null || true
mv supp.nt.dll supp_arm.dll 2>/dev/null || true
echo "✓ ARM32 build complete"

# Verify builds
echo ""
echo "=== Verifying builds ==="
echo ""
echo "x86 DLLs:"
i686-w64-mingw32-objdump -f "$RELEASE_DIR"/yact_emu_x86.dll | grep "file format"
i686-w64-mingw32-objdump -f "$RELEASE_DIR"/x86_emu_x86.dll | grep "file format"
i686-w64-mingw32-objdump -f "$RELEASE_DIR"/supp_x86.dll | grep "file format"

echo ""
echo "ARM32 DLLs:"
armv7-w64-mingw32-objdump -f "$RELEASE_DIR"/yact_emu_arm.dll | grep "file format"
armv7-w64-mingw32-objdump -f "$RELEASE_DIR"/x86_emu_arm.dll | grep "file format"
armv7-w64-mingw32-objdump -f "$RELEASE_DIR"/supp_arm.dll | grep "file format"

# List final files
echo ""
echo "=== Final Release Files ==="
ls -lh "$RELEASE_DIR"

echo ""
echo "=== Build Complete ==="
echo ""
echo "Next steps:"
echo "1. Test ARM DLLs on a jailbroken Windows RT device"
echo "2. Create GitHub release:"
echo "   git tag -a v1.0.0 -m \"Release v1.0.0\""
echo "   git push origin v1.0.0"
echo "   gh release create v1.0.0 --title \"Win86emu v1.0.0\" --notes \"ARM32 and x86 binaries\" release/*.dll"
