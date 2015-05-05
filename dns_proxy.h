/*
  dns_proxy.h - forward DNS requests to servers and relay replies back
  Noel Burton-Krahn
  Feb 9, 2005

*/


#ifndef DNS_PROXY_H_INCLUDED
#define DNS_PROXY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "fdselect.h"
#include "mstime.h"
#include "sock.h"

#define DNS_PROXY_EXPIRE_REQ 10 /* max time for forwarded reply */
#define DNS_PROXY_EXPIRE_POLL 1

struct dns_proxy_t;
typedef struct dns_proxy_t dns_proxy_t;

dns_proxy_t*
dns_proxy_new(fdselect_t *fdselect, 
	      struct sockaddr_in *forward_addrs, int naddrs);

void
dns_proxy_delete(dns_proxy_t *proxy);

int
dns_proxy_poll(dns_proxy_t *proxy, mstime_t mstime);

// forward a DNS request to my servers, and get ready to relay reply
// back to fromaddr
int
dns_proxy_forward(dns_proxy_t *proxy, char *buf, int len, 
		  sock_t orig_sock, struct sockaddr_in *orig_addr);

// receive a reply from an external DNS server and pass reply to a
// callback function
void
dns_proxy_select_recv(fdselect_t *sel, fd_t sock, int events, void *arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DNS_PROXY_H_INCLUDED

