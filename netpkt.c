/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "netpkt.h"
#include "eth_util.h"
#include "warn.h"

#define DEBUG_NETPKT "netpkt"

unsigned long
netpkt_ip_addr_src(struct netpkt *netpkt) {
    return netpkt->pkt_ip ? netpkt->pkt_ip->src : 0;
}

unsigned long 
netpkt_ip_addr_dst(struct netpkt *netpkt) {
    return netpkt->pkt_ip ? netpkt->pkt_ip->dst : 0;
}

int
netpkt_parse_ip(struct netpkt *netpkt) {
    int err=-1;
    netpkt_ip *ip;
    netpkt_tcp *tcp;
    netpkt_udp *udp;
    netpkt_icmp *icmp;
    int hdrlen;
    
    do {
	netpkt->pkt_ip = ip = (netpkt_ip*)netpkt->pkt_msg;
	hdrlen = ip_hl(ip) * 4;
	netpkt->pkt_msg += hdrlen;
	netpkt->pkt_len = ntohs(ip->len) - hdrlen;
	
	// ip opts
	netpkt->pkt_ip_opt_len = hdrlen - sizeof(*netpkt->pkt_ip);
	if( netpkt->pkt_ip_opt_len > 0) {
	    netpkt->pkt_ip_opt = (char*)(netpkt->pkt_ip+1);
	}

	switch( netpkt->pkt_ip->p ) {
	case IPPROTO_TCP:
	    netpkt->pkt_tcp = tcp = (netpkt_tcp*)(netpkt->pkt_msg);
	    hdrlen = tcp_doff(tcp) * 4;
	    netpkt->pkt_msg += hdrlen;
	    netpkt->pkt_len -= hdrlen;
	
	    // tcp opts
	    netpkt->pkt_tcp_opt_len = hdrlen - sizeof(*netpkt->pkt_tcp);
	    if( netpkt->pkt_tcp_opt_len > 0 ) {
		netpkt->pkt_tcp_opt = (char*)(netpkt->pkt_tcp+1);
	    }
	    
	    break;

	case IPPROTO_UDP:
	    netpkt->pkt_udp = udp = (netpkt_udp*)(netpkt->pkt_msg);
	    hdrlen = sizeof(*udp);
	    netpkt->pkt_msg += hdrlen;
	    netpkt->pkt_len -= hdrlen;
	    break;

	case IPPROTO_ICMP:
	    netpkt->pkt_icmp = icmp = (netpkt_icmp*)(netpkt->pkt_msg);
	    hdrlen = sizeof(*icmp);
	    netpkt->pkt_msg += hdrlen;
	    netpkt->pkt_len -= hdrlen;
	    break;

	default:
	    break;
	}

#if 0 // force the user to cksum the packet themselves
	i = netpkt_cksum_check(netpkt);
	if( i ) {
	    warn(("netpkt_parse_ip: ERROR: checksum failed for pkt! i=%d\n", i));
	    err = -2;
	    break;
	}
#endif


	err = 0;
    } while(0);
    return err;
}

int
netpkt_parse_arp(struct netpkt *netpkt) {
    int err=-1;

    do {
	netpkt->pkt_arp = (netpkt_arp*)netpkt->pkt_msg;
	netpkt->pkt_msg += sizeof(netpkt_arp);
	netpkt->pkt_len = netpkt->pkt_rawlen 
	    - (netpkt->pkt_msg - netpkt->pkt_raw);
	err = 0;
    } while(0);
    return err;
}

int 
netpkt_parse_ether(struct netpkt *netpkt) {
    int i, err=-1;
    int ether_type;

    do {
	netpkt->pkt_eth = (netpkt_ether*)netpkt->pkt_msg;
	netpkt->pkt_msg += sizeof(netpkt_ether);
	ether_type = ntohs(netpkt->pkt_eth->ether_type);

	if( ether_type > ETHERTYPE_TRAIL ) {
	    i = ether_type - ETHERTYPE_TRAIL;
	    netpkt->pkt_msg += i * 512;
	}
	switch(ether_type) {
	case ETHERTYPE_IP:
	    err = netpkt_parse_ip(netpkt);
	    break;
	case ETHERTYPE_ARP:
	    err = netpkt_parse_arp(netpkt);
	    break;
	default:
	    warn(("netpkt_parse_ether: ERROR: unknown ether_type=%d\n", ether_type));
	    err = -1;
	}
    } while(0);
    return err;
}

