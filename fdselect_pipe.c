/* 
   fdselect_pipe.c - bidirectional buffered pipe between sockets 
   Noel Burton-Krahn <noel@burton-krahn.com>
   Sep 24, 2006

 */

#include "bklib/configbk.h"
#include "bklib/debug.h"

#include "bklib/fdselect_pipe.h"

void
fdselect_pipe_sock_read(fdselect_t *sel, fd_t fd, int state, void *arg);

void
fdselect_pipe_sock_write(fdselect_t *sel, fd_t fd, int state, void *arg);

int
fdselect_pipe_close(fdselect_pipe_t *pipe, int close_socks) {
    int i, r;
    
    for(i=0; i<2; i++) {
	r = fdselect_unset(pipe->fdselect, pipe->sock[i].sock, FDSELECT_ALL);
	assertb(r>=0);
	if( close_socks ) {
	    sock_close(pipe->sock[i].sock);
	}
    }
    return 0;
}

/* return >0 iff both sockets were closed */
int
fdselect_pipe_closed(fdselect_pipe_t *pipe) {
    int i;
    for(i=0; i<2; i++) {
	if( pipe->sock[i].select_read  || pipe->sock[i].select_write ) {
	    return 0;
	}
    }
    return 1;
}

int
fdselect_pipe_init(fdselect_pipe_t *pipe, fdselect_t *fdselect
		   ,fdselect_callback_t callback, void *callback_arg
		   ,sock_t sock0, char *buf0, int max0
		   ,sock_t sock1, char *buf1, int max1
		   ) {
    int i, r, err=0;

    memset(pipe, 0, sizeof(*pipe));
    pipe->fdselect = fdselect;
    pipe->callback = callback;
    pipe->callback_arg = callback_arg;

    pipe->sock[0].pipe = pipe;
    pipe->sock[0].peer = &pipe->sock[1];
    pipe->sock[0].sock = sock0;
    pipe->sock[0].buf = buf0;
    pipe->sock[0].max = max0;

    pipe->sock[1].pipe = pipe;
    pipe->sock[1].peer = &pipe->sock[0];
    pipe->sock[1].sock = sock1;
    pipe->sock[1].buf = buf1;
    pipe->sock[1].max = max1;

    do {
	for(i=0; i<2; i++) {
	    pipe->sock[i].select_read = 1;
	    r = fdselect_set(pipe->fdselect, pipe->sock[i].sock, FDSELECT_READ,
			     fdselect_pipe_sock_read, &pipe->sock[i]);
	    assertb(r>=0);
	}
	assertb(i=2);
	err = 0;
    } while(0);

    if( err ) {
	fdselect_pipe_close(pipe, 0);
    }

    return err;
}

int
fdselect_pipe_closed_callback(fdselect_pipe_t *pipe) {
    int err=-1;
    do {
	if( fdselect_pipe_closed(pipe) ) {
	    if( pipe->callback ) {
		pipe->callback(pipe, 0, FDSELECT_PIPE_CLOSED, 0, 0, 
			       pipe->callback_arg);
	    }
	    else {
		fdselect_pipe_close(pipe, 1);
	    }
	}
	err = 0;
    } while(0);
    return err;
}

void
fdselect_pipe_sock_read(fdselect_t *sel, fd_t fd, int state, void *arg) {
    fdselect_pipe_sock_t *client = (fdselect_pipe_sock_t *)arg;
    fdselect_pipe_t      *pipe = client->pipe;
    fdselect_pipe_sock_t *peer = client->peer;
    int r, err = -1;
    int off, len;
    
    do {
	off = client->off + client->len;

	if( client->max - off <= 0 ) {
	    err = 0;
	    break;
	}

	len = sock_recv(client->sock, client->buf + off, client->max - off);

	debug(DEBUG_INFO, 
	      ("fdselect_pipe_sock_read: sock=%d len=%d\n", client->sock, len));

	if( len <= 0 ) {
	    if( sock_wouldblock(client->sock, len) ) {
		/* nothing to do */
		err = 0;
		break;
	    }
	    
	    /* read error.  Stop reading, but don't shut down until
	       all my buffered read data was written */
	    client->err_read = 1;

	    debug(DEBUG_INFO, 
		  ("fdselect_pipe_sock_read: shutdown(sock=%d SOCK_SHUTDOWN_RECV)\n", client->sock));

	    sock_shutdown(client->sock, SOCK_SHUTDOWN_RECV);

	    debug(DEBUG_INFO, 
		  ("fdselect_pipe_sock_read: fdselect_unset(sock=%d FDSELECT_READ)\n", client->sock));

	    fdselect_unset(sel, client->sock, FDSELECT_READ);
	    err = 0;
	    break;
	}
	else {
	    client->len += len;
	    assertb(client->off + client->len <= client->max);
	}

	if( pipe->callback ) {
	    pipe->callback(pipe, client, FDSELECT_PIPE_READ, 
			   client->buf + off, len, 
			   pipe->callback_arg);
	}
    
	err = 0;

	/* loop forever until sock would block */
	err = -1;
    } while(1);

    /* set up FDSELECT_READ on myself */
    do {
	assertb(!err);
	err = -1;

	/* see if I can write right away, this also sets up
	   FDSELECT_WRITE if necessary  */
	fdselect_pipe_sock_write(sel, 0, 0, peer);

	if( client->off + client->len < client->max 
	    && !client->err_read  ) {
	    if( !client->select_read ) {

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_read: fdselect_set(sock=%d FDSELECT_READ)\n", client->sock));

		r = fdselect_set(sel, client->sock, FDSELECT_READ,
				 fdselect_pipe_sock_read, client);
		assertb(r>=0);
		client->select_read = 1;
	    }
	}
	else {
	    if( client->select_read ) {

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_read: fdselect_unset(sock=%d FDSELECT_READ)\n", client->sock));

		fdselect_unset(sel, client->sock, FDSELECT_READ);
		client->select_read = 0;
	    }
	}

	fdselect_pipe_closed_callback(pipe);

	err = 0;
    } while(0);
}


