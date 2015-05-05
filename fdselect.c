#include <stdlib.h>

#include "config.h"
#include "sock.h"
#include "fdselect.h"
#include "array.h"
#include "debug.h"
#include "mstime.h"

struct fdselect_fd {
    fd_t              fd;
    int               events;

    unsigned long     os_event;
    int               free_os_event;
    int               not_sock;

    fdselect_fd_func  read_func;
    void             *read_farg;

    fdselect_fd_func  write_func;
    void             *write_farg;

    mstime_t          t_last;
};

fdselect_t*
fdselect_alloc() {
    fdselect_t *sel;
    sel = calloc(sizeof(*sel), 1);
    fdselect_init(sel);
    return sel;
}

void
fdselect_free(fdselect_t *sel) {
    fdselect_clear(sel);
    array_clear(&sel->fd_array);
    free(sel);
}

int
fdselect_init(fdselect_t *sel) {
    memset(sel, 0, sizeof(*sel));
    return array_init(&sel->fd_array, sizeof(fdselect_fd), 0);
}

fdselect_t*
fdselect_new() {
    fdselect_t *sel;
    sel = (fdselect_t*)malloc(sizeof(*sel));
    fdselect_init(sel);
    return sel;
}

void
fdselect_clear(fdselect_t *sel) {
    array_clear(&sel->fd_array);
    sel->count = 0;
    sel->select_break = 1;
}

int
fdselect_count(fdselect_t *sel) {
    return sel->count;
}

fdselect_fd*
fdselect_get(fdselect_t *sel, fd_t fd, int events) {
    fdselect_fd *f;
    int i;
    do {
	f = (fdselect_fd *)array_get(&sel->fd_array, -1);
	for(i=array_count(&sel->fd_array)-1; i>=0; i--, f--) {
	    if( f->fd == fd && f->events & events ) {
		return f;
	    }
	}
    } while(0);
    return 0;
}

void
fdselect_free_os_event(fdselect_fd *f);

int
fdselect_set(fdselect_t *sel, fd_t fd, int events, 
	     fdselect_fd_func func, void *farg
	     ) {
    fdselect_fd *f;
    int new_events;
    int i, err=-1;
    
    sel->select_break = 1;
    if( !events ) {
	events = FDSELECT_READ | FDSELECT_WRITE;
    }
    do {
	f = (fdselect_fd *)array_get(&sel->fd_array, -1);
	for(i=array_count(&sel->fd_array)-1; i>=0; i--, f--) {
	    if( f->fd == fd ) break;
	}
	if( i < 0 ) {
	    f = (fdselect_fd *)array_add(&sel->fd_array, 1);
	    assertb(f);
	    f->fd = fd;
	    sel->count++;
	}
	
	/* if no func, then clear events, otherwise add to event set */
	new_events = func ? (f->events | events) : (f->events & ~events);
	
	if( new_events != f->events 
	    && f->os_event 
	    && f->free_os_event ) {
	    fdselect_free_os_event(f);
	    f->os_event = 0;
	}
	f->events = new_events;
	if( events & (FDSELECT_READ | FDSELECT_ADDRESS_LIST_CHANGE) ) {
	    f->read_func = func;
	    f->read_farg = farg;
	}
	if( events & FDSELECT_WRITE ) {
	    f->write_func = func;
	    f->write_farg = farg;
	}
	if( !f->events ) {
	    array_remove_ptr(&sel->fd_array, f);
	    sel->count--;
	}
	err = 0;
    } while(0);
    return err;
}

int
fdselect_unset(fdselect_t *sel, fd_t fd, int events) {
    if( !events ) { events = FDSELECT_ALL; }
    sel->select_break = 1;
    return fdselect_set(sel, fd, events, 0, 0);
}

void
fdselect_break(fdselect_t *sel) {
    sel->select_break = 1;
}

#if OS == OS_UNIX 
#include "fdselect_unix.c"
#endif

#if OS == OS_WIN32
#include "fdselect_win32.c"
#endif

int
fdselect_select(fdselect_t *sel, int timeout) {
    sel->select_break = 0;
    return fdselect_select_os(sel, timeout);
}

int
fdselect_enable_win_loop(fdselect_t *sel, int enable) {
    int old = sel->enable_win_loop;
    sel->enable_win_loop = enable;
    return old;
}