struct netpkt* 
netpkt_init_max(struct netpkt *pkt, char *data, int len, int max) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->pkt_raw = pkt->pkt_msg  = data;
    pkt->pkt_rawlen = pkt->pkt_len = len;
    pkt->pkt_max = max;
    return pkt;
}

struct netpkt* 
netpkt_init(struct netpkt *pkt, char *data, int len) {
    return netpkt_init_max(pkt, data, len, len);
}

char *
netpkt_icmp_code2str(int icmp_code) {
    char *s=0;
    switch(icmp_code) {
    case 0: s="ECHOREPLY"; break;
    case 3: s="DEST_UNREACH"; break;
    case 4: s="SOURCE_QUENCH"; break;
    case 5: s="REDIRECT"; break;
    case 8: s="ECHO"; break;
    case 11: s="TIME_EXCEEDED"; break;
    case 12: s="PARAMETERPROB"; break;
    case 13: s="TIMESTAMP"; break;
    case 14: s="TIMESTAMPREPLY"; break;
    case 15: s="INFO_REQUEST"; break;
    case 16: s="INFO_REPLY"; break;
    case 17: s="ADDRESS"; break;
    case 18: s="ADDRESSREPLY"; break;
    default: s=0; break;
    }
    if( !s ) {
	static char buf[32];
	s = buf;
	snprintf(buf, sizeof(buf), "%d", icmp_code);
    }
    return s;
}

char *
netpkt_ip_proto2str(int proto) {
    char *s=0;
    
    switch(proto) {
    case 0: s="HOPOPTS"; break;
    case 1: s="ICMP"; break;
    case 2: s="IGMP"; break;
    case 4: s="IPIP"; break;
    case 6: s="TCP"; break;
    case 8: s="EGP"; break;
    case 12: s="PUP"; break;
    case 17: s="UDP"; break;
    case 22: s="IDP"; break;
    case 29: s="TP"; break;
    case 41: s="IPV6"; break;
    case 43: s="ROUTING"; break;
    case 44: s="FRAGMENT"; break;
    case 46: s="RSVP"; break;
    case 47: s="GRE"; break;
    case 50: s="ESP"; break;
    case 51: s="AH"; break;
    case 58: s="ICMPV6"; break;
    case 59: s="NONE"; break;
    case 60: s="DSTOPTS"; break;
    case 92: s="MTP"; break;
    case 98: s="ENCAP"; break;
    case 103: s="PIM"; break;
    case 108: s="COMP"; break;
    case 255: s="RAW"; break;
    }
    if( !s ) {
	static char buf[32];
	s = buf;
	snprintf(buf, sizeof(buf), "%d", proto);
    }

    return s;
}

char *
netpkt_ether_type2str(int ether_type) {
    char *s=0;
    
    switch(ether_type) {
    case 0x0200: s="PUP"; break;
    case 0x0800: s="IP"; break;
    case 0x0806: s="ARP"; break;
    case 0x8035: s="REVARP"; break;
    }
    if( !s ) {
	static char buf[32];
	s = buf;
	snprintf(buf, sizeof(buf), "%d", ether_type);
    }
    return s;
}

int
netpkt_ip_opt_enum(struct netpkt *pkt, char **p, char **data, int *len) {
    return netpkt_ip_opt_next(pkt->pkt_ip_opt, pkt->pkt_ip_opt_len,
			      p, data, len);
}

int
netpkt_tcp_opt_enum(struct netpkt *pkt, char **p, char **data, int *len) {
    return netpkt_ip_opt_next(pkt->pkt_tcp_opt, pkt->pkt_tcp_opt_len,
			      p, data, len);
}

int
netpkt_ip_opt_next(char *start, int optlen, char **p, char **data, int *len) {
    int type;
    char *end = start + optlen;

    if( !*p ) {
	*p = start;
    }

    if( !*p || *p >= end ) {
	return 0;
    }
    
    type = p[0][0];
    *data = 0;
    *len = 0;
    switch(type) {
    case 0: // eop
	*p = end;
	break;
    case 1: // noop
	*p += 1;
	break;
    default:
	*len = p[0][1];
	if( *len < 2 || *p + *len > end ) {
	    *p = end;
	    *len = 0;
	    type = 0;
	}
	else {
	    *data = *p + 2;
	    *p += *len;
	    *len -= 2;
	}
	break;
    }
    return type;
}

