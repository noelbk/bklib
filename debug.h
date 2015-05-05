#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "bklib/configbk.h"
#include "bklib/debug_leak.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum debug_level_e {
    DEBUG_ASSERT = -2,
    DEBUG_ERROR  = -1,
    DEBUG_WARN   =  0,
    DEBUG_INFO   =  1,
    DEBUG_MAX    =  9999
};

typedef int (*debug_func_t)(const char *buf, void *arg);

int
debug_init(int level, debug_func_t func, void *arg);

// enable line buffering
int
debug_line_buffer(int buflen);

int
debug_hdr(int level, const char *file, int line);

int
debugf(const char *fmt, ...);

#define debug_v debugf

extern debug_func_t  debug_func;
extern void         *debug_arg;
extern int           debug_level;

#define debug(level, vargs) \
    if( debug_level >= level ) { \
	debug_hdr(level, __FILE__, __LINE__); \
	debugf vargs; \
    }

#define debug_if(level) \
    if( debug_level >= level && debug_hdr(level, __FILE__, __LINE__) >= 0 )

#define warn(vargs)	\
    debug(DEBUG_WARN, vargs)

#define warn_v debug_v

#define warn_at(file, line) \
    debug_hdr(DEBUG_WARN, file, line)

#define assertb(cond) \
    if( !(cond) ) { \
	debug(DEBUG_ASSERT, ("assert(%s) failed\n", #cond));	\
	break; \
    }

#define assertbf(cond, vargs) \
    if( !(cond) ) { \
	debug(DEBUG_ASSERT, ("assert(%s) failed: ", #cond));	\
        debugf vargs; \
	break; \
    }

#define assert_continue(cond) \
    if( !(cond) ) { \
	debug(DEBUG_ASSERT, ("assert(%s) failed: \n", #cond));	\
	continue; \
    }

// returns GetLastError() or errno
int syserr();

// calls FormatMessage() or strerror()
char *syserr2str(int syserr, char *buf, int len);

// so debug functions don't reset GetLastError
void syserr_save();

void syserr_restore();

void warn_syserr();
#define assertb_syserr(cond) \
    if( !(cond) ) { \
	debug(DEBUG_ASSERT, ("assert(%s) failed: ", #cond));	\
        warn_syserr(); \
	break; \
    }

void warn_sockerr();
#define assertb_sockerr(cond) \
    if( !(cond) ) { \
	debug(DEBUG_ASSERT, ("assert(%s) failed: ", #cond));	\
        warn_sockerr(); \
	break; \
    }

#if OS == OS_UNIX
#include "warn_unix.h"
#endif

#if OS == OS_WIN32
#include "warn_win32.h"
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DEBUG_H_INCLUDED
