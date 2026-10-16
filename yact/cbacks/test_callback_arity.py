#!/usr/bin/env python3
# Regression test for native -> x86 callback argument-count handling.
#
# This is a host-side (macOS/Linux) *static* regression test. It does not run the
# emulator; it parses the real sources and enforces the invariants that the
# cbacks.nt.dll / x86_emu.dll bridge relies on:
#
#   1. CbNP() must forward exactly N parameters to EmuExecute (N == callback arity).
#   2. The CbFuncs[] dispatch table must map index N to CbNP in order.
#   3. CbCreateNativeStdcallCallback() must reject NumParams >= table size
#      (indices 0..8 are valid, so NumParams==9 must be rejected).
#   4. x86_emu.cpp EmuExecute() must accept the same maximum parameter count
#      (cases 0..8).
#
# Exit code 0 == all invariants hold, 1 == a regression was found.

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, os.pardir, os.pardir))
CALLBACKS = os.path.join(ROOT, "yact", "cbacks", "callbacks.cpp")
X86_EMU = os.path.join(ROOT, "yact", "x86_emu", "x86_emu.cpp")
DOSBOX = os.path.join(ROOT, "yact", "dosbox_emu", "dosbox_emu.cpp")
BOCHS = os.path.join(ROOT, "yact", "bochs_emu", "bochs_test.cpp")

failures = []


def fail(msg):
    failures.append(msg)


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def _extract_function_body(src, name):
    """Return the brace-matched body of a function definition, or None."""
    m = re.search(re.escape(name) + r"\s*\([^;{]*\)\s*\{", src)
    if not m:
        return None
    start = m.end()
    depth = 1
    i = start
    while i < len(src) and depth > 0:
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
        i += 1
    return src[start:i - 1]


def check_cb_forwarding(src):
    # Match "static DWORD CbNP(void* Param, DW *R)" ... "EmuExecute((DWORD) Param, X,"
    pattern = re.compile(
        r"static\s+DWORD\s+Cb(\d+)P\s*\([^)]*\)\s*\{(.*?)\}",
        re.DOTALL,
    )
    found = {}
    for m in pattern.finditer(src):
        n = int(m.group(1))
        body = m.group(2)
        call = re.search(r"EmuExecute\s*\(\s*\(DWORD\)\s*Param\s*,\s*(\d+)\s*[,)]", body)
        if not call:
            fail("Cb%dP: could not find EmuExecute(...) call" % n)
            continue
        found[n] = int(call.group(1))
    for n in range(0, 9):
        if n not in found:
            fail("Cb%dP: function not found" % n)
        elif found[n] != n:
            fail("Cb%dP forwards NParams=%d to EmuExecute (expected %d)" % (n, found[n], n))
    return found


def check_dispatch_table(src):
    m = re.search(r"CbFuncs\s*\[\s*\]\s*=\s*\{(.*?)\}", src, re.DOTALL)
    if not m:
        fail("CbFuncs[] table not found")
        return
    names = re.findall(r"Cb(\d+)P", m.group(1))
    names = [int(x) for x in names]
    if names != list(range(0, 9)):
        fail("CbFuncs[] order is %s, expected 0..8" % names)


def check_bounds_guard(src):
    # The guard must reject NumParams == sizeof(CbFuncs)/sizeof(CbFuncs[0]) (== 9).
    m = re.search(r"if\s*\((.*?)\)\s*\n\s*return 0;", src)
    guard = None
    for gm in re.finditer(r"if\s*\(([^)]*NumParams[^)]*)\)", src):
        guard = gm.group(1)
    if guard is None:
        fail("bounds guard for NumParams not found")
        return
    if ">=" not in guard:
        fail("bounds guard '%s' allows NumParams==9 (index out of range); use '>='" % guard)


def check_emu_execute_cases(src):
    cases = set(int(x) for x in re.findall(r"case\s+(\d+)\s*:", src))
    for n in range(0, 9):
        if n not in cases:
            fail("x86_emu.cpp EmuExecute(): missing 'case %d:'" % n)


def check_engine_supports_nparams(src, label):
    # Both CPU engines push args in a loop driven by NParams, so NParams up to 8
    # (used by 6/7/8-argument callbacks) is safe. Guard that assumption.
    if not re.search(r"for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*NParams\s*;\s*i\+\+\s*\)", src):
        fail("%s: EmuExecute() does not loop over NParams" % label)
    if "va_arg(args,int)" not in src.replace(" ", ""):
        fail("%s: EmuExecute() does not read args via va_arg" % label)


