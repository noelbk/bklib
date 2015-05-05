#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "sock.h"
#include "warn.h"

int sock_init() {
    return 1;
}

int sock_fini() {
    return 1;
}

int sock_valid(sock_t sock) {
    return sock >= 0;
}


int sock_errno(int fd) {
    return errno;
}

int
sock_wouldblock(int fd, int ret) {
    return (ret < 0) && 
	(errno == EWOULDBLOCK 
	 || errno == EAGAIN 
	 || errno == EINPROGRESS ); 
}

int
sock_isconn(int fd, int ret) {
    return (ret < 0) && (errno == EISCONN); 
}

int 
sock_nonblock(sock_t sock, int nonblock) {
    int i, old;
    i = fcntl(sock, F_GETFL, 0);
    if( i < 0 ) {
	return i;
    }

    old = (i & O_NONBLOCK) ? 1 : 0;
    
    if( nonblock ) {
	i |= O_NONBLOCK;
    }
    else {
	i &= ~O_NONBLOCK;
    }
    fcntl(sock, F_SETFL, i);
    return old;
}

int 
sock_close_on_exec(sock_t sock, int close) {
    int i = close ? 0 : FD_CLOEXEC;
    return fcntl(sock, F_SETFD, i);
}


int
sock_pair(sock_t sock[2]) {
    return socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, sock);
}

int
sock_close(sock_t sock) {
    return close(sock);
}

int
sock_read_avail(sock_t sock) {
    int i, n;
    n = 0;
    i = ioctl(sock, FIONREAD, &n);
    if( i<0 ) {
	warn_sockerr();
	n = i;
    }
    return n;
}

char*
sock_err2str(int err) {
  return "unknown";
}
