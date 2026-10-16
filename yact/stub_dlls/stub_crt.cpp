// stub_crt.cpp - minimal freestanding CRT support for the x86 stub DLLs.
//
// The MSVC build of the stub DLLs uses the static CRT (MultiThreaded), so the
// DLLs carry their own CRT code and import no CRT DLLs.  The emulated x86
// environment only contains the *.86 stub DLLs, so any api-ms-win-crt-*.dll or
// msvcrt.dll import would fail to resolve at load time (PeLdrProcessModuleImports
// fails the whole module load when an import DLL cannot be found).
//
// LLVM-MinGW is UCRT-only (no static UCRT), therefore the MinGW build of the
// stubs links with -nostartfiles and this file supplies the few CRT entry
// points the stub sources actually need:
//   - memcpy        (kernel32_impl86.cpp)
//   - memset        (dinput_common.cpp)
//   - sprintf       (ole32_impl86.cpp: %08x/%04x/%02x formatting)
//   - operator new/delete (ddraw_impl86.cpp: CFactory allocation)
//
// This file is NOT part of any MSVC vcxproj and must not be added to one:
// the MSVC build gets these symbols from the static CRT.
//
#if defined(_MSC_VER)
#error "stub_crt.cpp is for the MinGW/clang Makefile build only (MSVC uses the static CRT)"
#endif

#include <windows.h>
#include <stddef.h>
#include <stdarg.h>

// ---------------------------------------------------------------------------
// DLL entry point (replaces crt2.o's DllMainCRTStartup, which -nostartfiles
// drops together with the UCRT-dependent static initializer machinery).
// The stub DLLs have no static initializers, so going straight to DllMain is
// safe.
// ---------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved);

extern "C" BOOL WINAPI DllMainCRTStartup(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    return DllMain(hinst, reason, reserved);
}

// ---------------------------------------------------------------------------
// memcpy
// ---------------------------------------------------------------------------
extern "C" void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *) dst;
    const unsigned char *s = (const unsigned char *) src;
    while (n--)
        *d++ = *s++;
    return dst;
}

// ---------------------------------------------------------------------------
// memset
// ---------------------------------------------------------------------------
extern "C" void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *) dst;
    while (n--)
        *d++ = (unsigned char) c;
    return dst;
}

// ---------------------------------------------------------------------------
// minimal sprintf: flags '0'/'-'/' ' , width, '.'precision (ignored), length
// modifiers l/ll/h/hh/z (parsed, treated as long), conversions
// d i u x X o p c s %.
// Enough for the stub sources (CLSID formatting in ole32_impl86.cpp).
// ---------------------------------------------------------------------------
static char *fmt_unsigned(char *p, unsigned long v, unsigned base, int upper,
                          int width, int zero, int prec, char sign)
{
    char tmp[32];
    int n = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (v == 0 && prec == 0)
        tmp[n++] = '0';
    while (v)
    {
        tmp[n++] = digits[v % base];
        v /= base;
    }
    while (n < prec)
        tmp[n++] = '0';
    if (sign)
        tmp[n++] = sign;                    // '-' or '+' or ' '
    while (!zero && n < width)
        tmp[n++] = ' ';
    while (n < width)
        tmp[n++] = '0';
    while (n)
        *p++ = tmp[--n];
    return p;
}

extern "C" int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    char *p = buf;

    va_start(ap, fmt);
    while (*fmt)
    {
        if (*fmt != '%')
        {
            *p++ = *fmt++;
            continue;
        }
        fmt++;
        if (*fmt == '%')
        {
            *p++ = *fmt++;
            continue;
        }
        int zero = 0, width = 0, prec = -1;
        char sign = 0;
        if (*fmt == '0') { zero = 1; fmt++; }
        else if (*fmt == '-') { fmt++; }
        else if (*fmt == '+' || *fmt == ' ') { sign = *fmt++; }
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');
        if (*fmt == '.')
        {
            fmt++;
            prec = 0;
            while (*fmt >= '0' && *fmt <= '9')
                prec = prec * 10 + (*fmt++ - '0');
        }
        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 'I' || *fmt == 'j' || *fmt == 't')
            fmt++;
        switch (*fmt)
        {
        case 'd': case 'i':
        {
            long v = va_arg(ap, long);
            unsigned long u = v < 0 ? (unsigned long)(-v) : (unsigned long)v;
            p = fmt_unsigned(p, u, 10, 0, width, zero, prec, v < 0 ? '-' : sign);
            break;
        }
        case 'u':
            p = fmt_unsigned(p, va_arg(ap, unsigned long), 10, 0, width, zero, prec, sign);
            break;
        case 'x':
            p = fmt_unsigned(p, va_arg(ap, unsigned long), 16, 0, width, zero, prec, sign);
            break;
        case 'X':
            p = fmt_unsigned(p, va_arg(ap, unsigned long), 16, 1, width, zero, prec, sign);
            break;
        case 'o':
            p = fmt_unsigned(p, va_arg(ap, unsigned long), 8, 0, width, zero, prec, sign);
            break;
        case 'p':
        {
            *p++ = '0'; *p++ = 'x';
            p = fmt_unsigned(p, (unsigned long)(size_t) va_arg(ap, void *), 16, 0, width, 1, prec, 0);
            break;
        }
        case 'c':
            *p++ = (char) va_arg(ap, int);
            break;
        case 's':
        {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            while (*s)
                *p++ = *s++;
            break;
        }
        default:
            *p++ = '%';
            if (*fmt)
                *p++ = *fmt;
            break;
        }
        if (*fmt)
            fmt++;
    }
    va_end(ap);
    *p = 0;
    return (int)(p - buf);
}

// ---------------------------------------------------------------------------
// C++ operator new/delete backed by the Win32 process heap (a kernel32 import
// that resolves through kernel32.86.dll in the emulated environment).
// ---------------------------------------------------------------------------
void *operator new(size_t n) { return HeapAlloc(GetProcessHeap(), 0, n ? n : 1); }
void *operator new[](size_t n) { return HeapAlloc(GetProcessHeap(), 0, n ? n : 1); }
void operator delete(void *p) { if (p) HeapFree(GetProcessHeap(), 0, p); }
void operator delete[](void *p) { if (p) HeapFree(GetProcessHeap(), 0, p); }
