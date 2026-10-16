#!/usr/bin/env python3
"""Host-side regression tests for the supp.nt.dll logger (yact/supp/logging.cpp).

The real logger implementation is compiled unmodified against a small Windows
API shim (yact/supp/test/shim) and executed on the host. One process is started
per scenario, because the logger caches the level and the file path for the
process lifetime.

Usage: python3 yact/supp/test_logging_host.py
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TEST = os.path.join(HERE, "test")
SHIM = os.path.join(TEST, "shim")

MODES = [
    ["basic"],
    ["levels", "0"],
    ["levels", "1"],
    ["levels", "2"],
    ["levels", "3"],
    ["levels", "4"],
    ["levels", "5"],
    ["threads"],
    ["fallback"],
    ["rotation"],
    ["preinit"],
    ["defaultpath"],
    ["exception"],
    ["compat"],
]


def build(out_dir):
    exe = os.path.join(out_dir, "logging_host_test")
    cmd = [
        os.environ.get("CXX", "clang++"),
        "-std=c++17",
        "-O1",
        "-g",
        "-Wall",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-D__declspec(x)=",
        "-I", TEST,
        "-I", SHIM,
        "-I", os.path.join(ROOT, "yact", "inc"),
        os.path.join(HERE, "logging.cpp"),
        os.path.join(TEST, "logging_host_test.cpp"),
        os.path.join(SHIM, "shim_state.cpp"),
        "-o", exe,
        "-pthread",
    ]
    print("compile: " + " ".join(cmd))
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.stdout:
        print(proc.stdout)
    if proc.returncode != 0:
        print(proc.stderr)
        raise RuntimeError("compile failed")
    if proc.stderr:
        print(proc.stderr)
    return exe


def supp_syntax_check():
    """Compile-check supp.cpp (registry/util part of supp.nt.dll) against the shim."""
    cmd = [
        os.environ.get("CXX", "clang++"),
        "-std=c++17",
        "-fsyntax-only",
        "-Wall",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-D__declspec(x)=",
        "-I", TEST,
        "-I", SHIM,
        "-I", os.path.join(ROOT, "yact", "inc"),
        os.path.join(HERE, "supp.cpp"),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode == 0:
        print("PASS supp/compile-check")
        return 1, 0
    print("FAIL supp/compile-check")
    print(proc.stderr[:4000])
    return 1, 1


def main():
    out_dir = tempfile.mkdtemp(prefix="win86emu_logtest_build_")
    try:
        try:
            exe = build(out_dir)
        except RuntimeError:
            print("1 checks, 1 failures")
            print("FAIL: yact/supp logger host tests (compile)")
            return 1

        checks = 0
        failures = 0

        c, f = supp_syntax_check()
        checks += c
        failures += f

        for mode in MODES:
            proc = subprocess.run([exe] + mode, capture_output=True, text=True)
            for line in proc.stdout.splitlines():
                if line.startswith("PASS "):
                    checks += 1
                elif line.startswith("FAIL "):
                    checks += 1
                    failures += 1
                    print(line)
            if proc.returncode not in (0, 1):
                failures += 1
                print("mode %s crashed (rc=%d):\n%s" % (mode, proc.returncode, proc.stderr[-2000:]))
            if proc.stderr and os.environ.get("SHIM_ECHO_DEBUG"):
                print(proc.stderr)

        print("%d checks, %d failures" % (checks, failures))
        if failures:
            print("FAIL: yact/supp logger host tests")
            return 1
        print("OK: yact/supp logger host tests passed")
        return 0
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