#define BUF_ADD(a)  i=a; if(i<0) break; buf += i, len -= i;

// IP options
char *
netpkt_fmt_ip_opts(char *buf, int len, struct netpkt *pkt) {
    int dlen;
    int i, err=-1;
    char *p, *data;
    int type;
    char *orig=buf;
    
    len--; /* for terminating null */
    do {
	if( !(p = pkt->pkt_ip_opt) ) {
	    err = 0;
	    break;
	}

	BUF_ADD(snprintf(buf, len, " ip_opt=[%d ", pkt->pkt_ip_opt_len));

	p = 0;
	while( (type = netpkt_ip_opt_enum(pkt, &p, &data, &dlen)) ) {
	    switch(type) {
	    case 1: // noop
		break;
	    case 130: // security
		BUF_ADD(snprintf(buf, len, " sec"));
		break;
	    case 131: // loose source routing
		BUF_ADD(snprintf(buf, len, " lsr"));
		break;
	    case 137: // strict source routing
		BUF_ADD(snprintf(buf, len, " ssr"));
		break;
	    case 7: // record route
		BUF_ADD(snprintf(buf, len, " rr"));
		break;
	    case 136: // stream id
		BUF_ADD(snprintf(buf, len, " sid"));
		break;
	    case 68: { // timestamp
		//int oflw;
		int flg;
		char *time_p, *time_q;
		    
		//oflw = data[0] >> 4;
		flg = data[1] & 0xf;
		if( flg == 2 ) {
		    BUF_ADD(snprintf(buf, len, " error! flg=2"));
		    err = 0;
		    break;
		}
		time_p = data + 2;
		time_q = data + (data[0] - 1) * (flg ? 8 : 4);
		if( time_q > data+dlen ) {
		    BUF_ADD(snprintf(buf, len, " error! time_q>opt_q len=%ld", 
				     time_q-time_p));
		    time_q = data+dlen;
		}
		    
		while(time_p < time_q) {
		    switch(flg) {
		    case 1:
		    case 3:
			BUF_ADD(snprintf(buf, len, " time=%s:%lu", 
					 inet_ntoa(*(struct in_addr*)time_p),
					 *(unsigned long*)time_p+4));
			time_p += 8;
			err=0;
			break;
		    case 0:
		    default:
			BUF_ADD(snprintf(buf, len, " time=%lu", *(unsigned long*)time_p));
			time_p += 4;
			err=0;
			break;
		    }
		}
		err=0;
		break;
	    }

	    default:
		BUF_ADD(snprintf(buf, len, " ip(type=%d len=%d)", type, dlen));
		break;
	    }
	}
	BUF_ADD(snprintf(buf, len, "]"));

	err = 0;
    } while(0);
    *buf = 0;
    return err ? 0 : orig;
}

// TCP options
int
netpkt_fmt_tcp_opts(char *buf, int len, struct netpkt *pkt) {
    char *p, *data;
    int i, err=-1;
    int dlen;
    int type;
    char *orig = buf;

    len--; /* for terminating null */
    do {
	if( !(p = pkt->pkt_tcp_opt) ) {
	    err = 0;
	    break;
	}
	
	BUF_ADD(snprintf(buf, len, " tcp_opt=[%d", pkt->pkt_tcp_opt_len));
	
	p = 0;
	while( (type = netpkt_tcp_opt_enum(pkt, &p, &data, &dlen)) ) {
	    switch(type) {
	    case 1: // noop
		break;
	    case 2: // mss
		BUF_ADD(snprintf(buf, len, " mss=%d", ntohs(*(short*)data)));
		break;
	    case 4: // sackok
		BUF_ADD(snprintf(buf, len, " sackok"));
		break;
	    case 5: // sack
		BUF_ADD(snprintf(buf, len, " sack=("));
		for(i=0; i+4 <= dlen; i+=4) {
		    if( i > 0 ) { BUF_ADD(snprintf(buf, len, ",")); }
		    BUF_ADD(snprintf(buf, len, "%u", 
				     (tcp_seq_t)ntohl(*(unsigned long*)(data+i))));
		}
		BUF_ADD(snprintf(buf, len, ")"));
		break;
	    default:
		BUF_ADD(snprintf(buf, len, " (type=%d len=%d)", type, dlen));
		break;
	    }
	}
	
	BUF_ADD(snprintf(buf, len, "]"));
	
	err = 0;
    } while(0);
     
    *buf = 0;
   return err ? err : buf - orig;
}

