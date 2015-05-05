#include "config.h"
#include "shell.h"

#if OS & OS_WIN32
#include "shell_win32.c"
#endif


// under unix, proc_shellex is defined in proc_unix.c

