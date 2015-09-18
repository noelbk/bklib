/*
  dns_proxy.c - forward DNS requests to servers and relay replies back
  Author: Noel Burton-Krahn
  Created: Feb 8, 2005

*/

#include <stdlib.h>

#include "sock.h"
#include "fdselect.h"
#include "mstime.h"
#include "debug.h"
#include "hash.h"
#include "array.h"
#include "dns.h"
#include "rand.h"

#include "dns_proxy.h"

#define DNS_PROXY_EXPIRE_REQ 10 /* max time for forwarded reply */
#define DNS_PROXY_EXPIRE_POLL 1

// used by dns_proxy to send replies to teh return address with
// original dns id
typedef struct {
    int                id;        /* proxied id */
    int                orig_id;   /* original request id */
    struct sockaddr_in orig_addr; /* return address */
    sock_t             orig_sock;
    mstime_t           expire;
    char               hostname[1024]; /* debugging */
} dns_proxy_req_t;

struct dns_proxy_t {
    sock_t      sock;
    fdselect_t *fdselect;
    array_t     forward_addrs;
    hash_t      req_hash_id;
    mstime_t    expire;
    mstime_t    mstime;
};

void
dns_proxy_delete(dns_proxy_t *proxy) {
    if( proxy ) {
	array_clear(&proxy->forward_addrs);
	hash_clear(&proxy->req_hash_id);
	if( proxy->sock >= 0 ) {
	    fdselect_unset(proxy->fdselect, proxy->sock, 0);
	    sock_close(proxy->sock);
	}
	free(proxy);
    }
}

dns_proxy_t*
dns_proxy_new(fdselect_t *fdselect,
	      struct sockaddr_in *forward_addrs, int naddrs) {
    dns_proxy_t *proxy = 0;
    int i, err=-1;
    struct sockaddr_in *paddr;

    do {
	proxy = calloc(1, sizeof(*proxy));
	assertb(proxy);

	i = array_init(&proxy->forward_addrs, sizeof(struct sockaddr_in), 0);
	assertb(i>=0);

	i = hash_init(&proxy->req_hash_id, hash_hash_int, hash_cmp_int, 
		  0, sizeof(dns_proxy_req_t), 0);
	assertb(i>=0);

	paddr = (struct sockaddr_in *)array_add(&proxy->forward_addrs, naddrs);
	assertb(paddr);
	memcpy(paddr, forward_addrs, naddrs * sizeof(*paddr));

	// a socket to send forwarded requests and receive replies
	proxy->sock = socket(AF_INET, SOCK_DGRAM, 0);
	assertb(proxy->sock>=0);

	proxy->fdselect = fdselect;
	fdselect_set(proxy->fdselect, proxy->sock, FDSELECT_READ, 
		     dns_proxy_select_recv, proxy);
	
	err = 0;
    } while(0);
    if( err ) {
	dns_proxy_delete(proxy);
	proxy = 0;
    }
    return proxy;
}

int
dns_proxy_foreach_expire(hash_t *hash, hash_node_t *node, void *arg) {
    dns_proxy_t *proxy = (dns_proxy_t*)arg;
    dns_proxy_req_t *req = (dns_proxy_req_t*)node->node_val;
    if( proxy->mstime > req->expire ) {
	debug(DEBUG_INFO, 
	      ("dns_proxy_foreach_expire:"
	       " expiring request id=%d for hostname=%s\n"
	       ,req->id, req->hostname));
	hash_free(hash, node);
    }
    return 0;
}

int
dns_proxy_poll(dns_proxy_t *proxy, mstime_t mstime) {
    int err=-1;
    do {
	proxy->mstime = mstime;
	if( proxy->mstime < proxy->expire ) {
	    err = 0;
	    break;
	}
	proxy->expire = proxy->mstime + DNS_PROXY_EXPIRE_POLL;
	err = hash_foreach(&proxy->req_hash_id, dns_proxy_foreach_expire, proxy);
    } while(0);
    return err;
}