char*
netpkt_fmt(struct netpkt *pkt, char *buf, int len) {
    netpkt_ether  *eth;
    netpkt_ip   *ip;
    netpkt_tcp  *tcp;
    netpkt_udp  *udp;
    netpkt_icmp *icmp;
    netpkt_arp *arp;
    char buf2[4096];
    tcp_seq_t seq_next;
    char *orig=buf;
    int i, err=-1;

    
    do {
	len--; /* for terminating null */
	BUF_ADD(snprintf(buf, len, "netpkt"));
	
	/* ethernet */
	if( (eth = pkt->pkt_eth) ) {
	    BUF_ADD(snprintf(buf, len, " ether"));
	    BUF_ADD(snprintf(buf, len, " %s >", eth_addr_str(&eth->ether_shost, buf2, sizeof(buf2))));
	    BUF_ADD(snprintf(buf, len, " %s",   eth_addr_str(&eth->ether_dhost, buf2, sizeof(buf2))));
	}

	/* IP */
	if( (ip = pkt->pkt_ip) ) {
	    
	    BUF_ADD(snprintf(buf, len, " ip_hl=%d ip_chk=%04x", ip_hl(ip), ntohs(ip->sum)));
	
	    if( (tcp = pkt->pkt_tcp) ) {
		BUF_ADD(snprintf(buf, len, " tcp"));
		BUF_ADD(snprintf(buf, len, " %s:%d ",   netpkt_ntoa(ip->src, 0), ntohs(tcp->source)));
		BUF_ADD(snprintf(buf, len, " > %s:%d ", netpkt_ntoa(ip->dst, 0), ntohs(tcp->dest)));
		BUF_ADD(snprintf(buf, len, 
				 " %c%c%c%c%c%c",
				 (tcp_syn(tcp) ? 's' : '-'),
				 (tcp_ack(tcp) ? 'a' : '-'),
				 (tcp_fin(tcp) ? 'f' : '-'),
				 (tcp_rst(tcp) ? 'r' : '-'),
				 (tcp_urg(tcp) ? 'u' : '-'),
				 (tcp_psh(tcp) ? 'p' : '-')
				 ));
		seq_next = ntohl(tcp->seq) + pkt->pkt_len;
		if( tcp_fin(tcp) ) {
		    seq_next++;
		}
		BUF_ADD(snprintf(buf, len, 
				 " seq=%lu:%lu ack=%lu win=%u tcp_chk=%04x",
				 (unsigned long)ntohl(tcp->seq), 
				 (unsigned long)seq_next, 
				 (unsigned long)ntohl(tcp->ack_seq),
				 ntohs(tcp->window), 
				 ntohs(tcp->check)
				 ));
		BUF_ADD(netpkt_fmt_tcp_opts(buf, len, pkt));
	    }
	    else if( (udp = pkt->pkt_udp) ) {
		BUF_ADD(snprintf(buf, len, " udp"));
		BUF_ADD(snprintf(buf, len, " %s:%d ",   netpkt_ntoa(ip->src, 0), (int)ntohs(udp->source)));
		BUF_ADD(snprintf(buf, len, " > %s:%d ", netpkt_ntoa(ip->dst, 0), (int)ntohs(udp->dest)));
		BUF_ADD(snprintf(buf, len, " udp_chk=%04x", ntohs(udp->check)));
	    }
	    else if( (icmp = pkt->pkt_icmp) ) {
		BUF_ADD(snprintf(buf, len, " icmp"));
		BUF_ADD(snprintf(buf, len, " %s",   netpkt_ntoa(ip->src, 0)));
		BUF_ADD(snprintf(buf, len, " > %s", netpkt_ntoa(ip->dst, 0)));
		BUF_ADD(snprintf(buf, len, " %s",   netpkt_icmp_code2str(icmp->code)));
	    }
	    else {
		BUF_ADD(snprintf(buf, len, " ip=%d", (int)ntohs(ip->p)));
		BUF_ADD(snprintf(buf, len, " %s",   netpkt_ntoa(ip->src, 0)));
		BUF_ADD(snprintf(buf, len, " > %s", netpkt_ntoa(ip->dst, 0)));
		BUF_ADD(snprintf(buf, len, " %s",   netpkt_ip_proto2str(ip->p)));
	    }
	    netpkt_fmt_ip_opts(buf, len, pkt);
	    i = strlen(buf);
	    BUF_ADD(i);
	} 
	/* ARP */
	else if( (arp = pkt->pkt_arp) ) {
	    switch(ntohs(arp->op)) {
	    case ARPOP_REQUEST:
	    case ARPOP_RREQUEST:
		BUF_ADD(snprintf(buf, len, " arp who-has %s", 
				 ip_addr_str(&arp->tpa, buf2, sizeof(buf2))));
		BUF_ADD(snprintf(buf, len, " tell %s", 
				 ip_addr_str(&arp->spa, buf2, sizeof(buf2))));
		break;
	    
	    case ARPOP_REPLY:
	    case ARPOP_RREPLY:
		BUF_ADD(snprintf(buf, len, " arp %s", 
				 ip_addr_str(&arp->spa, buf2, sizeof(buf2))));
		BUF_ADD(snprintf(buf, len, " is-at %s", 
				 eth_addr_str(&arp->sha, buf2, sizeof(buf2))));
		break;
	    
	    default: 
		BUF_ADD(snprintf(buf, len, " arp op=%d", ntohs(arp->op)));
		BUF_ADD(snprintf(buf, len, " spa=%s", 
				 ip_addr_str(&arp->spa, buf2, sizeof(buf2))));
		BUF_ADD(snprintf(buf, len, " sha=%s", 
				 ip_addr_str(&arp->sha, buf2, sizeof(buf2))));
		BUF_ADD(snprintf(buf, len, " tpa=%s", 
				 ip_addr_str(&arp->tpa, buf2, sizeof(buf2))));
		BUF_ADD(snprintf(buf, len, " tha=%s", 
				 ip_addr_str(&arp->tha, buf2, sizeof(buf2))));
	    }
	}
	else {
	    BUF_ADD(snprintf(buf, len, " ether_type %d", htons(eth->ether_type)));
	}
    
	BUF_ADD(snprintf(buf, len, " len=%d off=%ld rawlen=%d", 
			 pkt->pkt_len, pkt->pkt_msg - pkt->pkt_raw, pkt->pkt_rawlen));
	*buf = 0;
	err = 0;
    } while(0);
    
    return err ? 0 : orig;
}

