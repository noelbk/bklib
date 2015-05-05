/*
    REVISION HISTORY:

    2001/12/10 D.Chan - add for OS_WIN32, add OutputDebugString to warn_v().
    2002/01/02 D.Chan - add for OS_WIN32, support for warn_OutputDebugString flag
*/
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#include "warn.h"
#include "mstime.h"

#if OS == OS_UNIX
#include "warn_unix.c"
#include <unistd.h>
#elif OS == OS_WIN32
#include "warn_win32.c"
#define getpid GetCurrentProcessId
int warn_OutputDebugString = 0; /* defaults to disabled */
#endif

mstime_t warn_last_time=0;

FILE *warn_fp = 0;

FILE *warn_get_fp() {
    return warn_fp ? warn_fp : stderr;
}

void warn_set_fp(FILE *fp) {
    warn_fp = fp;
}

void assert_warn(char *file, int line) {
    FILE *f = warn_get_fp();
    warn_at(file, line);
    fprintf(f, "assert failed\n");
    fflush(f);
}

void warn_at(char *file, int line) {
    FILE *f = warn_get_fp();
    mstime_t t = mstime();
    fprintf(f, "%0.3f %s:%d [%u] ", t - warn_last_time, file, line, (unsigned)getpid());
    warn_last_time = t;
}

void warn_v(char *fmt, ...) {
    FILE *f = warn_get_fp();
    va_list vargs;
    va_start(vargs, fmt);
#if OS == OS_UNIX
    vfprintf(f, fmt, vargs);
#elif OS == OS_WIN32
    vfprintf(f, fmt, vargs);
    if (warn_OutputDebugString) { /* echo to win32 debug console too */
	char buffer[2048]; /* NOTE: finite buffer! */
	vsprintf(buffer, fmt, vargs);
	OutputDebugString(buffer);
    }
#endif
    va_end(vargs);
    fflush(f);
}

int
warn_log(char *file) {
    int err = -1;
    FILE *fd;
    time_t t;

    do {
	fd = fopen(file, "a+");
	assertb(fd);

	if( warn_fp ) {
	    fclose(warn_fp);
	}
	warn_fp = fd;
	
	time(&t);
	fprintf(warn_fp, "\nLog started at %s\n", ctime(&t));
        fflush(warn_fp);
    } while(0);
    return err;
}

int DEBUG_LEVEL=DEBUG_ERROR;

int
debug_level_get(char *module) {
    char buf[4096];
    char *key=0, *val=0, *p;
    int level;
    
    // debug - disable per-module debugging for speed
    return DEBUG_LEVEL;

    if( module ) {
	sprintf(buf, "DEBUG_%s", module);
	key = buf;
	val = getenv(key);
    }

    if( !val ) {
	key = "DEBUG";
	val = getenv(key);
    }

    if( val ) {
	p = val;
	level = (int)strtoul(val, &p, 0);
	if( !level && p<=val ) {
	    level = DEBUG_MAX;
	}
    }
    else {
	level = DEBUG_LEVEL;
    }

    return level;
}

int
debug_level_set(char *module, int level) {
    char buf[4096];

    if( module ) {
	sprintf(buf, "DEBUG_%s=%d", module, level);
    }
    else {
	sprintf(buf, "DEBUG=%d", level);
    }
    putenv(strdup(buf));
    return debug_level_get(module);
}


char*
debug_level2str(int level) {
    static char buf[1024], *p=0;
    
    switch(level) {
    case DEBUG_ASSERT: p = "error assert"; break; 
    case DEBUG_ERROR: p = "error"; break;
    case DEBUG_WARN: p = "warn"; break;
    case DEBUG_INFO: p = "info"; break;
    default:
	sprintf(buf, "level=%d", level);
	p = buf;
	break;
    }
    return p;
}

int
debug_hdr(int force, char *module, int level, char *file, int line) {
    if( !(force || level <= debug_level_get(module) ) ) {
	return 0;
    }
    
    warn_at(file, line);
    /*
    warn_v("[");
    if( module ) {
	warn_v("%s ", module);
    }
    warn_v("%s] ", debug_level2str(level));
    */
    return 1;
}

int
debug_init(int debug_level) {
    DEBUG_LEVEL = debug_level;
    return 0;
}
