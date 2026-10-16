// io.h shim for the host build of yact/supp/logging.cpp (see shim/windows.h).

#pragma once

#ifndef _WIN32

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>

#ifndef errno_t
typedef int errno_t;
#endif

// CRT file flags used by logging.cpp, mapped to POSIX.
#ifndef _O_RDWR
#define _O_RDWR O_RDWR
#endif
#ifndef _O_APPEND
#define _O_APPEND O_APPEND
#endif
#ifndef _O_CREAT
#define _O_CREAT O_CREAT
#endif
#ifndef _S_IREAD
#define _S_IREAD 0400
#endif
#ifndef _S_IWRITE
#define _S_IWRITE 0200
#endif
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t) -1)
#endif

// Bounded formatting. _TRUNCATE is ignored: vsnprintf/snprintf truncate and
// report the would-be length, which logging.cpp detects as truncation too.
#define _snprintf_s(Buffer, Size, Count, ...) snprintf((Buffer), (Size), __VA_ARGS__)
#define _vsnprintf_s(Buffer, Size, Count, Format, Args) vsnprintf((Buffer), (Size), (Format), (Args))

errno_t _sopen_s(int *File, const char *Path, int Flags, int Share, int Mode);
int _write(int File, const void *Buffer, unsigned int Count);
int _commit(int File);
int _close(int File);
long long _lseeki64(int File, long long Offset, int Whence);

#endif // _WIN32
