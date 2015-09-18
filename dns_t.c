#include <stdio.h>
#include "sock.h"
#include "dns.h"
#include "memutil.h"

#define ETHER_HDR \
0x84, 0x34, 0x80, 0x30, 0xa3, 0xc9, 0x00, 0xff, 0x28, 0x2a, 0x45, 0x94, 0x08, 0x00

#define v1_str "request for peer.p2p"
u8 v1[] = {
    ETHER_HDR,
    0x45, 0x00,
    0x00, 0x36, 0x8b, 0xf6, 0x00, 0x00, 0x80, 0x11, 0x98, 0xfb, 0x0a, 0x63, 0x00, 0x02, 0x0a, 0x63,
    0x00, 0xfe, 0x04, 0x05, 0x00, 0x35, 0x00, 0x22, 0x25, 0x9b, 0x03, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x70, 0x65, 0x65, 0x72, 0x03, 0x70, 0x32, 0x70, 0x00,
    0x00, 0x01, 0x00, 0x01,
};

// # tcpdump -lnxev -s 1500 -i eth0 port 53

#define v2_str "10.0.0.134.1029 > 10.0.0.1.53: [udp sum ok]  888+ A? bkbox.com. (27) (ttl 128, id 5178, len 55) "
u8 v2[] =  {
    ETHER_HDR,
    0x45, 0x00, 0x00, 0x37, 0x14, 0x3a, 0x00, 0x00, 0x80, 0x11, 0x11, 0xf6, 0x0a, 0x00, 0x00, 0x86,
    0x0a, 0x00, 0x00, 0x01, 0x04, 0x05, 0x00, 0x35, 0x00, 0x23, 0x8d, 0x61, 0x03, 0x78, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x62, 0x6b, 0x62, 0x6f, 0x78, 0x03, 0x63,
    0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 01
};

#define v3_str "10.0.0.1.53 > 10.0.0.134.0x10, 0x29,: [udp sum ok]  888* 1/1/0 bkbox.com. A 209.139.221.111 (68) (DF) (ttl 64, id 0, len 96)"
u8 v3[] =  {
    ETHER_HDR,
	
    0x45, 0x00, 0x00, 0x60, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x26, 0x07, 0x0a, 0x00, 0x00, 0x01,
    0x0a, 0x00, 0x00, 0x86, 0x00, 0x35, 0x04, 0x05, 0x00, 0x4c, 0x33, 0x8a, 0x03, 0x78, 0x85, 0x80,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x05, 0x62, 0x6b, 0x62, 0x6f, 0x78, 0x03, 0x63,
    0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x78, 0x00, 0x04, 0xd1, 0x8b, 0xdd, 0x6f, 0xc0, 0x0c, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0xf4,
    0x80, 0x00, 0x0d, 0x03, 0x6e, 0x73, 0x31, 0x06, 0x68, 0x79, 0x70, 0x62, 0x75, 0x73, 0xc0, 0x12,
};

#define v4_str "10.0.0.157.2677, > 10.0.0.1.53: [udp sum ok]  0x13, 0x76,+ A? bkbox.com. (27) (ttl 128, id 0x36, 0x65,3, len 55)"
u8 v4[] = {
	ETHER_HDR,
	0x45, 0x00, 0x00, 0x37, 0x8f, 0x2d, 0x00, 0x00, 0x80, 0x11, 0x96, 0xeb, 0x0a, 0x00, 0x00, 0x9d,
	0x0a, 0x00, 0x00, 0x01, 0x0a, 0x75, 0x00, 0x35, 0x00, 0x23, 0x84, 0xf2, 0x05, 0x60, 0x01, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x62, 0x6b, 0x62, 0x6f, 0x78, 0x03, 0x63,
	0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01
};
    
#define v5_str "10.0.0.1.53 > 10.0.0.157.2677,: [udp sum ok]  1376, 1/1/1 bkbox.com. A 209.139.221.111 (84) (DF) (ttl 64, id 0, len 112)"
u8 v5[] = {
    ETHER_HDR,
    0x45, 0x00, 0x00, 0x70, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x25, 0xe0, 0x0a, 0x00, 0x00, 0x01,
    0x0a, 0x00, 0x00, 0x9d, 0x00, 0x35, 0x0a, 0x75, 0x00, 0x5c, 0x09, 0x0a, 0x05, 0x60, 0x81, 0x80,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x05, 0x62, 0x6b, 0x62, 0x6f, 0x78, 0x03, 0x63,
    0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x71, 0x00, 0x04, 0xd1, 0x8b, 0xdd, 0x6f, 0xc0, 0x0c, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0xde,
    0x5a, 0x00, 0x0d, 0x03, 0x6e, 0x73, 0x31, 0x06, 0x68, 0x79, 0x70, 0x62, 0x75, 0x73, 0xc0, 0x12,
    0xc0, 0x37, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xc9, 0x27, 0x00, 0x04, 0x8b, 0x8e, 0x40, 0x12
};

