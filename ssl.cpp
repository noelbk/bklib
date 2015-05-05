// ssl_nonblock.cpp - how to handle SSL_ERROR_WANT_READ/WRITE with select()
//
// Noel Burton-Krahn <noel@burton-krahn.com>
// Oct 07, 2002
//
// SSL functions like SSL_read, SSL_write will fail if a read or write
// would block on a non-blocking socket.  You are expected to select()
// the socket for read- or write-ability and try the call again.
// 
//

#include <openssl.h>

// SSL on nonblocking sockets may fail if a read or write would block.
// this checks the SSL error condition and does a select() for read or
// write.  You should retry the SSL_ call if this returns >0.
//
// ret is the return code from the last SSL function call.  timeout is
// the max milliseconds to block on select, <0 is infinite.  returns 0
// => ssl closed, <0 => ssl error, >0 => try again.
//
// use this in a loop with SSL_read, SSL_write, SSL_accept,
// SSL_connect, etc.  See ssl_read_nonblock() below for an example.
//
int
ssl_want_retry(SSL *ssl, int ret, int timeout) {
    int i;
    fd_set fds;
    struct timeval tv, *ptv=0;
    int sock;

    // something went wrong.  I'll retry, die quietly, or complain
    i = SSL_get_error(ssl, ret);
    if( i == SSL_ERROR_NONE ) {
	return 1;
    }

    // get ready to select
    if( timeout >=0 ) {
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000) * 1000;
	ptv = &tv;
    }
    FD_ZERO(&fds);

    switch(i) {
    case SSL_ERROR_WANT_READ:
	// pause until the socket is readable
	sock = SSL_get_rfd(ssl);
	FD_SET(sock, &fds);
	i = select(sock+1, &fds, 0, 0, ptv);
	break;

    case SSL_ERROR_WANT_WRITE:
	// pause until the socket is writeable
	sock = SSL_get_wfd(ssl);
	FD_SET(sock, &fds);
	i = select(sock+1, 0, &fds, 0, ptv);
	break;

    case SSL_ERROR_ZERO_RETURN:
	// the sock closed, just return quietly
	i = 0;
	break;

    default:
	// ERROR - unexpected error code
	i = -1;
    }
    return i;
}

int
ssl_read_nonblock(SSL *ssl, void *buf, int len, int timeout) {
    int i;

    // loop over ssl_want_retry
    while(1) {
	i = SSL_read(ssl, (char*)buf, len);
	if( i > 0 ) { break; }
	i = want_retry(ssl, i, timeout);
	if( i <= 0 ) { break; }
    }
    return i;
}

