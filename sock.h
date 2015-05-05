#ifndef SOCK_H_INCLUDED
#define SOCK_H_INCLUDED

// these function are implemented in os_{unix,win32}.c 
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
#if OS & OS_UNIX

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>

#include "sock_send_fd.h"
#include "config.h"
#include "itypes.h"

#define SOCK_HAS_AF_UNIX
#define SOCK_SHUTDOWN_RECV 0
#define SOCK_SHUTDOWN_SEND 1
typedef socklen_t sockaddrlen_t;

//---------------------------------------------------------------------
#elif OS & OS_WIN32
#include <io.h>

#include "bkwin32.h"
// included in bkwin32.h
//#include <winsock2.h>

#define SOCK_SHUTDOWN_RECV 0 // SD_RECEIVE in <winsock2.h>
#define SOCK_SHUTDOWN_SEND 1 // SD_SEND

#undef SOCK_HAS_AF_UNIX

typedef int sockaddrlen_t;

#endif // OS
//---------------------------------------------------------------------

typedef int sock_t;

// INADDR_ANY and INADDR_LOOPBACK are defined in host-order.  To avoid
// wrapping these macros in htonl all the time, all ip aruments are
// returned and passed in host-order

// returns the host's ip address in host order, or INADDR_NONE
long inet_resolve(const char *ip);

// ip and port are assumed to be in host -order and will be convertexd to network-order
int iaddr_pack(struct sockaddr_in *addr, long ip, int port);

// formats addr as ip:port
char *iaddr_fmt(struct sockaddr_in *addr, char *buf, int len);

// converts ip:port to a sockaddr_in.  returns 0 iff ok.
int iaddr_parse(struct sockaddr_in *addr, char *ip_port);

char *iaddr_ntoa(unsigned long i, char *buf);

// returns address in host format
unsigned long iaddr_aton(char *buf);


#ifdef SOCK_HAS_AF_UNIX
int uaddr_pack(struct sockaddr_un *addr, const char *path);
int uaddr_len(const struct sockaddr_un *addr);
#endif // SOCK_HAS_AF_UNIX


// returns in host order
#define IADDR(a,b,c,d) (((a&0xff)<<24 | (b&0xff)<<16 | (c&0xff)<<8 | (d&0xff)))

// Windows requires WSAStartup().  returns >0 iff ok
int sock_init();

// WSACleanup (or such like)
int sock_fini();

// is this thing really a socket?
int sock_valid(sock_t sock);

// return errno or WSAGetLastError()
int sock_errno(sock_t sock);

// makes the socket non-blocking.  returns >0 iff O_NONBLOCK was
// previously set, <0 on error
int sock_nonblock(sock_t sock, int nonblock);

// sets the close_on_exec flag
int sock_close_on_exec(sock_t sock, int close);

// returns 1 iff the last write, send, or sendto on fd failed (ret<0)
// with EAGAIN or EWOULDBLOCK
int sock_wouldblock(sock_t sock, int ret);

// returns 1 iff the (tcp) socket is already connected (EISCONN)
int sock_isconn(sock_t sock, int ret);

int sock_pair(sock_t sock[2]);

int sock_close(sock_t sock);

// returns the number of bytes available for reading, from ioctl(FIONBIO)
int sock_read_avail(sock_t sock);

// converts WSAERROR => "WSAERROR"
char* sock_err2str(int err);

// returns >0 something read, =0 closed, <0 error
typedef int (*sock_func_t)(sock_t sock, void *buf, int len, void *arg);

// returns length read/written <len on timeout, <0 on error
int
sock_func_timeout(sock_func_t func, int select_read,
		   sock_t sock, void *buf, int len, int ms_timeout, void *arg);

int sock_func_recv(sock_t sock, void *buf, int len, void *arg);
int sock_func_send(sock_t sock, void *buf, int len, void *arg);
int sock_func_accept(sock_t sock, void *buf, int len, void *arg);
int sock_func_connect(sock_t sock, void *buf, int len, void *arg);

int sock_recv_timeout(sock_t sock, void *buf, int len, int ms_timeout);
int sock_send_timeout(sock_t sock, void *buf, int len, int ms_timeout);
int sock_sendto_timeout(sock_t sock, void *buf, int len, 
			struct sockaddr *addr, 
			sockaddrlen_t addrlen,
			int timeout);

int sock_accept_timeout(sock_t sock, struct sockaddr *buf, sockaddrlen_t len, int timeout);
int sock_connect_timeout(sock_t sock, struct sockaddr *buf, sockaddrlen_t len,  int timeout);

static inline
int
sock_recv(sock_t sock, char *buf, int len) {
    return recv(sock, buf, len, 0);
}

static inline
int
sock_send(sock_t sock, char *buf, int len) {
    return send(sock, buf, len, 0);
}

static inline 
int
sock_shutdown(sock_t sock, int what) {
    return shutdown(sock, what);
}


static inline
int
ipaddr_netmask(unsigned long addr, unsigned long network, unsigned long netmask) {
    return (addr & netmask) == (network & netmask);
}

static inline
int
ipaddr_netbits(unsigned long addr, unsigned long network, int netmask_bits) {
    return ipaddr_netmask(addr, network, ~((1<<netmask_bits)-1));
}

// http://www.ietf.org/rfc/rfc1918.txt
static inline
int
ipaddr_routable(unsigned long ipaddr) {
    return !(ipaddr == 0
	     || ipaddr == INADDR_LOOPBACK
	     || ipaddr_netbits(ipaddr, IADDR(127,0,0,0), 8)
	     || ipaddr_netbits(ipaddr, IADDR(10,0,0,0), 8)
	     || ipaddr_netbits(ipaddr, IADDR(192,168,0,0), 16)
	     || ipaddr_netbits(ipaddr, IADDR(172,16,0,0), 16)
	     );
 }

#ifdef __cplusplus
}
#endif

#endif // SOCK_H_INCLUDED