int
netpkt_print(FILE *f, struct netpkt *pkt) {
    char buf[4096];
    
    netpkt_fmt(pkt, buf, sizeof(buf));
    return fprintf(f, "%s", buf);
}


int
netpkt_cksum_in(void *buf, int len) {
    unsigned short *sp = (unsigned short *)buf;
    int sum = 0;
    while( len >= 2 ) {
	sum += *sp++;
	len -= 2;
    }
    if( len > 0 ) {
	sum += ntohs((short)((((int)*(unsigned char *)sp))<<8));
    }
    return sum;
}

/* from tcpdump-3.3 */
int
netpkt_cksum_in_final(int sum) {
    sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
    sum += (sum >> 16);			/* add carry */
    sum = ~sum & 0xffff;		/* truncate to 16 bits */
    return sum;
}

/* rfc 791 */
int
netpkt_cksum_ip(struct netpkt *pkt) {
    int sum, oldsum;
    netpkt_ip *ip = pkt->pkt_ip;

    if( !ip ) {
	return 0;
    }

    oldsum = ip->sum;
    ip->sum = 0;

    sum = netpkt_cksum_in(ip, ip_hl(ip)*4);
    sum = netpkt_cksum_in_final(sum);

    ip->sum = oldsum;
    return sum;
}

