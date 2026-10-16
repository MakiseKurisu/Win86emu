#!/bin/bash
# Win86emun Build Verification Script
# Run from the repository root with LLVM-MinGW in PATH
#
# Usage: ./verify_build.sh [x86|arm|all]
#   x86 - build x86 DLLs (core + stubs)
#   arm - build ARM32 DLLs (core + native + dosbox-slots)
#   all - both (default)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for LLVM-MinGW
if ! command -v armv7-w64-mingw32-g++ &> /dev/null; then
    echo -e "${RED}Error: LLVM-MinGW not found in PATH${NC}"
    echo "Download from: https://github.com/mstorsjo/llvm-mingw/releases"
    echo "Example: export PATH=/tmp/llvm-mingw-20261006-ucrt-macos-universal/bin:\$PATH"
    exit 1
fi

echo "=== Win86emun Build Verification ==="
echo "LLVM-MinGW: $(armv7-w64-mingw32-g++ --version | head -1)"
echo ""

MODE="${1:-all}"
FAILED=0

if [ "$MODE" = "x86" ] || [ "$MODE" = "all" ]; then
    echo "--- x86 Build ---"
    make -f Makefile.x86 clean
    make -f Makefile.x86 -j4 || FAILED=1
    make -f Makefile.x86 -j4 stubs || FAILED=1
    
    # Verify PE headers
    echo "Verifying x86 PE headers..."
    for dll in release/supp.nt.dll release/x86_emu.dll release/cbacks.nt.dll; do
        machine=$(llvm-readobj --file-headers "$dll" 2>&1 | grep "Machine:" | head -1)
        if echo "$machine" | grep -q "I386"; then
            echo -e "  ${GREEN}OK${NC}: $dll ($machine)"
        else
            echo -e "  ${RED}FAIL${NC}: $dll ($machine)"
            FAILED=1
        fi
    done
    
    # Verify no UCRT in stubs
    echo "Verifying x86 stubs have no UCRT imports..."
    for dll in release/*.86.dll; do
        if llvm-readobj --coff-imports "$dll" 2>&1 | grep -q "api-ms-win-crt"; then
            echo -e "  ${RED}FAIL${NC}: $dll imports UCRT"
            FAILED=1
        fi
    done
    echo -e "  ${GREEN}OK${NC}: No UCRT imports in stub DLLs"
    echo ""
fi

if [ "$MODE" = "arm" ] || [ "$MODE" = "all" ]; then
    echo "--- ARM32 Build ---"
    make -f Makefile.arm clean
    make -f Makefile.arm -j4 || FAILED=1
    make -f Makefile.arm -j4 native || FAILED=1
    make -f Makefile.arm dosbox-slots || FAILED=1
    
    # Verify PE headers
    echo "Verifying ARM32 PE headers..."
    for dll in release/supp.nt.dll release/x86_emu.dll release/cbacks.nt.dll release/peldr.nt.dll release/dosbox_emu.dll; do
        machine=$(llvm-readobj --file-headers "$dll" 2>&1 | grep "Machine:" | head -1)
        if echo "$machine" | grep -q "ARMNT"; then
            echo -e "  ${GREEN}OK${NC}: $dll ($machine)"
        else
            echo -e "  ${RED}FAIL${NC}: $dll ($machine)"
            FAILED=1
        fi
    done
    echo ""
fi

# Run host tests
echo "--- Host Tests ---"
python3 yact/supp/test_logging_host.py || FAILED=1
python3 yact/yact_emu/test_isa_host.py || FAILED=1
python3 yact/cbacks/test_callback_arity.py || FAILED=1
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}=== ALL CHECKS PASSED ===${NC}"
    exit 0
else
    echo -e "${RED}=== SOME CHECKS FAILED ===${NC}"
    exit 1
fi
