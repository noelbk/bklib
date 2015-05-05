#ifndef NETPKT_INET_H_INCLUDED
#define NETPKT_INET_H_INCLUDED

#include "itypes.h"
#include "bitops.h"

#define ETH_ALEN 6

#define	ETHERTYPE_PUP		0x0200          /* Xerox PUP */
#define	ETHERTYPE_IP		0x0800		/* IP */
#define	ETHERTYPE_ARP		0x0806		/* Address resolution */
#define	ETHERTYPE_REVARP	0x8035		/* Reverse ARP */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */

// NBK - Nov 28 2004 - force msvc to pack these structs
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif // _MSC_VER

/* 10Mb/s ethernet header */
typedef struct netpkt_ether
{
    u8  ether_dhost[ETH_ALEN];	/* destination eth addr	*/
    u8  ether_shost[ETH_ALEN];	/* source ether addr	*/
    u16 ether_type;		        /* packet type ID field	*/
} netpkt_ether;


/* ARP protocol opcodes. */
#define	ARPOP_REQUEST	1		/* ARP request.  */
#define	ARPOP_REPLY	2		/* ARP reply.  */
#define	ARPOP_RREQUEST	3		/* RARP request.  */
#define	ARPOP_RREPLY	4		/* RARP reply.  */
#define	ARPOP_InREQUEST	8		/* InARP request.  */
#define	ARPOP_InREPLY	9		/* InARP reply.  */
#define	ARPOP_NAK	10		/* (ATM)ARP NAK.  */

typedef struct netpkt_arp {
    u16 hrd;		/* Format of hardware address.  */
    u16 pro;		/* Format of protocol address.  */
    u8  hln;		/* Length of hardware address.  */
    u8  pln;		/* Length of protocol address.  */
    u16 op;		/* ARP opcode (command).  */
    u8  sha[ETH_ALEN];	/* sender hardware address */
    u8  spa[4];		/* sender protocol address */
    u8  tha[ETH_ALEN];	/* target hardware address */
    u8  tpa[4];		/* target protocol address */
} netpkt_arp;

// rfc760
#define ip_v(ip)         bit_get(ip->vh, 4, 4)
#define ip_v_set(ip, v)  ip->vh = (u8)bit_set(ip->vh, 4, 4, v)
#define ip_hl(ip)        bit_get(ip->vh, 4, 0)
#define ip_hl_set(ip, v) ip->vh = (u8)bit_set(ip->vh, 4, 0, v)

typedef struct netpkt_ip {
    /* use ip_(v|hl) */
    u8 vh;              /* version(4), header length(4) */
    u8  tos;		/* type of service */
    u16 len;		/* total length */
    u16 id;		/* identification */
    u16 off;		/* fragment offset field */
    u8  ttl;		/* time to live */
    u8  p;		/* protocol */
    u16 sum;		/* checksum */
    u32 src, dst;	/* source and dest address */
} netpkt_ip;

/* buf must be at least 13 chars, or NULL for static buf */
char *
netpkt_ntoa(u32 addr, char *buf);

// rfc 761
#define tcp_doff(tcp)          bit_get(tcp->doffres, 4, 4)
#define tcp_doff_set(tcp, v)   tcp->doffres = (u8)bit_set(tcp->doffres, 4, 4, v)
#define tcp_res(tcp)           bit_get(tcp->doffres, 4, 0) << 2 \
                               | bit_get(tcp->resflags, 2, 6)
#define tcp_res_set(tcp, v)    tcp->doffres = (u8)bit_set(tcp->doffres, 4, 0, bit_get(v, 4, 2)) \
	; tcp->resflags = (u8)bit_set(tcp->resflags, 2, bit_get(v, 2, 6))
#define tcp_urg(tcp)           bit_get(tcp->resflags, 1, 5)
#define tcp_urg_set(tcp, v)    tcp->resflags = bit_set(tcp->resflags, 1, 5, v)
#define tcp_ack(tcp)           bit_get(tcp->resflags, 1, 4)
#define tcp_ack_set(tcp, v)    tcp->resflags = bit_set(tcp->resflags, 1, 4, v)
#define tcp_psh(tcp)           bit_get(tcp->resflags, 1, 3)
#define tcp_psh_set(tcp, v)    tcp->resflags = bit_set(tcp->resflags, 1, 3, v)
#define tcp_rst(tcp)           bit_get(tcp->resflags, 1, 2)
#define tcp_rst_set(tcp, v)    tcp->resflags = bit_set(tcp->resflags, 1, 2, v)
#define tcp_syn(tcp)           bit_get(tcp->resflags, 1, 1)
#define tcp_syn_set(tcp, v)    tcp->resflags = bit_set(tcp->resflags, 1, 1, v)
#define tcp_fin(tcp)           bit_get(tcp->resflags, 1, 0)
#define tcp_fin_set(tcp, v)    tcp->resflags = bit_set(tcp->resflags, 1, 0, v)

