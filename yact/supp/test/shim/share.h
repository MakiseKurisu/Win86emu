// share.h shim for the host build of yact/supp/logging.cpp (see shim/windows.h).

#pragma once

#ifndef _WIN32

// Windows sharing modes; POSIX open() has no equivalent and the shim ignores
// the value (logging.cpp only uses SH_DENYNO).
#define SH_DENYNO 0x40
#define SH_DENYRD 0x30
#define SH_DENYWR 0x20
#define SH_DENYRW 0x10

#endif // _WIN32
