/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

/* netpkt.h - functions for parseing and printing network packets

   Noel Burton-Krahn <noel@burton-krahn.com>
   Feb 12, 2002

*/

#ifndef NETPKT_H_INCLUDED
#define NETPKT_H_INCLUDED

#include "netpkt_inet.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IP(a,b,c,d) htonl(((a)<<24)|((b)<<16)|((c)<<8)|(d))

typedef struct netpkt {
    char  *pkt_raw;    // the original packet
    int    pkt_rawlen; // the original packet length
    char  *pkt_msg;    // pointer to the last unparsed portion of the packet
    int    pkt_len;    // the length of the last unparsed portion of the packet
    int    pkt_max;    // max len this packet can grow to
    int    pkt_cksum_ok;     // true iff the checksums are ok
    int    pkt_flags;

    // pkt_raw is embedded in a larger buffer, and netpkt_new_hdr
    // decrements pkt_raw until it reaches pkt_min
    char   *pkt_min;   
    char   *put_max;

    netpkt_ether *pkt_eth;
    netpkt_arp    *pkt_arp;
    netpkt_ip           *pkt_ip;
    netpkt_tcp       *pkt_tcp;
    netpkt_udp       *pkt_udp;
    netpkt_icmp      *pkt_icmp;

    // IP and TCP options
    char   *pkt_ip_opt;
    int     pkt_ip_opt_len;
    char   *pkt_tcp_opt;
    int     pkt_tcp_opt_len;

    // the difference between this seq and the expected value.
    // computed by IpConnTrack::track_pkt.
    int    pkt_tcp_seq_diff; 
} netpkt;

#ifndef TH_FIN 
#define TH_FIN	0x01
#define TH_SYN	0x02
#define TH_RST	0x04
#define TH_PUSH	0x08
#define TH_ACK	0x10
#define TH_URG	0x20
#endif // TH_XXX

enum ip_opt_types {
    IP_OPT_END=0,
    IP_OPT_NOOP=1, 
    IP_OPT_SEC=130,
    IP_OPT_LSR=131,
    IP_OPT_SSR=137,
    IP_OPT_RR=7,
    IP_OPT_SID=136,
    IP_OPT_TIME=68
};

enum tcp_opt_types {
    TCP_OPT_MSS=2,
    TCP_OPT_SACKOK=4,
    TCP_OPT_SACK=5
};

struct netpkt* netpkt_init_max(struct netpkt *pkt, char *data, int len, int max);

struct netpkt* netpkt_init(struct netpkt *pkt, char *pkt_data, int pkt_len);

int netpkt_parse_ether(struct netpkt *netpkt);

int netpkt_parse_ip(struct netpkt *netpkt);

int netpkt_parse_arp(struct netpkt *netpkt);

// find and return the ip src or dst address from a parsed pkt (network order)
unsigned long netpkt_ip_addr_src(struct netpkt *netpkt);
unsigned long netpkt_ip_addr_dst(struct netpkt *netpkt);

char *netpkt_icmp_code2str(int icmp_code);

char *netpkt_ip_proto2str(int proto);

char *netpkt_fmt(struct netpkt *pkt, char *buf, int len);

char*
netpkt_proto_fmt(int proto, char *buf, int len);

/* compute and set all required checksums in a packet */

int netpkt_cksum_set(struct netpkt *pkt);

int netpkt_cksum_ip(struct netpkt *pkt);

int netpkt_cksum_tcp(struct netpkt *pkt);

int netpkt_cksum_udp(struct netpkt *pkt);

/* compute a cksum a la rfc791.  Returns an int you can keep adding
   to.  Call netpkt_cksum_in_final(sum) to get the official cksum in
   network order */
int netpkt_cksum_in(void *buf, int len);
int netpkt_cksum_in_final(int sum);

// return 0 iff the checksums are ok
int netpkt_cksum_check(struct netpkt *pkt);

typedef u32 tcp_seq_t;

/* returns a - b mod 2**32, can be < 0 */
int tcp_seq_diff(tcp_seq_t a, tcp_seq_t b);

/* returns the next expected seq after this pkt, or 0 if not tcp */
tcp_seq_t tcp_seq_next(struct netpkt *pkt);

int netpkt_ip_opt_enum(struct netpkt *pkt, char **p, char **data, int *len);

int netpkt_tcp_opt_enum(struct netpkt *pkt, char **p, char **data, int *len);

/* advances *p, sets *data and *dlen, returns type, 0 at end of opts.  */
int netpkt_ip_opt_next(char *start, int optlen, char **p, char **data, int *len);

/* returns 1 iff the sack option was removed.  You'll need to call
   netpkt_cksum_set() afterwards */
int netpkt_tcp_remove_sack(struct netpkt *pkt);

char *
netpkt_ntoa(u32 addr, char *buf);

//---------------------------------------------------------------------
typedef struct _PktBuf {
    int from; // HSM_PKT_FROM_*
    int max;
    int len;
    struct netpkt pkt;
    char buf[2048];
} PktBuf;

PktBuf* pktbuf_init(PktBuf *pktbuf);

PktBuf* pktbuf_new();

void pktbuf_free(PktBuf *pktbuf);
void pktbuf_free_pool(void *pktbuf, void *arg);

void pktbuf_free_ppktbuf(void *p);

//---------------------------------------------------------------------
// make a new IP-over-ethernet packet
int netpkt_new_eth_tcp(struct netpkt *pkt, int len);

// make a UDP packet, reply to sender
int
netpkt_new_udp(struct netpkt *pkt, 
	       char *buf, int buflen,
	       char *msg, int msglen,
	       struct netpkt *replyto);

int netpkt_new_tcp_len(struct netpkt *pkt, int len);

int netpkt_new_tcp_flags(struct netpkt *pkt, int flags);

// unimplemented
int netpkt_new_ip_opt(struct netpkt *pkt, int type, int len, char *data);

// unimplemented
int netpkt_new_tcp_opt(struct netpkt *pkt, int type, int len, char *data);

#include "netpkt_inline.c"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETPKT_H_INCLUDED */

/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

