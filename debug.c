#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#include "config.h"
#include "debug.h"
#include "debug_stdio.h"
#include "mstime.h"
#include "proc.h"
#include "dir.h"

#if OS == OS_UNIX
#include "warn_unix.c"
#elif OS == OS_WIN32
#include "warn_win32.c"
#endif

debug_func_t  debug_func = 0;
void         *debug_arg = 0;
int           debug_level = DEBUG_ERROR;

int
debug_init(int level, debug_func_t func, void *arg) {
    debug_level = level;
    if( !func ) {
	debug_func = debug_func_stdio;
	debug_arg  = stderr;
    }
    else {
	debug_func = func;
	debug_arg  = arg;
    }
    debug_leak_init();
    return 0;
}


char*
debug_level2str(int level) {
    static char buf[1024], *p=0;
    
    switch(level) {
    case DEBUG_ASSERT: p = "assert"; break; 
    case DEBUG_ERROR: p = "error"; break;
    case DEBUG_WARN: p = "warn"; break;
    case DEBUG_INFO: p = "info"; break;
    default:
	sprintf(buf, "debug=%d", level);
	p = buf;
	break;
    }
    return p;
}

mstime_t debug_last_time = 0;
procid_t debug_last_pid = 0;
threadid_t debug_last_tid = 0;
mstime_t debug_hdr_time_interval = 0;

int
debug_hdr(int level, const char *file, int line) {
    char buf1[1024];
    mstime_t t = mstime();
    procid_t pid = proc_getpid();
    threadid_t tid = proc_getthreadid();

    syserr_save();
    
    if( (debug_hdr_time_interval > 0 
	 && t - debug_last_time > debug_hdr_time_interval)
	|| pid != debug_last_pid
	|| tid != debug_last_tid ) {
	
	debugf("%s pid=%lu tid=%lu\n"
	       ,mstime_fmt(t, buf1, sizeof(buf1))
	       ,pid
	       ,tid
	       );
	debug_last_time = t;
    }

    debugf("%7.04f %s:%-3d "
	   ,t - debug_last_time
	   ,file
	   ,line
	   );
    
    debug_last_time = t;
    debug_last_pid = pid;
    debug_last_tid = tid;

    syserr_restore();
    return 0;
}

int
debug_s(char *buf) {
    // used to be buffered, but that overflowed mem.  So, to make
    // things easier, I'll just print
    if( !debug_func ) {
	debug_func = debug_func_stdio;
	debug_arg  = stderr;
    }
    return debug_func(buf, debug_arg);
}

int
debugf(const char *fmt, ...) {
    char buf[4096]; /* NOTE: finite buffer! */
    va_list vargs;

    va_start(vargs, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vargs);
    va_end(vargs);

    return debug_s(buf);
}

void
warn_syserr() {
    char buf[4096];
    int i = syserr();
    warn_v("GetLastError: (0x%08x %d) %s\n", i, i, 
	   syserr2str(i, buf, sizeof(buf)));
}

