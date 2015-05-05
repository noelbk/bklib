/*
  dns_proxy_t.c - test dns_proxy.c
  Noel Burton-Krahn
  Feb 9, 2005

  
*/
#include <stdio.h>
#include <stdlib.h>

#include "dns_proxy.h"
#include "sock.h"
#include "debug.h"
#include "defutil.h"

void
select_recv_proxy(fdselect_t *sel, fd_t sock, int events, void *arg) {
    dns_proxy_t *proxy = (dns_proxy_t *)arg;
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    int i, err=-1;
    char buf[4096];

    do {
	addrlen = sizeof(addr);
	i = recvfrom(sock, buf, sizeof(buf), 0, 
		     (struct sockaddr*)&addr, &addrlen);
	assertb_sockerr(i>=0);
	dns_proxy_forward(proxy, buf, i, sock, &addr);
	err = 0;
    } while(0);
}

int
usage() {
    fprintf(stderr, 
	    "usage: dns_proxy_t port addr:port...\n"
	    "proxies DNS requests received on port to DNS servers at addr:port\n");
    return 1;
}
int
main(int argc, char **argv) {
    dns_proxy_t *proxy=0;
    fdselect_t fdselect;
    struct sockaddr_in addrs[100], addr;
    sockaddrlen_t addrlen;
    int naddrs = 0;
    int port;
    char *p;
    int i, err=-1;
    sock_t sock=-1;

    do {
	debug_init(DEBUG_INFO, 0, 0);
	sock_init();
	
	/* arguments */

	if( argc < 3 ) {
	    exit(usage());
	}

	port = strtoul(argv[1], &p, 0);
	assertb(port && p>argv[1]);

	naddrs=0;
	for(i=2; i<argc && naddrs < NELTS(addrs); i++) {
	    iaddr_parse(&addrs[naddrs++], argv[i]);
	}
	assertb(naddrs>0);

	fdselect_init(&fdselect);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assertb(sock>=0);

	addrlen = iaddr_pack(&addr, INADDR_ANY, port);
	//i = 1;
	//i = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
	//	       (void*)&i, sizeof(i));
	//assertb_sockerr(i == 0);
	i = bind(sock, (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(i>=0);

	proxy = dns_proxy_new(&fdselect, addrs, naddrs);
	assertb(proxy);

	i = fdselect_set(&fdselect, sock, FDSELECT_READ, 
			 select_recv_proxy, proxy);
	assertb(i>=0);

	while(1) {
	    fdselect_select(&fdselect, 1000);
	    dns_proxy_poll(proxy, mstime());
	}
	err = 0;
    } while(0);
    sock_close(sock);
    return err;
}
