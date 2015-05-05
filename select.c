typedef void (*fdselect_fd_func)(int fd, int state, void *arg);
typedef void (*fdselect_fd_free)(void *arg);

typedef enum {
    FDSELECT_EVENT_READ  = 1,
    FDSELECT_EVENT_WRITE = 1<<1,
    FDSELECT_EVENT_ERROR = 1<<2,
} fdselect_events;

struct fdselect_fd {
    int fd;
    int events;
    fdselect_fd_func func;
    fdselect_fd_free free;
    void *arg;
}

struct fdselect {
    array fd_array;
}

int
fdselect_init(fdselect *sel) {
    array_init(sel->fd_array, sizeof(fdselect_fd));
}

int
fdselect_free(fdselect *sel) {
    array_free(sel->fd_array);
}

int
fdselect_set(fdselect *sel, 
	     int fd, 
	     int events, 
	     fdselect_fd_func func,
	     void *arg
	     ) {
    fdselect_fd *f;
    int i;
    
    do {
	for(i=array_len(sel->fd_array)-1; i>=0; i--) {
	    fdselect_fd *g = (fdselect_fd *)array_get(sel->fd_array, i);
	    if( !g->events ) {
		f = g;
	    }
	    if( g->fd == fd && g->events == events ) {
		f = g;
		break;
	    }
	}
	if( !found ) {
	    f = (fdselect_fd *)array_add(sel->fd_array, 1);
	}
	assertb(f);

	f->fd = fd;
	f->events = events;
	f->func = func;
	f->free = free;
	f->arg = arg;

	err = 0;
    } while(0);
    return err;
}

int
fdselect_clear(int fd) {
    do {
	for(i=array_len(sel->fd_array)-1; i>=0; i--) {
	    f = (fdselect_fd *)array_get(sel->fd_array, i);
	    if( f->fd == fd && !events || fd->events & events ) {
		if( f->free ) {
		    f->free(f->arg);
		}
		f->events &= ~events;
	    }
	}
    } while(0);
}


int
fdselect_select(fdselect sel, int tmieout_ms) {
    fd_set rfds;
    int i, fd_max, nfds;
    
    FD_ZERO(&rfds);
    fd_max = -1;
    
    for(i=array_len(sel->fd_array)-1; i>=0; i--) {
	fdselect_fd *f = (fdselect_fd *)array_get(sel->fd_array, i);
	if( !f->events ) continue;
	FD_SET(f->fd, &rfds);
	if( f->fd > fd_max ) fd_max = f->fd;
    }

    nfds = select(fd_max+1, &rfds, &wfds, 0, ptv);
    
    for(i=array_len(sel->fd_array)-1; nfds>0 && i>=0; i++) {
	fdselect_fd *f = (fdselect_fd *)array_get(sel->fd_array, i);

	if( FD_SET(f->fd, &rfds) ) {
	    nfds--;
	    f->func(f->fd, FDSELECT_READ, f->arg);
	}
	else {
	}
    }
}