/* send bytes from my other half's buffer */
void
fdselect_pipe_sock_write(fdselect_t *sel, fd_t fd, int state, void *arg) {
    fdselect_pipe_sock_t *client = (fdselect_pipe_sock_t *)arg;
    fdselect_pipe_t      *pipe = client->pipe;
    fdselect_pipe_sock_t *peer = client->peer;
    int i, r, err = -1;

    do {
	if( peer->len <= 0 ) {
	    err = 0;
	    break;
	}

	i = sock_send(client->sock, peer->buf + peer->off, peer->len);

	debug(DEBUG_INFO, 
	      ("fdselect_pipe_sock_write: sock=%d len=%d\n", client->sock, i));

	if( i <= 0 && sock_wouldblock(client->sock, i) ) {
	    /* no error */
	    err = 0;
	    break;
	}

	if( pipe->callback ) {
	    pipe->callback(pipe, client, FDSELECT_PIPE_WRITE,
			   peer->buf + peer->off, i, 
			   pipe->callback_arg);
	}

	if( i <= 0 ) {
	    /* write error: close writing and stop reading */
	    client->err_write = 1;

	    debug(DEBUG_INFO, 
		  ("fdselect_pipe_sock_write: shutdown(sock=%d SOCK_SHUTDOWN_RECV)\n", peer->sock));

	    sock_shutdown(peer->sock, SOCK_SHUTDOWN_RECV);

	    if( peer->select_read ) {

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_write: fdselect_unset(sock=%d FDSELECT_READ)\n", peer->sock));

		fdselect_unset(sel, peer->sock, FDSELECT_READ);
		peer->select_read = 0;
	    }
		
	    err = 0;
	    break;
	}

	assertb(i <= peer->len);
	peer->off += i;
	peer->len -= i;
	if( peer->len == 0 ) {
	    peer->off = 0;
	}
	
	
	err = 0;
	
	/* loop forever until sock would block */
	err = -1;
    } while(1);

    /* set my FDSELECT bits */
    do {
	assertb(err==0);
	err = -1;
	
	/* set FDSELECT_READ on my peer if I opened up some buffer space */
	if( peer->off + peer->len < peer->max && !peer->err_read ) {
	    if( !peer->select_read ) {

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_write: fdselect_set(sock=%d FDSELECT_READ)\n", peer->sock));

		r = fdselect_set(sel, peer->sock, FDSELECT_READ, 
				 fdselect_pipe_sock_read, peer);
		assertb(r>=0);
		peer->select_read = 1;
	    }
	}

	/* set up FDSELECT_WRITE on myself */
	if( peer->len > 0 && !client->err_write ) {
	    if( !client->select_write ) {

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_write: fdselect_set(sock=%d FDSELECT_WRITE)\n", client->sock));

		r = fdselect_set(sel, client->sock, FDSELECT_WRITE, 
				 fdselect_pipe_sock_write, client);
		assertb(r>=0);
		client->select_write = 1;
	    }
	}
	else {
	    /* finally stop reading the peer when the read buffer is
	       empty */
	    if( peer->err_read && !peer->err_read_done ) {
		peer->err_read_done = 1;

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_write: shutdown(sock=%d SOCK_SHUTDOWN_SEND)\n", client->sock));

		sock_shutdown(client->sock, SOCK_SHUTDOWN_SEND);
	    }
	    if( client->select_write ) {

		debug(DEBUG_INFO, 
		      ("fdselect_pipe_sock_write: fdselect_unset(sock=%d FDSELECT_WRITE)\n", client->sock));

		fdselect_unset(sel, client->sock, FDSELECT_WRITE);
		client->select_write = 0;
	    }
	}

	fdselect_pipe_closed_callback(pipe);

	err = 0;
    } while(0);
}