def _split_top_level(s):
    """Split a comma-separated argument list at top-level (paren-depth aware)."""
    parts = []
    depth = 0
    cur = ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def check_all_emuexecute_calls():
    """Every literal EmuExecute(addr, N, a1..aN) call must pass exactly N args."""
    exts = (".cpp", ".h")
    for dirpath, dirnames, filenames in os.walk(ROOT):
        if ".git" in dirpath.split(os.sep):
            continue
        for fn in filenames:
            if not fn.endswith(exts):
                continue
            path = os.path.join(dirpath, fn)
            try:
                src = read(path)
            except OSError:
                continue
            for m in re.finditer(r"\bEmuExecute\s*\(", src):
                start = m.end()
                depth = 1
                i = start
                while i < len(src) and depth > 0:
                    if src[i] == "(":
                        depth += 1
                    elif src[i] == ")":
                        depth -= 1
                    i += 1
                inner = src[start:i - 1]
                args = _split_top_level(inner)
                if len(args) < 2:
                    continue
                nparam = args[1]
                if not re.fullmatch(r"\d+", nparam):
                    continue
                n = int(nparam)
                passed = len(args) - 2
                if passed != n:
                    rel = os.path.relpath(path, ROOT)
                    fail("%s: EmuExecute(%s, %d, ...) passes %d argument(s)"
                         % (rel, args[0], n, passed))


# Callback typedefs whose arity was confirmed against Microsoft API docs during
# the audit. Maps typedef -> (arity, [APIs that register a callback of that type]).
# This guards against a registration passing a wrong ncbparams (which would make
# the native trampoline disagree with the API's callback signature, and would also
# break pointer-identity lookups such as UnhookWindowsHook vs SetWindowsHookA).
CALLBACK_TYPEDEF_ARITY = {
    "HOOKPROC": (3, ["SetWindowsHookA", "SetWindowsHookW",
                     "SetWindowsHookExA", "SetWindowsHookExW",
                     "UnhookWindowsHook"]),
    "SUBCLASSPROC": (6, ["SetWindowSubclass", "GetWindowSubclass",
                         "RemoveWindowSubclass"]),
    "WINEVENTPROC": (7, ["SetWinEventHook"]),
    "LPCONDITIONPROC": (8, ["WSAAccept"]),
    "PFNCALLBACK": (8, ["DdeInitializeA", "DdeInitializeW"]),
}

DEFINE_FUNC_CB_RE = re.compile(
    r"DEFINE_FUNC_CB(\d+)\s*\(\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)")


def parse_cb_entries():
    """(relpath, api, nargs, cbpos, ncbparams) for every DEFINE_FUNC_CB* usage."""
    entries = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        if ".git" in dirpath.split(os.sep):
            continue
        for fn in filenames:
            if not fn.endswith((".cpp", ".h")):
                continue
            path = os.path.join(dirpath, fn)
            if os.path.basename(path) == "dllyact_def.h":
                continue
            try:
                src = read(path)
            except OSError:
                continue
            rel = os.path.relpath(path, ROOT)
            for m in DEFINE_FUNC_CB_RE.finditer(src):
                entries.append((rel, m.group(2), int(m.group(1)),
                                int(m.group(3)), int(m.group(4))))
    return entries


def check_cb_ranges(entries):
    for rel, api, nargs, cbpos, ncbparams in entries:
        if not (1 <= cbpos <= nargs):
            fail("%s: DEFINE_FUNC_CB%d(%s,%d,%d): cbpos out of range (1..%d)"
                 % (rel, nargs, api, cbpos, ncbparams, nargs))
        if not (0 <= ncbparams <= 8):
            fail("%s: DEFINE_FUNC_CB%d(%s,%d,%d): ncbparams must be 0..8 "
                 "(CbFuncs[] holds Cb0P..Cb8P)"
                 % (rel, nargs, api, cbpos, ncbparams))


def check_typedef_arity(entries):
    by_api = {}
    for rel, api, nargs, cbpos, ncbparams in entries:
        by_api.setdefault(api, []).append((rel, ncbparams))
    for typedef, (arity, apis) in CALLBACK_TYPEDEF_ARITY.items():
        for api in apis:
            for rel, ncbparams in by_api.get(api, []):
                if ncbparams != arity:
                    fail("%s: %s registers ncbparams=%d but its callback (%s) "
                         "takes %d parameter(s)"
                         % (rel, api, ncbparams, typedef, arity))


