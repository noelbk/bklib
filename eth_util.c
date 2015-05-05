/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#include "config.h"
#include "eth_util.h"
#include <string.h>
#include <stdio.h>

char *
dot_addr_str(const unsigned char *addr, char *buf, int buflen,
	     int dots, char *templ, char *sep)
{
    int i, j, n, sep_len;
    char *p;
    
    p = buf;
    n = buflen; 
    sep_len = strlen(sep);
    for(i=0; i<dots; i++) {
	if( i>0 ) {
	    if( n < sep_len ) { break; }
	    strncat(p, sep, n);
	    p += sep_len;
	    n -= sep_len;
	}
	j = snprintf(p, n, templ, (unsigned int)addr[i]);
	if( j < 0 ) { break; }
	p += j;
	n -= j;
    }
    if( n>0 ) { 
	*p++ = 0; 
    }
    else {
	buf[buflen-1] = 0;
    }

    return buf;
}

#define ETH_ADDR_STR_MAX (6*2+5+1)
char *
eth_addr_str(const void *addr, char *buf, int buflen)
{
    return dot_addr_str((const unsigned char*)addr, buf, buflen, 
			6, "%02x", ":");
}

#define IP_ADDR_STR_MAX (6*2+5+1)
char *
ip_addr_str(const void *addr, char *buf, int buflen)
{
    return dot_addr_str((const unsigned char*)addr, buf, buflen,
			4, "%d", ".");
}

/* from tcpdump-3.3 */
int
in_cksum(const netpkt_ip *ip)
{
    register const u_short *sp = (u_short *)ip;
    register u32 sum = 0;
    register int count;

    /*
     * No need for endian conversions.
     */
    for (count = ip_hl(ip) * 2; --count >= 0; )
	sum += *sp++;
    while (sum > 0xffff)
	sum = (sum & 0xffff) + (sum >> 16);
    sum = ~sum & 0xffff;

    return (sum);
}

#define ETH_TYPE_STR_MAX (100)
char *
eth_type_str(unsigned int ether_type, char *buf, int n)
{
    char *s = 0;
    switch(ether_type) {
    case ETHERTYPE_PUP:    s = "pup"; break;
    case ETHERTYPE_IP:     s = "ip"; break;
    case ETHERTYPE_ARP:    s = "arp"; break;
    case ETHERTYPE_REVARP: s = "revarp"; break;
    default:
	if( ether_type > ETHERTYPE_TRAIL ) {
	    s = "trail";
	}
    }
    if( s ) {
	snprintf(buf, n, "%s", s);
    }
    else {
	snprintf(buf, n, "%3u", (unsigned int)ether_type);
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