/* rfc 793 */
int
netpkt_cksum_tcp(struct netpkt *pkt) {
    int sum, hdr, len, oldsum;
    netpkt_tcp *tcp = pkt->pkt_tcp;

    if( !tcp ) {
	return 0;
    }
    oldsum = tcp->check;
    tcp->check = 0;

    /* length of tcp hdr + data */
    len = ntohs(pkt->pkt_ip->len) - ip_hl(pkt->pkt_ip)*4;

    /* tcp hdr+body */
    sum = 0;
    sum += netpkt_cksum_in(tcp, len);

    /* pseudo header */
    hdr = 0;
    hdr += netpkt_cksum_in(&pkt->pkt_ip->src, 8);
    hdr += htons(pkt->pkt_ip->p);
    hdr += htons((short)len);    

    sum = netpkt_cksum_in_final(sum+hdr);

    tcp->check = oldsum;
    return sum;
}

/* rfc 768 */
int
netpkt_cksum_udp(struct netpkt *pkt) {
    int sum, oldsum, len;
    netpkt_udp *udp = pkt->pkt_udp;

    if( !udp ) {
	return 0;
    }

    oldsum = udp->check;
    udp->check = 0;

    /* length of udp hdr + data */
    len = ntohs(pkt->pkt_ip->len) - ip_hl(pkt->pkt_ip)*4;
    sum = 0;
    sum += netpkt_cksum_in(pkt->pkt_udp, len);
    /* pseudo header */
    sum += netpkt_cksum_in(&pkt->pkt_ip->src, 8);
    sum += htons(pkt->pkt_ip->p);
    sum += htons((short)len);
    sum = netpkt_cksum_in_final(sum);

    udp->check = oldsum;
    return sum;
}

int
netpkt_cksum_set(struct netpkt *pkt) {
    if( pkt->pkt_ip ) {
	if( pkt->pkt_tcp ) {
	    pkt->pkt_tcp->check = netpkt_cksum_tcp(pkt);
	}
	else if( pkt->pkt_udp ) {
	    pkt->pkt_udp->check = netpkt_cksum_udp(pkt);
	}
	pkt->pkt_ip->sum = netpkt_cksum_ip(pkt);
    }
    return 0;
}

// return 0 iff the checksums are ok
int
netpkt_cksum_check(struct netpkt *pkt) {
    int i;
    if( pkt->pkt_ip ) {
	if( pkt->pkt_ip->sum != (i=netpkt_cksum_ip(pkt)) ) {
	    warn(("netpkt_cksum_check: ERROR: bad ip_chk=%x expected=%x\n", 
		  i, pkt->pkt_ip->sum));
	    return -1;
	}
	if( pkt->pkt_tcp && pkt->pkt_tcp->check != (i=netpkt_cksum_tcp(pkt)) ) {
	    warn(("netpkt_cksum_check: ERROR: bad tcp_chk=%x expected=%x\n", 
		  i, pkt->pkt_tcp->check));
	    return -2;
	}
	if( pkt->pkt_udp && pkt->pkt_udp->check != (i=netpkt_cksum_udp(pkt)) ) {
	    warn(("netpkt_cksum_check: ERROR: bad udp_chk=%x expected=%x\n", 
		  i, pkt->pkt_udp->check));
	    return -3;
	}
    }
    pkt->pkt_cksum_ok = 1;
    return 0;
}

int
tcp_seq_diff(tcp_seq_t a, tcp_seq_t b) {
    return a - b;
}

int
netpkt_new_eth_tcp(struct netpkt *pkt, int len) {
    char *p, *start;

    // make space for ether, ip, and tcp
    p = pkt->pkt_msg;
    start = p;
    pkt->pkt_eth = (netpkt_ether*)p;
    p += sizeof(*pkt->pkt_eth);
    
    pkt->pkt_ip = (netpkt_ip*)p;
    p += sizeof(*pkt->pkt_ip);

    pkt->pkt_tcp = (netpkt_tcp*)p;
    p += sizeof(*pkt->pkt_tcp);
    pkt->pkt_msg = p;
    pkt->pkt_len = len;
    p += len;
    pkt->pkt_rawlen = p - pkt->pkt_raw;

    // start with a clean slate
    memset(start, 0, p-start);

    // ether
    pkt->pkt_eth->ether_type = htons(ETHERTYPE_IP);

    // ip
    ip_v_set(pkt->pkt_ip, 4);
    ip_hl_set(pkt->pkt_ip, 5);
    pkt->pkt_ip->len = htons((short)(p - (char*)pkt->pkt_ip));
    pkt->pkt_ip->ttl = 255;
    pkt->pkt_ip->p = IPPROTO_TCP;
    
    // tcp
    tcp_doff_set(pkt->pkt_tcp, 5);
    
    return 0;
}

