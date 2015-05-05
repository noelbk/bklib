#include <stdio.h>
#include "config.h"

#if OS == OS_UNIX
#include "sock_unix.c"
#endif

#if OS == OS_WIN32
#include "sock_win32.c"
#endif

long
inet_resolve(const char *hostname) {
    struct hostent *h;
    long l;

    l = ntohl(inet_addr(hostname));
    if( l != INADDR_NONE ) {
	return l;
    }
    h = gethostbyname(hostname);
    return h ? ntohl(((unsigned long*)(h->h_addr))[0]) : INADDR_NONE;
}

int
iaddr_pack(struct sockaddr_in *addr, long ip, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(ip);
    addr->sin_port = htons((short)port);
    return sizeof(*addr);
}

char*
iaddr_fmt(struct sockaddr_in *addr, char *buf, int len) {
    static char sbuf[4096];
    if( !buf ) { 
	buf = sbuf; 
	len = sizeof(sbuf); 
    }
    snprintf(buf, len, "%s:%d", 
	     inet_ntoa(addr->sin_addr), 
	     ntohs(addr->sin_port));
    return buf;
}

// converts ip:port to a sockaddr_in.  returns 0 iff ok.
int
iaddr_parse(struct sockaddr_in *addr, char *ip_port) {
    int i, n, err=-1;
    unsigned long ip;
    int port;
    char *buf = ip_port;
    char *p;
    char hostname[4096];

    do {
	memset(addr, 0, sizeof(*addr));
    
	strncpy(hostname, buf, sizeof(hostname)-1);
	hostname[sizeof(hostname)-1] = 0;
	
	p = strchr(hostname, ':');
	if( p ) { *p = 0; }

	ip = inet_resolve(hostname);
	assertb(ip != INADDR_NONE);

	buf += strlen(hostname);
	
	port = 0;
	i = sscanf(buf, ":%d%n", &port, &n);
	if( i == 1) {
	    buf += n;
	}
	err = iaddr_pack(addr, ip, port);
    } while(0);
    return err;
}

char*
iaddr_ntoa(unsigned long i, char *buf) {
    static char sbuf[4096];
    struct in_addr a;

    if( !buf ) {
	buf = sbuf;
    }

    a.s_addr = i;
    snprintf(buf, 20, "%s", inet_ntoa(a));
    return buf;
}

unsigned long
iaddr_aton(char *buf) {
    unsigned long addr;
    int a[4];
    int i, err=-1;
    do {
	i = sscanf(buf, "%d.%d.%d.%d", a, a+1, a+2, a+3);
	if( i != 4 ) {
	    break;
	}
	addr = 0;
	for(i=0; i<4; i++) {
	    assertb(a[i]>=0 && a[i]<=255);
	    addr = (addr <<8) | a[i];
	}
	assertb(i==4);
	err = 0;
    } while(0);
    return err ? 0 : addr;
}

#ifdef SOCK_HAS_AF_UNIX
int
uaddr_pack(struct sockaddr_un *addr, const char *path) {
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path));
    return uaddr_len(addr);
}

int
uaddr_len(const struct sockaddr_un *addr) {
    return (sizeof(*addr) 
	    - sizeof(addr->sun_path) 
	    + strlen(addr->sun_path));
}
#endif // SOCK_HAS_AF_UNIX

int
sock_func_timeout(sock_func_t func, int select_read,
		  sock_t sock, void *vbuf, int len, int ms_timeout, void *arg) {
    int i, err=-1;
    fd_set fds;
    struct timeval tv;
    char *buf=(char*)vbuf;
    char *orig=(char*)vbuf;

    while(len>0) {
	i = func(sock, buf, len, arg);
	if( i > 0 ) {
	    assertb(i<=len);
	    buf += i;
	    len -= i;
	    if( len == 0 ) {
		err = 0;
		break;
	    }
	    continue;
	}
	if( !sock_wouldblock(sock, i) ) {
	    break;
	}

	FD_ZERO(&fds);
	FD_SET((unsigned)sock, &fds);
	tv.tv_sec = ms_timeout/1000;
	tv.tv_usec = (ms_timeout%1000) * 1000;
	if( select_read ) {
	    i = select(sock+1, &fds, 0, 0, &tv);
	}
	else {
	    i = select(sock+1, 0, &fds, 0, &tv);
	}
	assertb_sockerr(i>=0);
	if( i == 0 ) {
	    err = 0;
	    break;
	}
    }
    return err ? err : buf-orig;
}

int
sock_func_recv(sock_t sock, void *buf, int len, void *arg) {
    return recv(sock, buf, len, 0);
}

int
sock_recv_timeout(sock_t sock, void *buf, int len, int timeout) {
    return sock_func_timeout(sock_func_recv, 1, sock, buf, len, timeout, 0);
}

int
sock_func_send(sock_t sock, void *buf, int len, void *arg) {
    return send(sock, buf, len, 0);
}

int
sock_send_timeout(sock_t sock, void *buf, int len, int timeout) {
    return sock_func_timeout(sock_func_send, 0, sock, buf, len, timeout, 0);
}

typedef struct {
    struct sockaddr *addr;
    sockaddrlen_t    addrlen;
} sock_func_sendto_t;

int
sock_func_sendto(sock_t sock, void *buf, int len, void *arg) {
    sock_func_sendto_t *x = (sock_func_sendto_t*)arg;
    return sendto(sock, buf, len, 0, x->addr, x->addrlen);
}

int
sock_sendto_timeout(sock_t sock, void *buf, int len, 
		    struct sockaddr *addr, 
		    sockaddrlen_t addrlen,
		    int timeout) {
    sock_func_sendto_t arg = { addr, addrlen };
    return sock_func_timeout(sock_func_sendto, 0, sock, buf, len, timeout, &arg);
}

int
sock_func_accept(sock_t sock, void *buf, int len, void *arg) {
    sockaddrlen_t n = len;
    int i;
    i = accept(sock, (struct sockaddr*)buf, &n);
    return i == 0 ? len : i;
}

int
sock_accept_timeout(sock_t sock, struct sockaddr *buf, sockaddrlen_t len, int timeout) {
    return sock_func_timeout(sock_func_accept, 1, sock, buf, len, timeout, 0);
}

int
sock_func_connect(sock_t sock, void *buf, int len, void *arg) {
    int i;
    i = connect(sock, (struct sockaddr*)buf, (sockaddrlen_t)len);
    return i == 0 ? len : i;
}

int
sock_connect_timeout(sock_t sock, struct sockaddr *buf, sockaddrlen_t len, int timeout) {
    return sock_func_timeout(sock_func_connect, 1, sock, buf, len, timeout, 0);
}
