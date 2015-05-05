#include "config.h"
#include "proc.h"

#if OS & OS_WIN32
#include "proc_win32.c"
#endif

#if OS & OS_UNIX
#include "proc_unix.c"
#endif

// shorthand for proc_wait(proc_start()).  returns exit status
int
proc_run(char *argv0, char *cmdline, char *env) {
    procid_t pid;
    int i, status;

    pid = proc_start(argv0, cmdline, env);
    if( !pid ) return -1;
    i = proc_wait(pid, &status, -1);
    if( i ) return -2;
    return status;
}

