// thanks to bindd

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <string.h>
#include "warn.h"

int
sock_send_fd(int sock, char *buf, int len, int *fd, int nfds) {
    struct iovec	iov[1];
    struct msghdr	msg;
    char cmsgbuf[sizeof(*fd) * 256];
    struct cmsghdr *cmsg;
    int n;

#if DUMMY_BYTE_HACK
    char c=0;
    if( !len || !buf) {
	buf = &c;
	len = 1;
    }
#endif

    iov[0].iov_base = buf;
    iov[0].iov_len = len;
    cmsg = (typeof(cmsg))cmsgbuf;
    cmsg->cmsg_len = CMSG_LEN(nfds*sizeof(*fd));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    memcpy(CMSG_DATA(cmsg), fd, nfds*sizeof(*fd));
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_SPACE(nfds * sizeof(*fd));
    msg.msg_flags = 0;

    n = sendmsg(sock, &msg, MSG_NOSIGNAL);

    //debug(("sock_send_fd: n=%d nfds=%d buf=%s\n", n, nfds, buf));

    return n;
}

int
sock_recv_fd(int sock, const void *buf, size_t len, int *fd, int *nfds)
{
    struct msghdr msg;
    char cmsgbuf[4096];
    struct cmsghdr *cmsg;
    struct iovec iovec;
    int i, n;
    char c=0;

    if( !len || !buf) {
	buf = &c;
	len = 1;
    }

    iovec.iov_base = (char*)buf;
    iovec.iov_len = len;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iovec;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    msg.msg_flags = MSG_WAITALL;

    n = recvmsg(sock, &msg, 0);

    if( n < 0 ) {
	return n;
    }
    
    i = 0;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
	 cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if( cmsg->cmsg_level != SOL_SOCKET ) {
	    continue;
	}
	if( cmsg->cmsg_type != SCM_RIGHTS ) {
	    continue;
	}
	i = (cmsg->cmsg_len - CMSG_LEN(0))/sizeof(*fd);
	
	if( i > *nfds ) { i = *nfds; }
	*nfds = i;
	memcpy(fd, CMSG_DATA(cmsg), i*sizeof(*fd));
	break;
    }
    *nfds = i;

    ((char*)buf)[n>0?n:0]=0;
    //debug(("sock_recv_fd: n=%d nfds=%d buf=%s\n", n, *nfds, buf));

    return n;
}
