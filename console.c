#include "bklib/config.h"
#include "bklib/console.h"

#if OS == OS_UNIX
#include "console_unix.c"
#endif

#if OS == OS_WIN32
#include "console_win32.c"
#endif

