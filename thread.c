#include "configbk.h"
#include "thread.h"

#if OS == OS_UNIX
#include "thread_unix.c"
#endif

#if OS == OS_WIN32
#include "thread_win32.c"
#endif