# Boundary values used to exercise the EmuExecute() NParams guard.
GUARD_ACCEPT = (0, 1, 7, 8)
GUARD_REJECT = (9, -1, -2147483648, 2147483647)


def _guard_rejects(guard, nparams, maxargs):
    """Evaluate a C guard expression for one NParams value (True == rejected)."""
    expr = guard.replace("NParams", str(nparams))
    expr = re.sub(r"\bMAXARGS\b", str(maxargs), expr)
    expr = expr.replace("||", " or ").replace("&&", " and ")
    expr = re.sub(r"(?<![<>=!])!(?!=)", " not ", expr)
    return bool(eval(expr, {"__builtins__": {}}, {}))


def check_emuexecute_guard(src):
    """EmuExecute() must reject NParams<0 and NParams>MAXARGS before Tmp[] writes."""
    body = _extract_function_body(src, "EmuExecute")
    if body is None:
        fail("x86_emu.cpp: EmuExecute() definition not found")
        return

    mm = re.search(r"#define\s+MAXARGS\s+(\d+)", body)
    if not mm:
        fail("x86_emu.cpp: MAXARGS is not defined inside EmuExecute()")
        return
    maxargs = int(mm.group(1))
    if maxargs != 8:
        fail("x86_emu.cpp: MAXARGS=%d, expected 8" % maxargs)

    if not re.search(r"DWORD\s+Tmp\s*\[\s*MAXARGS\s*\]", body):
        fail("x86_emu.cpp: Tmp[MAXARGS] declaration not found")

    va = body.find("va_start")
    loop = re.search(
        r"for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*NParams\s*;\s*i\+\+\s*\)"
        r"\s*Tmp\[i\]\s*=\s*va_arg", body)
    if va < 0:
        fail("x86_emu.cpp: va_start not found in EmuExecute()")
        return
    if not loop:
        fail("x86_emu.cpp: Tmp[i] = va_arg(...) copy loop not found")
        return

    guard_m = re.search(r"if\s*\(([^)]*NParams[^)]*)\)", body)
    if guard_m is None:
        fail("x86_emu.cpp: no NParams range check found; the copy loop can write "
             "past Tmp[] for NParams>%d" % maxargs)
        return
    if guard_m.start() > va or guard_m.start() > loop.start():
        fail("x86_emu.cpp: NParams range check runs AFTER va_start/Tmp[] copy; "
             "out-of-range NParams overflows Tmp[] first")
        return

    guard = guard_m.group(1)
    for n in GUARD_ACCEPT:
        if _guard_rejects(guard, n, maxargs):
            fail("x86_emu.cpp: guard '%s' rejects valid NParams=%d" % (guard, n))
    for n in GUARD_REJECT:
        if not _guard_rejects(guard, n, maxargs):
            fail("x86_emu.cpp: guard '%s' accepts out-of-range NParams=%d"
                 % (guard, n))


def check_cbex_no_array_index(src):
    """CbCreateNativeStdcallCallbackEx() must not index any array by NumParams."""
    body = _extract_function_body(src, "CbCreateNativeStdcallCallbackEx")
    if body is None:
        fail("callbacks.cpp: CbCreateNativeStdcallCallbackEx() definition not found")
        return
    if re.search(r"\[\s*NumParams\s*\]", body):
        fail("callbacks.cpp: CbCreateNativeStdcallCallbackEx() indexes an array by "
             "NumParams (unvalidated input could corrupt a buffer)")


def _emu_maxargs(x86_src):
    m = re.search(r"#define\s+MAXARGS\s+(\d+)", x86_src)
    return int(m.group(1)) if m else None


