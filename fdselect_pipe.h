/* 
   fdselect_pipe.c - bidirectional buffered pipe between sockets 
   Noel Burton-Krahn <noel@burton-krahn.com>
   Sep 24, 2006

 */

#include "bklib/sock.h"
#include "bklib/fdselect.h"

#ifdef __cplusplus
#extern "C" {
#endif /* __cplusplus */

typedef struct fdselect_pipe_t fdselect_pipe_t;

typedef struct fdselect_pipe_sock_t fdselect_pipe_sock_t;

typedef enum {
    FDSELECT_PIPE_READ = 1
    ,FDSELECT_PIPE_WRITE = 2
    ,FDSELECT_PIPE_CLOSED = 4
} fdselect_pipe_event_t;

typedef int (*fdselect_callback_t)(fdselect_pipe_t *pipe, 
				   fdselect_pipe_sock_t *sock, 
				   fdselect_pipe_event_t event, 
				   char *buf, int len,
				   void *arg);

struct fdselect_pipe_sock_t {
    fdselect_pipe_t *pipe;
    fdselect_pipe_sock_t *peer;

    /* read/write sock.  read to self->buf, write from peer->buf */
    sock_t sock;
    int err_read;
    int err_write;
    int select_read;
    int select_write;

    /** read buffer */
    char *buf;
    int off;
    int len;
    int max;

    void *extra;

    int err_read_done; /** internal: true iff err_read and called callback */

};

struct fdselect_pipe_t {
    fdselect_t *fdselect;
    fdselect_pipe_sock_t sock[2];
    fdselect_callback_t callback;
    void *callback_arg;
};


int
fdselect_pipe_init(fdselect_pipe_t *pipe, fdselect_t *fdselect
		   ,fdselect_callback_t callback, void *callback_arg
		   ,sock_t sock0, char *buf0, int max0
		   ,sock_t sock1, char *buf1, int max1
		   );

int
fdselect_pipe_close(fdselect_pipe_t *pipe, int close_socks);

#ifdef __cplusplus
}
#endif /* __cplusplus */