typedef struct netpkt_tcp {
    u16 source;
    u16 dest;
    u32 seq;
    u32 ack_seq;

    /* use tcp_(doff|urg|psh|etc...) */
    u8 doffres;  /* doff(4), reserved(4) */
    u8 resflags; /* reeserved(2), flags(6) */

    u16 window;
    u16 check;
    u16 urg_ptr;
} netpkt_tcp;

typedef struct netpkt_udp {
    u16	source;
    u16	dest;
    u16	len;
    u16	check;
} netpkt_udp;

typedef struct netpkt_icmp {
    u8 type;		/* message type */
    u8 code;		/* type sub-code */
    u16 checksum;
    union
    {
	struct
	{
	    u16	id;
	    u16	sequence;
	} echo;			/* echo datagram */
	u32	gateway;	/* gateway address */
	struct
	{
	    u16	__unused;
	    u16	mtu;
	} frag;			/* path mtu discovery */
    } un;
} netpkt_icmp;

#define ICMP_ECHOREPLY		0	/* Echo Reply			*/
#define ICMP_DEST_UNREACH	3	/* Destination Unreachable	*/
#define ICMP_SOURCE_QUENCH	4	/* Source Quench		*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO		8	/* Echo Request			*/
#define ICMP_TIME_EXCEEDED	11	/* Time Exceeded		*/
#define ICMP_PARAMETERPROB	12	/* Parameter Problem		*/
#define ICMP_TIMESTAMP		13	/* Timestamp Request		*/
#define ICMP_TIMESTAMPREPLY	14	/* Timestamp Reply		*/
#define ICMP_INFO_REQUEST	15	/* Information Request		*/
#define ICMP_INFO_REPLY		16	/* Information Reply		*/
#define ICMP_ADDRESS		17	/* Address Mask Request		*/
#define ICMP_ADDRESSREPLY	18	/* Address Mask Reply		*/
#define NR_ICMP_TYPES		18


/* Codes for UNREACH. */
#define ICMP_NET_UNREACH	0	/* Network Unreachable		*/
#define ICMP_HOST_UNREACH	1	/* Host Unreachable		*/
#define ICMP_PROT_UNREACH	2	/* Protocol Unreachable		*/
#define ICMP_PORT_UNREACH	3	/* Port Unreachable		*/
#define ICMP_FRAG_NEEDED	4	/* Fragmentation Needed/DF set	*/
#define ICMP_SR_FAILED		5	/* Source Route failed		*/
#define ICMP_NET_UNKNOWN	6
#define ICMP_HOST_UNKNOWN	7
#define ICMP_HOST_ISOLATED	8
#define ICMP_NET_ANO		9
#define ICMP_HOST_ANO		10
#define ICMP_NET_UNR_TOS	11
#define ICMP_HOST_UNR_TOS	12
#define ICMP_PKT_FILTERED	13	/* Packet filtered */
#define ICMP_PREC_VIOLATION	14	/* Precedence violation */
#define ICMP_PREC_CUTOFF	15	/* Precedence cut off */
#define NR_ICMP_UNREACH		15	/* instead of hardcoding immediate value */

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET		0	/* Redirect Net			*/
#define ICMP_REDIR_HOST		1	/* Redirect Host		*/
#define ICMP_REDIR_NETTOS	2	/* Redirect Net for TOS		*/
#define ICMP_REDIR_HOSTTOS	3	/* Redirect Host for TOS	*/

/* Codes for TIME_EXCEEDED. */
#define ICMP_EXC_TTL		0	/* TTL count exceeded		*/
#define ICMP_EXC_FRAGTIME	1	/* Fragment Reass time exceeded	*/

#ifdef _MSC_VER
#pragma pack(pop)
#endif // _MSC_VER

#endif // NETPKT_INET_H_INCLUDED