def check_max_params_claims(x86_src):
    """No built source may document a parameter limit above MAXARGS.

    This is the contract behind EmuExecute(): the upper interface accepts
    0..MAXARGS. A stale comment claiming a higher limit (e.g. "max 100 params")
    misrepresents that contract. Unbuilt historical backups (bak/) are excluded
    because they are not referenced by any vcxproj/Makefile.
    """
    maxargs = _emu_maxargs(x86_src)
    if maxargs is None:
        fail("x86_emu.cpp: MAXARGS not defined; cannot pin the parameter contract")
        return
    for dirpath, dirnames, filenames in os.walk(ROOT):
        if ".git" in dirpath.split(os.sep):
            continue
        dirnames[:] = [d for d in dirnames if d != "bak"]
        for fn in filenames:
            if not fn.endswith((".cpp", ".h")):
                continue
            path = os.path.join(dirpath, fn)
            try:
                src = read(path)
            except OSError:
                continue
            rel = os.path.relpath(path, ROOT)
            for m in re.finditer(r"max\s+(\d+)\s*params", src, re.I):
                n = int(m.group(1))
                if n > maxargs:
                    fail("%s: claims 'max %d params' but EmuExecute's contract is "
                         "0..%d (MAXARGS)" % (rel, n, maxargs))


def check_emuexecute_call_bounds(x86_src):
    """Every EmuExecute() caller must pass a literal NParams within 0..MAXARGS.

    A literal argument is statically provable; a dynamic NParams would bypass
    the upper interface's contract and must be reviewed instead of assumed.
    """
    maxargs = _emu_maxargs(x86_src)
    if maxargs is None:
        fail("x86_emu.cpp: MAXARGS not defined; cannot pin the call-site contract")
        return
    for dirpath, dirnames, filenames in os.walk(ROOT):
        if ".git" in dirpath.split(os.sep):
            continue
        for fn in filenames:
            if not fn.endswith((".cpp", ".h")):
                continue
            path = os.path.join(dirpath, fn)
            try:
                src = read(path)
            except OSError:
                continue
            rel = os.path.relpath(path, ROOT)
            for m in re.finditer(r"\bEmuExecute\s*\(", src):
                start = m.end()
                depth = 1
                i = start
                while i < len(src) and depth > 0:
                    if src[i] == "(":
                        depth += 1
                    elif src[i] == ")":
                        depth -= 1
                    i += 1
                inner = src[start:i - 1]
                if "..." in inner:
                    continue  # declaration / definition, not a call
                args = _split_top_level(inner)
                if len(args) < 2:
                    continue
                nparam = args[1]
                if re.search(r"\b(int|void|DWORD)\b", nparam):
                    continue  # declaration
                if not re.fullmatch(r"\d+", nparam):
                    fail("%s: EmuExecute(..., %s, ...) does not pass a literal "
                         "NParams; the 0..%d contract cannot be proven"
                         % (rel, nparam.strip(), maxargs))
                    continue
                n = int(nparam)
                if n > maxargs:
                    fail("%s: EmuExecute(..., %d, ...) exceeds MAXARGS=%d"
                         % (rel, n, maxargs))


def main():
    if not os.path.isfile(CALLBACKS):
        fail("missing %s" % CALLBACKS)
    if not os.path.isfile(X86_EMU):
        fail("missing %s" % X86_EMU)
    if failures:
        for f in failures:
            print("ERROR:", f)
        return 1

    cb_src = read(CALLBACKS)
    check_cb_forwarding(cb_src)
    check_dispatch_table(cb_src)
    check_bounds_guard(cb_src)
    check_cbex_no_array_index(cb_src)

    x86_src = read(X86_EMU)
    check_emu_execute_cases(x86_src)
    check_emuexecute_guard(x86_src)

    if os.path.isfile(DOSBOX):
        check_engine_supports_nparams(read(DOSBOX), "dosbox_emu.cpp")
    else:
        fail("missing %s" % DOSBOX)
    if os.path.isfile(BOCHS):
        check_engine_supports_nparams(read(BOCHS), "bochs_test.cpp")
    else:
        fail("missing %s" % BOCHS)

    check_all_emuexecute_calls()
    check_emuexecute_call_bounds(x86_src)
    check_max_params_claims(x86_src)

    maxargs = _emu_maxargs(x86_src)
    if maxargs is None:
        maxargs = 8


    cb_entries = parse_cb_entries()
    if not cb_entries:
        fail("no DEFINE_FUNC_CB* usages found")
    check_cb_ranges(cb_entries)
    check_typedef_arity(cb_entries)

    if failures:
        for f in failures:
            print("FAIL:", f)
        print("\n%d invariant(s) violated" % len(failures))
        return 1

    print("OK: callback arity invariants hold (Cb0P..Cb8P, table, bounds guard, "
          "EmuExecute cases 0..8 + NParams guard, call-sites 0..%d, no overstated "
          "param-limit comments, %d DEFINE_FUNC_CB* registrations)"
          % (maxargs, len(cb_entries)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
