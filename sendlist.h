//--------------------------------------------------------------------
// fdselect.h - set callbacks on sockets
// Noel Burton-Krahn
// Oct 25, 2003
//
// Copyright 2002 Burton-Krahn, Inc.
//
//
#ifndef SENDLIST_H_INCLUDED
#define SENDLIST_H_INCLUDED

#include <sys/types.h>

#include "array.h"
#include "fdselect.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct sendlist_s sendlist_t;

typedef struct sendbuf_s sendbuf_t;
typedef void (*sendbuf_free_func_t)(sendbuf_t *buf, void *arg);
struct sendbuf_s {
    sendlist_t *sendlist;
    void *buf;  // if non-null, call write(fd_out, buf+off, len)
    int fd;     // if buf is null, call sendfile(fd_out, fd, off, len)
    size_t len;
    size_t off;
    sendbuf_free_func_t free_func;
    void *free_arg;
};

// called at the end of sendlist_flush to handle errors or do something else if the queue is empty
typedef int (*sendlist_flushed_func_t)(sendlist_t *sendlist, int err, void *arg);
    
struct sendlist_s {
    fd_t send_fd;
    fdselect_t *fdsel;
    array_t sendbufs;
    sendlist_flushed_func_t flushed_func;
    void *func_arg;
};


// flushed_func gets called after fdselect_select_write on send_fd.
// See sendlist_send.
int
sendlist_init(sendlist_t *sendlist, fdselect_t *fdsel, fd_t send_fd,
	      sendlist_flushed_func_t flushed_func, void *func_arg);

// Free all pending sendbufs and turns off select_write on send_fd
int
sendlist_close(sendlist_t *sendlist);

// Returns the sendbuf at the head of the send queue, or null
sendbuf_t*
sendlist_peek(sendlist_t *sendlist);

// Remove the head of the send queue and calls free_func.
sendbuf_t*
sendlist_shift(sendlist_t *sendlist);

// Add a new sendbuf_t on to the queue and return it.  The caller must
// call sendbuf_flush() after filling the sendbuf to actually send it
sendbuf_t*
sendlist_push(sendlist_t *sendlist);

// Send a buffer.  Calls free_func when the buffer has been completely
// sent.  Calls flushed_func after every select_write on
// sendlist->send_fd.
int
sendlist_send_buf(sendlist_t *sendlist, void *buf, size_t len,
		  sendbuf_free_func_t free_func, void *free_arg);

// Send a portion of a file with sendfile
int
sendlist_send_fd(sendlist_t *sendlist, fd_t fd, size_t off, size_t len,
		 sendbuf_free_func_t free_func, void *free_arg);

// call shutdown on the socket after everything was written
int
sendlist_shutdown(sendlist_t *sendlist,
		  sendbuf_free_func_t free_func, void *free_arg);

// Try to send all sendbufs.  Returns 0 if list is empty, 1 if there
// are pending writes, or <0 on error.  Calls flushed_func with err==0
// if sendlist is empty, >0 if writes are pending, or <0 on error.
int
sendlist_flush(sendlist_t *sendlist);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // SENDLIST_H_INCLUDED