int
netpkt_new_udp(struct netpkt *pkt, 
	       char *buf, int buflen,
	       char *msg, int msglen,
	       struct netpkt *replyto) {
    char *p;
    int err=-1;

    do {
	memset(buf, 0, buflen);
	netpkt_init(pkt, buf, buflen);

	// make space for ether, ip, and tcp
	p = pkt->pkt_msg;

	// ether
	pkt->pkt_eth = (netpkt_ether*)p;
	p += sizeof(*pkt->pkt_eth);
	pkt->pkt_eth->ether_type = htons(ETHERTYPE_IP);
    
	// ip
	pkt->pkt_ip = (netpkt_ip*)p;
	p += sizeof(*pkt->pkt_ip);

	// udp
	pkt->pkt_udp = (netpkt_udp*)p;
	p += sizeof(*pkt->pkt_udp);

	// message
	pkt->pkt_msg = p;
	pkt->pkt_len = msglen;
	memcpy(pkt->pkt_msg, msg, msglen);
	p += msglen;
	pkt->pkt_rawlen = p - pkt->pkt_raw;
	
	// ip (set len after message)
	ip_v_set(pkt->pkt_ip, (char)4);
	ip_hl_set(pkt->pkt_ip, (char)5);
	pkt->pkt_ip->id = 1;
	pkt->pkt_ip->len = htons((short)(p - (char*)pkt->pkt_ip));
	pkt->pkt_ip->ttl = 255;
	pkt->pkt_ip->p = IPPROTO_UDP;

	// udp len
	pkt->pkt_udp->len = htons((short)(p - (char*)pkt->pkt_udp));

	// reply
	if( replyto ) {
	    memcpy(pkt->pkt_eth->ether_shost, replyto->pkt_eth->ether_dhost, 6);
	    memcpy(pkt->pkt_eth->ether_dhost, replyto->pkt_eth->ether_shost, 6);
	    pkt->pkt_ip->src = replyto->pkt_ip->dst;
	    pkt->pkt_ip->dst = replyto->pkt_ip->src;
	    pkt->pkt_udp->source = replyto->pkt_udp->dest;
	    pkt->pkt_udp->dest   = replyto->pkt_udp->source;

	    // finally, checksum
	    netpkt_cksum_set(pkt);
	}

	err = 0;
    } while(0);
    return err;
}


int
netpkt_new_ip_opt(struct netpkt *pkt, int type, int len, char *data) {
    // unimplemented
    return -1;
}

int
netpkt_new_tcp_opt(struct netpkt *pkt, int type, int len, char *data) {
    // unimplemented
    return -1;
}

int
netpkt_new_tcp_len(struct netpkt *pkt, int len) {
    pkt->pkt_len = len;
    pkt->pkt_ip->len = htons((short)((pkt->pkt_msg + len) - (char*)pkt->pkt_ip));
    pkt->pkt_rawlen = (pkt->pkt_msg + len) - (char*)pkt->pkt_raw;
    return 0;
}

int
netpkt_new_tcp_flags(struct netpkt *pkt, int flags) {
    netpkt_tcp *tcp = pkt->pkt_tcp;
    tcp_syn_set(tcp, (flags & TH_SYN)  ? 1 : 0);
    tcp_ack_set(tcp, (flags & TH_ACK)  ? 1 : 0);
    tcp_fin_set(tcp, (flags & TH_FIN)  ? 1 : 0);
    tcp_rst_set(tcp, (flags & TH_RST)  ? 1 : 0);
    tcp_urg_set(tcp, (flags & TH_URG)  ? 1 : 0);
    tcp_psh_set(tcp, (flags & TH_PUSH) ? 1 : 0);
    return 0;
}

