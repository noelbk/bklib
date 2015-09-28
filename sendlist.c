#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include "sock.h"
#include "sendlist.h"
#include "debug.h"

int
sendlist_init(sendlist_t *sendlist, fdselect_t *fdsel, fd_t send_fd,
	      sendlist_flushed_func_t flushed_func, void *func_arg) {
    int e, err=-1;
    do {
	memset(sendlist, 0, sizeof(*sendlist));
	sendlist->fdsel = fdsel;
	sendlist->send_fd = send_fd;
	e = array_init(&sendlist->sendbufs, sizeof(sendbuf_t), 10);
	assertb(!e);
	sendlist->flushed_func = flushed_func;
	sendlist->func_arg = func_arg;
	err = 0;
    } while(0);
    return err;
}

int
sendlist_close(sendlist_t *sendlist) {
    int e, err=-1;
    do {
	// remove and free all sendbufs
	while( sendlist_shift(sendlist) ) {}
	e = fdselect_unset(sendlist->fdsel, sendlist->send_fd, FDSELECT_WRITE);
	assertb(!e);
	array_clear(&sendlist->sendbufs);
	err = 0;
    } while(0);
    return err;
}
    
sendbuf_t*
sendlist_peek(sendlist_t *sendlist) {
    return (sendbuf_t*)array_get(&sendlist->sendbufs, 0);
}

sendbuf_t*
sendlist_shift(sendlist_t *sendlist) {
    sendbuf_t *sendbuf = sendlist_peek(sendlist);
    if( sendbuf ) {
	if( sendbuf->free_func ) {
	    sendbuf->free_func(sendbuf, sendbuf->free_arg);
	}
	array_remove_idx(&sendlist->sendbufs, 0, 1);
    }
    return sendbuf;
}

sendbuf_t*
sendlist_push(sendlist_t *sendlist) {
    return (sendbuf_t*)array_add(&sendlist->sendbufs, 1);
}

static
int
sendlist_send(sendlist_t *sendlist, void *buf, fd_t fd, size_t off, size_t len,
	      sendbuf_free_func_t free_func, void *free_arg) {
    sendbuf_t *sendbuf=0;
    int err=-1;
    do {
	sendbuf = sendlist_push(sendlist);
	assertb(sendbuf);
	sendbuf->buf = buf;
	sendbuf->fd = fd;
	sendbuf->off = off;
	sendbuf->len = len;
	sendbuf->free_func = free_func;
	sendbuf->free_arg = free_arg;
	
	err = sendlist_flush(sendlist);
	assertb(err>=0);
    } while(0);
    return err;
}

int
sendlist_send_buf(sendlist_t *sendlist, void *buf, size_t len,
		  sendbuf_free_func_t free_func, void *free_arg) {
    return sendlist_send(sendlist, buf, -1, 0, len, free_func, free_arg);
}


int
sendlist_send_fd(sendlist_t *sendlist, fd_t fd, size_t off, size_t len,
		 sendbuf_free_func_t free_func, void *free_arg) {
    return sendlist_send(sendlist, 0, fd, off, len, free_func, free_arg);
}

int
sendlist_shutdown(sendlist_t *sendlist,
		  sendbuf_free_func_t free_func, void *free_arg) {
    return sendlist_send(sendlist, 0, -1, 0, 0, free_func, free_arg);
}

void
sendlist_select_write(fdselect_t *sel, fd_t fd, int state, void *arg) {
    sendlist_t *sendlist = (sendlist_t *)arg;
    int err=-1;
    
    do {
	assertb(fd == sendlist->send_fd);
	assertb(sel == sendlist->fdsel);
	err = sendlist_flush(sendlist);
	assertb(err >= 0);
    } while(0);
}

int
sendlist_flush(sendlist_t *sendlist) {
    int e, err=-1;
    sendbuf_t *sendbuf;
    ssize_t n;
    off_t off;
    
    do {
	// flush my outgoing buffer
	while(1) {
	    sendbuf = sendlist_peek(sendlist);
	    if( !sendbuf ) {
		// queue is empty, return 0
		err = 0;
		break;
	    }
	    if( sendbuf->buf ) {
		n = write(sendlist->send_fd, sendbuf->buf+sendbuf->off, sendbuf->len);
	    }
	    else if ( sendbuf->fd >=0 ) {
		off = sendbuf->off;
		n = sendfile(sendlist->send_fd, sendbuf->fd, &off, sendbuf->len);
	    }
	    else {
		e = sock_shutdown(sendlist->send_fd, SOCK_SHUTDOWN_SEND);
		assertb_sockerr(!e);
		n = sendbuf->len;
	    }
	    if( sock_wouldblock(sendlist->send_fd, n) ) {
		n = 0;
	    }
	    assertb_sockerr(n>=0);
	    sendbuf->len -= n;
	    sendbuf->off += n;
	    if( sendbuf->len > 0 ) {
		// blocked on write, so return 1
		err = 1;
		break;
	    }
	    sendlist_shift(sendlist);
	}
	if( err > 0 ) {
	    e = fdselect_set(sendlist->fdsel, sendlist->send_fd, FDSELECT_WRITE, sendlist_select_write, sendlist);
	}
	else {
	    e = fdselect_unset(sendlist->fdsel, sendlist->send_fd, FDSELECT_WRITE);
	}
	assertb(!e);

	assertb(err>=0);
    } while(0);
    
    if( sendlist->flushed_func ) {
	sendlist->flushed_func(sendlist, err, sendlist->func_arg);
    }

    return err;
}
