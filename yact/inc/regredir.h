/*
 * Yet Another Code Translator (c) mamaich, 2011
 *
 * x86 registry redirection function
 *
 */

#ifndef __EMUL_H
#define __EMUL_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UTIL_EXPORT
#define UTIL_EXPORT
#endif

    UTIL_EXPORT HKEY RegRedirect(HKEY Reg);

#ifdef __cplusplus
}
#endif

#endif // __EMUL_H
