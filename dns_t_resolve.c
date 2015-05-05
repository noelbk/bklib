/* dns_t_resolve - test resolving names */

#include <stdio.h>
#include "sock.h"
#include "dns.h"
#include "memutil.h"
#include "defutil.h"

int
main(int argc, char **argv) {
    char *ns, *hostname;
    sock_t sock;
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    char buf_query[4096];
    char buf_reply[4096];
    char buf_out[4096];
    char buf[4096];
    dns_rr_t hosts[100];
    int i, n, err=-1;
    int id;
    dns_t dns_reply;

    do {
	debug_init(DEBUG_INFO, 0, 0);
	sock_init();

	if( argc <= 2 ) {
	    fprintf(stderr, "USAGE: %s nameserver hostname\n", argv[0]);
	    break;
	}
	ns = argv[1];
	hostname = argv[2];

	i = dns_resolve_query(buf_query, sizeof(buf_query), hostname, 977);
	assertb(i>=0);
	memdump(buf_out, sizeof(buf_out), buf_query, i);
	fprintf(stderr, "query: len=%d\n%s\n", i, buf_out);
	
	/* send the query to ns*/
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assertb_sockerr(sock>=0);
	addrlen = iaddr_pack(&addr, inet_resolve(ns), 53);
	assertb(addrlen>0);
	n = sendto(sock, buf_query, i, 0,
		   (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(n==i);

	/* receive a response */
	n = recvfrom(sock, buf_reply, sizeof(buf_reply), 0,
		     (struct sockaddr*)&addr, &addrlen);
	assertb_sockerr(n>0);

	debug(DEBUG_INFO,
	      ("recvfrom raw query from %s\n%s\n"
	       , netpkt_ntoa(addr.sin_addr.s_addr, 0)
	       , memdump(buf_out, sizeof(buf_out), buf_reply, n)
	       ));
	
	dns_init(&dns_reply, buf_reply, n);
	dns_pack(&dns_reply, PACK_NET2HOST);
	debug(DEBUG_INFO,  
	      ("dns answer %s\n"
	       ,dns_fmt(&dns_reply, buf, sizeof(buf))));
	
	n = dns_resolve_reply(buf_reply, n, &id, hosts, NELTS(hosts));
	for(i=0; i<n; i++) {
	    fprintf(stderr, "hosts[%d]: %s=%s\n", i,
		    hosts[i].name, netpkt_ntoa(hosts[i].rdata.a.addr, 0));
	}
    } while(0);

}

