#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#define OS_UNIX  (1<<0)
#define OS_WIN32 (1<<1)

#ifndef OS 
#ifdef _WIN32
#define OS OS_WIN32
#else
#define OS OS_UNIX
#endif // WIN32
#endif // OS

#if OS == OS_WIN32
#include "os_win32.h"
#endif

#if OS == OS_UNIX
#include "os_unix.h"
#endif


#endif // CONFIG_H_INCLUDED