#if 0
int
netpkt_hdr_copy(struct netpkt *pkt_dst, struct netpkt *pkt_src) { 
    // construct a raw template
    netpkt_new_ether_tcp(pkt, 0);
    memcpy(pkt->pkt_eth, old->pkt_eth, 2*6);
    memcpy(&pkt->pkt_ip.ip_src, &old->pkt_ip.ip_src, 2*4);
    memcpy(&pkt->pkt_tcp.source, &&pkt->pkt_tcp.source, 2*2);

    // add the necessary bits and final checksum
    netpkt_new_tcp_len(pkt, len);
    pkt->pkt_tcp.win = win;
    pkt->pkt_tcp.ack_seq = ack;
    pkt->pkt_tcp.seq = seq;
    pkt->pkt_tcp.flags = (...);
    memcpy(pkt->pkt_msg, data, len);
    netpkt_cksum_set(pkt);
}
#endif

PktBuf* pktbuf_init(PktBuf *pktbuf) {
    pktbuf->from = 0;
    pktbuf->len = 0;
    pktbuf->max = sizeof(pktbuf->buf);
    netpkt_init(&pktbuf->pkt, pktbuf->buf, pktbuf->len);
    return pktbuf;
}

PktBuf*
pktbuf_new() {
    PktBuf *pktbuf=0;
    do {
	pktbuf = (PktBuf*)malloc(sizeof(*pktbuf));
	debug(DEBUG_INFO, ("pktbuf_new: pktbuf=%p\n", pktbuf));
	assertb(pktbuf);
	pktbuf_init(pktbuf);
    } while(0);
    return pktbuf;
}

void
pktbuf_free(PktBuf *pktbuf) {
    debug(DEBUG_INFO, ("pktbuf_free: pktbuf=%p\n", pktbuf));
    free(pktbuf);
}

void
pktbuf_free_pool(void *pktbuf, void *arg) {
    pktbuf_free((PktBuf*)pktbuf);
}

tcp_seq_t
tcp_seq_next(struct netpkt *pkt) {
    netpkt_tcp *tcp = pkt->pkt_tcp;
    tcp_seq_t seq;

    if( !tcp ) {
	return 0;
    }
    
    seq = ntohl(tcp->seq) + pkt->pkt_len;
    if( tcp_syn(tcp) ) {
	seq++;
    }
    if( tcp_fin(tcp) ) {
	seq++;
    }
    return seq;
}

int
netpkt_tcp_remove_sack(struct netpkt *pkt) {
    netpkt_tcp *tcp;
    int n, type;
    char *p, *data;
    int ret = 0;

    do {
	tcp = pkt->pkt_tcp;
	if( !(tcp && tcp_syn(tcp) && pkt->pkt_tcp_opt) ) {
	    break;
	}

	p = 0;
	while( (type = netpkt_tcp_opt_enum(pkt, &p, &data, &n)) ) {
	    if( type == TCP_OPT_SACKOK ) {
		/* overwrite with noop */
		memset(data-2, 1, n+2);
		ret = 1;
	    }
	}
    } while(0);
    return ret;
}

// packets grow up as each layer prepends its header.
//
// eth = pkt->pkt_eth = (struct etherhdr*)netpkt_add_hdr(pkt, sizeof(*pkt->pkt_eth));
void*
netpkt_add_hdr(struct netpkt *pkt, int len) {
    pkt->pkt_raw -= len;
    pkt->pkt_rawlen += len;
    return pkt->pkt_raw;
}


char *
netpkt_ntoa(u32 addr, char *buf) {
    static char sbuf[IP_ADDR_STR_MAX];
    if( !buf ) buf = sbuf;
    return ip_addr_str(&addr, buf, IP_ADDR_STR_MAX);
}


char*
netpkt_proto_fmt(int proto, char *buf, int len) {
    char *p = 0;
    switch(proto) {
    case IPPROTO_ICMP: p = "icmp"; break;
    case IPPROTO_TCP: p = "tcp"; break;
    case IPPROTO_UDP: p = "udp"; break;
    default: p = 0; break;
    }
    if( p ) {
	snprintf(buf, len, "%s", p);
    }
    else {
	snprintf(buf, len, "unknown(%d)", proto);
    }
    return buf;
}

/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