struct vec_s {
    char *ptr;
    int len;
    char *str;
};
typedef struct vec_s vec_t;
vec_t vec[] = {
    //{v1, sizeof(v1), v1_str},
    //{v2, sizeof(v2), v2_str},
    {(char*)v3, sizeof(v3), v3_str},
    {(char*)v4, sizeof(v4), v4_str},
    {(char*)v5, sizeof(v5), v5_str},
    {0, 0, 0}
};

u8 query[] = {
    0x03, 0xd1, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x70, 0x65, 0x65,
    0x72, 0x03, 0x70, 0x32, 0x70, 0x00, 0x00, 0x01, 0x00, 0x01
};



int
main(int argc, char **argv) {
    vec_t *v = vec;
    struct netpkt pkt;
    dns_t dns, dns_query, dns_reply;
    int i, n;
    char buf[4096], reply_buf[4096], buf2[4096], query_buf[4096];
    u32 addr = (1<<24) | (2<<16) | (3<<8) | (4);
    char *hostname = "home.noel.p2p.bkbox.com";
    
    debug_init(DEBUG_INFO, 0, 0);

    // parse and reply to a query
    if( 1 ) {
	dns_init(&dns_query, (char*)query, sizeof(query));
	dns_pack(&dns_query, PACK_NET2HOST);

	debug(DEBUG_INFO,  
	      ("dns query: %s\n", dns_fmt(&dns_query, buf, sizeof(buf))));
	
	i = dns_check_query_in_a(&dns_query, buf, sizeof(buf));
	debug(DEBUG_INFO,  
	      ("dns_check_query_in_a i=%d buf=%s\n", i, buf));

	dns_init(&dns_reply, reply_buf, sizeof(reply_buf));
	i = dns_pack_reply_in_a(&dns_reply, &dns_query, addr, 1);
	debug(DEBUG_INFO,  
	      ("dns_pack_reply_in_a i=%d s=%s\n"
	       ,i
	       ,dns_fmt(&dns_reply, buf, sizeof(buf))
	       ));

	dns_free(&dns_query);
	dns_free(&dns_reply);
    }

    // parse and dump raw DNS vectors
    if( 0 ) for(v=vec; v->ptr; v++) {
	netpkt_parse_ether(netpkt_init(&pkt, v->ptr, v->len));
	debug(DEBUG_INFO, 
	      ("\n%s\n%s\n%s\n"
	       ,v->str
	       ,memdump(buf, sizeof(buf), v->ptr, v->len)
	       ,netpkt_fmt(&pkt, buf2, sizeof(buf2))
	       ));
	dns_init(&dns, pkt.pkt_msg, pkt.pkt_len);
	dns_pack(&dns, PACK_NET2HOST);
	debug(DEBUG_INFO, 
	      ("%s\n", 
	       dns_fmt(&dns, buf, sizeof(buf))
	       ));
	dns_free(&dns);
    }
    
    // send a query and print reply
    if( 1 ) do {
	sock_t sock;
	struct sockaddr_in addr;
	sockaddrlen_t addrlen;
	
	i = sock_init();
	assertb(i>0);
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assertb_sockerr(sock>=0);

	if( argc > 1 ) {
	    hostname = argv[1];
	}
	dns_init(&dns_query, query_buf, sizeof(query_buf));
	dns_pack_query_in_a(&dns_query, hostname, 1);
	
	addrlen = iaddr_pack(&addr, inet_resolve("127.0.0.1"), 53);

	debug(DEBUG_INFO,  
	      ("sendto raw query to %s\n"
	       ,netpkt_ntoa(addr.sin_addr.s_addr, 0)
	       ));

	i = sendto(sock, query, sizeof(query), 0,
		   (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(i==sizeof(query));
	n = recvfrom(sock, buf, sizeof(buf), 0,
		     (struct sockaddr*)&addr, &addrlen);
	assertb_sockerr(n>0);

	debug(DEBUG_INFO,  
	      ("recvfrom raw query from %s\n", 
	       netpkt_ntoa(addr.sin_addr.s_addr, 0)));
	
	debug(DEBUG_INFO,  ("\n"));
	dns_init(&dns, buf, n);
	dns_pack(&dns, PACK_NET2HOST);
	debug(DEBUG_INFO,  
	      ("dns answer\n%s\n\n"
	       ,dns_fmt(&dns, buf2, sizeof(buf2))));
	dns_free(&dns);
	
    } while(0);

    sock_fini();
    return 0;
}