// forward a DNS request to my servers, and get ready to relay reply
// back to fromaddr
int
dns_proxy_forward(dns_proxy_t *proxy, char *buf, int len, 
		  sock_t orig_sock, struct sockaddr_in *orig_addr) {
    dns_proxy_req_t req;
    hash_node_t *node=0;
    int i, n, err=-1;
    struct sockaddr_in *paddr = 0;
    dns_t dns;

    memset(&dns, 0, sizeof(dns));
    do {
	// parse DNS
	i = dns_init(&dns, buf, len);
	assertb(i>=0);
	i = dns_pack(&dns, PACK_NET2HOST);
	assertb(i>=0);

	// save old request
	memset(&req, 0, sizeof(req));
	req.expire = mstime() + DNS_PROXY_EXPIRE_REQ;
	req.orig_sock = orig_sock;
	req.orig_addr = *orig_addr;
	req.orig_id = dns.id;
	i = dns_check_query_in_a(&dns, req.hostname, sizeof(req.hostname));

	/* get a new unique id */
	for(i=0; i<1000; i++) {
	    req.id = rand_u32(0xffff);
	    node = hash_get(&proxy->req_hash_id, (void*)(intptr_t)req.id);
	    if( !node ) break;
	}
	assertb(i<1000);

	debug(DEBUG_INFO, 
	      ("dns_proxy_forward: request for %s orig_id=%d proxy_id=%d addr=%s\n"
	       ,req.hostname, dns.id, req.id,
	       iaddr_fmt(orig_addr, 0, 0)));

	// put in new id
	dns.id = req.id;

	// save request by new id
	node = hash_put(&proxy->req_hash_id, (void*)(intptr_t)req.id, &req);
	assertb(node);
	
	// pack it all up for transmssion
	i = dns_pack(&dns, PACK_HOST2NET);
	assertb(i>=0);
	
	// forward to dns servers
	for(i=array_count(&proxy->forward_addrs)-1,
		paddr = (struct sockaddr_in*)array_get(&proxy->forward_addrs, -1)
		; i>=0; i--, paddr--) {
	    n = sendto(proxy->sock, dns.buf, dns.buflen, 0,
		       (struct sockaddr*)paddr, sizeof(*paddr));
	    assertb(n==(int)dns.buflen);
	    
	    debug(DEBUG_INFO,
		  ("forwarded req id=%x for host=%s to addr=%s buflen=%d sendto=%d\n",
		   req.id,
		   req.hostname, 
		   iaddr_fmt(paddr, 0, 0),
		   dns.buflen,
		   n
		   ));
	}
	err = 0;
    } while(0);

    if( err ) {
	/* instantly expire request on error */
	if( node ) {
	    hash_free(&proxy->req_hash_id, node);
	}
    }
    dns_free(&dns);

    return err;
}

// receive a reply from an external DNS server and pass reply to a
// callback function
void
dns_proxy_select_recv(fdselect_t *sel, fd_t sock, int events, void *arg) {
    dns_proxy_t *proxy = (dns_proxy_t *)arg;
    char buf[4096];
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    int i, n; //, err=-1;
    dns_t dns;
    hash_node_t *node=0;
    dns_proxy_req_t *req;

    memset(&dns, 0, sizeof(dns));
    do {
	// receive DNS packet
	addrlen = sizeof(addr);
	n = recvfrom(sock, buf, sizeof(buf), 0, 
		     (struct sockaddr*)&addr, &addrlen);
	assertb_sockerr(n>0);

	// parse DNS
	i = dns_init(&dns, buf, n);
	assertb(i>=0);
	i = dns_pack(&dns, PACK_NET2HOST);
	assertb(i>=0);

	debug(DEBUG_INFO,
	      ("received reply from addr=%s reply_id=%x\n",
	       iaddr_fmt(&addr, 0, 0), dns.id));

	// find a proxied request by the dns id
	node = hash_get(&proxy->req_hash_id, (void*)(long)dns.id);
	if( !node ) {
	    //err = 0;
	    break;
	}
	req = (dns_proxy_req_t*)node->node_val;

	debug(DEBUG_INFO,
	      ("relaying reply back to dns_id=%d addr=%s hostname=%s\n",
	       req->orig_id, iaddr_fmt(&req->orig_addr, 0, 0), req->hostname));

	// put back the original request id
	dns.id = req->orig_id;
	i = dns_pack(&dns, PACK_HOST2NET);
	assertb(i>=0);

	i = sendto(req->orig_sock, dns.buf, dns.buflen, 0,
		   (struct sockaddr*)&req->orig_addr, sizeof(req->orig_addr));
	assertb(i==dns.buflen);
	//err = 0;
    } while(0);

    if( node ) {
	hash_free(&proxy->req_hash_id, node);
    }

    dns_free(&dns);
}

