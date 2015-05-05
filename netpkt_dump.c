#include <stdio.h>

#include "sock.h"
#include "sock_raw.h"
#include "netpkt.h"
#include "memutil.h"
#include "debug.h"

int
main(int argc, char **argv) {
    int sock;
    char buf[16384], buf_pkt[4096], *p;
    int i, n;
    struct netpkt pkt;
    char *ifname = "eth0";

    do {
	if( argc>1 ) {
	    ifname = argv[1];
	}


	sock = sock_openraw(ifname, 1);
	assertb_sockerr(sock>=0);
	
	while(1) {
	    n = recv(sock, buf_pkt, sizeof(buf_pkt), 0);
	    assertb_sockerr(n>0);

	    i = netpkt_parse_ether(netpkt_init(&pkt, buf_pkt, n));
	    if( i >= 0 ) {
		i = netpkt_cksum_check(&pkt);
	    }

	    if( i < 0 ) {
		printf("unparsed packet:\n%s\n", 
		       memdump(buf, sizeof(buf), buf_pkt, n));
		continue;
	    }
	    
#if 0
	    /* only 9200/udp */
	    if( !(pkt.pkt_udp 
		  && (ntohs(pkt.pkt_udp->source) == 9200 
		      || ntohs(pkt.pkt_udp->dest) == 9200)) ) {
		continue;
	    }

	    if( pkt.pkt_tcp 
		&& (ntohs(pkt.pkt_tcp->source) == 22 
		    || ntohs(pkt.pkt_tcp->dest) == 22) ) {
		continue;
	    }
#endif


	    p = buf;
	    netpkt_fmt(&pkt, p, sizeof(buf) - (p-buf));
	    i = strlen(p);
	    assertb(i>=0);
	    p += i;
	    *p = '\n';
	    p++;
	    memdump(p, sizeof(buf) - (p-buf), pkt.pkt_msg, pkt.pkt_len);
	    p += strlen(p);
	
	    printf("%s\n", buf);
	}
	
    } while(0);
    return 0;
}
