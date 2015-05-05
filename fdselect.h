//--------------------------------------------------------------------
// fdselect.h - set callbacks on sockets
// Noel Burton-Krahn
// Oct 25, 2003
//
// Copyright 2002 Burton-Krahn, Inc.
//
//
#ifndef FDSELECT_H_INCLUDED
#define FDSELECT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "array.h"

struct fdselect_s;
typedef struct fdselect_s fdselect_t;

typedef int fd_t;

typedef void (*fdselect_fd_func)(fdselect_t *sel, fd_t fd, int state, void *arg);
typedef enum {
    FDSELECT_READ  = 1
    ,FDSELECT_WRITE = 2
    ,FDSELECT_ADDRESS_LIST_CHANGE = 4
    ,FDSELECT_ALL = -1
} fdselect_events;

struct fdselect_s {
    int count;
    int select_break;
    array_t fd_array;
    int enable_win_loop;
};
typedef struct fdselect_fd fdselect_fd;

int
fdselect_init(fdselect_t *sel);

fdselect_t*
fdselect_new();

void
fdselect_free(fdselect_t *sel);

void
fdselect_clear(fdselect_t *sel);

int
fdselect_count(fdselect_t *sel);

#define FDSELECT_SET_MAX 64

/* call func when events occur.  Does not clear previous event
   callbacks */
int
fdselect_set(fdselect_t *sel, 
	     fd_t fd, 
	     int events, 
	     fdselect_fd_func func,
	     void *arg
	     );

#if OS == OS_WIN32

#include "bkwin32.h"

// use WaitForMultipleEvents on handle
int
fdselect_set_handle(fdselect_t *sel, 
		    HANDLE h, 
		    int events, 
		    fdselect_fd_func func,
		    void *arg
		    );
#endif

/* remove callback handlers for events.  remove all if events == 0 */
int
fdselect_unset(fdselect_t *sel, fd_t fd, int events);

fdselect_fd*
fdselect_get(fdselect_t *sel, fd_t fd, int events);

#define FDSELECT_INFINITE -1

#define FDSELECT_NOWAIT 0

/* waits for events to happen, calls callback functions, then returns */
int
fdselect_select(fdselect_t *sel, int timeout_ms);

int
fdselect_enable_win_loop(fdselect_t *sel, int enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FDSELECT_H_INCLUDED */

