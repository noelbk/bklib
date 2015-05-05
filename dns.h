#ifndef DNS_H_INCLUDED
#define DNS_H_INCLUDED

#ifdef __cplusplus
extern "C" 
#endif

#include "warn.h"
#include "netpkt.h"

enum dns_class_t {
    DNS_CLASS_IN = 1, // the Internet
    DNS_CLASS_CS = 2, // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
    DNS_CLASS_CH = 3, // the CHAOS class
    DNS_CLASS_HS = 4, // Hesiod [Dyer 87]
};

enum dns_type_t {
    DNS_TYPE_A     = 1, // a host address
    DNS_TYPE_NS    = 2, // an authoritative name server
    DNS_TYPE_MD    = 3, // a mail destination (Obsolete - use MX)
    DNS_TYPE_MF    = 4, // a mail forwarder (Obsolete - use MX)
    DNS_TYPE_CNAME = 5, // the canonical name for an alias
    DNS_TYPE_SOA   = 6, // marks the start of a zone of authority
    DNS_TYPE_MB    = 7, // a mailbox domain name (EXPERIMENTAL)
    DNS_TYPE_MG    = 8, // a mail group member (EXPERIMENTAL)
    DNS_TYPE_MR    = 9, // a mail rename domain name (EXPERIMENTAL)
    DNS_TYPE_NULL  = 10, // a null RR (EXPERIMENTAL)
    DNS_TYPE_WKS   = 11, // a well known service description
    DNS_TYPE_PTR   = 12, // a domain name pointer
    DNS_TYPE_HINFO = 13, // host information
    DNS_TYPE_MINFO = 14, // mailbox or mail list information
    DNS_TYPE_MX    = 15, // mail exchange
    DNS_TYPE_TXT   = 16, // text strings
    DNS_TYPE_AAAA  = 28, // ipv6 from rfc1886, reply with 16byte ipv6 addr
};

enum dns_rcode_t {
    DNS_RCODE_OK        = 0, /* No error condition */
    DNS_RCODE_FORMAT    = 1, /* Format error - The name server was
				unable to interpret the query. */
    DNS_RCODE_SERVER    = 2, /* Server failure - The name server was
				unable to process this query due to a
				problem with the name server. */
    DNS_RCODE_NAME      = 3, /* Name Error - Meaningful only for
				responses from an authoritative name
				server, this code signifies that the
				domain name referenced in the query does
				not exist. */
    DNS_RCODE_NOTIMPL    = 4, /* Not Implemented - The name server does
				 not support the requested kind of query. */
    DNS_RCODE_REFUSED    = 5, /* Refused - The name server refuses to
				 perform the specified operation for
				 policy reasons.  For example, a name
				 server may not wish to provide the
				 information to the particular requester,
				 or a name server may not wish to perform
				 a particular operation (e.g., zone */
};

enum dns_opcode_t {
    DNS_OPCODE_QUERY  = 0, // a standard query (QUERY)
    DNS_OPCODE_IQUERY = 1, // an inverse query (IQUERY)
    DNS_OPCODE_STATUS = 2, // a server status request (STATUS)
};

enum dns_qtype_t {
    DNS_QTYPE_AXFR   = 252, // A request for a transfer of an entire zone
    DNS_QTYPE_MAILB  = 253, // A request for mailbox-related records (MB, MG or MR)
    DNS_QTYPE_MAILA  = 254, // A request for mail agent RRs (Obsolete - see MX)
    DNS_QTYPE_ALL    = 255, // A request for all records
};

#define dns_qr_mask  0x1
#define dns_qr_shift 15
#define dns_opcode_mask  0xf
#define dns_opcode_shift 11
#define dns_aa_mask  0x1
#define dns_aa_shift 10
#define dns_tc_mask  0x1
#define dns_tc_shift 9
#define dns_rd_mask  0x1
#define dns_rd_shift 8
#define dns_ra_mask  0x1
#define dns_ra_shift 7
#define dns_x3_mask  0x7
#define dns_x3_shift 4
#define dns_rcode_mask  0xf
#define dns_rcode_shift 0

#define dns_qr(h)     ((h)->flags>>15) & 0x1;
#define dns_opcode(h) ((h)->flags>>11) & 0xf;
#define dns_aa(h)     ((h)->flags>>10) & 0x1;
#define dns_tc(h)     ((h)->flags>>9)  & 0x1;
#define dns_rd(h)     ((h)->flags>>8)  & 0x1;
#define dns_ra(h)     ((h)->flags>>7)  & 0x1;
#define dns_x3(h)     ((h)->flags>>4)  & 0x7;
#define dns_rcode(h)  ((h)->flags>>0)  & 0xf;
struct dns_hdr_s {
    u16 id;
    u16 flags;
    u16 qrcount;
    u16 ancount;
    u16 nscount;
    u16 arcount;
};
typedef struct dns_hdr_s  dns_hdr_t;

#define DNS_HDR_LEN 12

struct dns_s {
    char *buf, *end, *ptr;
    u32 buflen;
    u8 rr_idx;
    void *name_hash; // an internal hash table for compressing names

    int id;
    int qr;
    int opcode;
    int aa;
    int tc;
    int rd;
    int ra;
    int x3;
    int rcode;

    int qrcount;
    int ancount;
    int nscount;
    int arcount;
};
typedef struct dns_s  dns_t;

#define DNS_NAME_PTR (0xc0)
#define DNS_NAME_MAX 256

enum dns_rr_section_t {
    DNS_RR_QUESTION=1,
    DNS_RR_ANSWER=2,
    DNS_RR_AUTHORITY=3,
    DNS_RR_ADDITIONAL=4,
};

struct dns_rr_s {
    int section; /* QUESTION, ANSWER, etc */

    char name[DNS_NAME_MAX];
    u16 type;
    u16 class;
    u32 ttl;
    u16 rdata_len;
    char *rdata_ptr;
    union {
	struct {
	    u32 addr;
	} a;
	struct  {
	    char name[DNS_NAME_MAX];
	} cname;
	struct {
	    char name[DNS_NAME_MAX];
	} ptr;
	struct {
	    char *cpu;
	    char *os;
	} hinfo;
	struct {
	    char *txt;
	} txt;
    } rdata;
};
typedef struct dns_rr_s dns_rr_t;

enum pack_t {
    PACK_NET2HOST = 1,    
    PACK_HOST2NET = 0
};

int
dns_init(dns_t *dns, char *buf, int len);

int
dns_free(dns_t *dns);

// returns length of dns->buf
int
dns_pack(dns_t *dns, int net2host);
    
// return the raw request
char *
dns_buf(dns_t *dns);

// return the raw number of bytes in dns->buf
int
dns_buflen(dns_t *dns);

// returns length of dst
int
dns_pack_name(dns_t *dns, char *src, char *dst, int dstlen);					       

		
// returns length of dst.  sets *src_end to next byte after src.
int
dns_unpack_name(dns_t *dns, char *src, int srclen, char *dst, int dstlen,
		char **src_end);
    
int
dns_rr_rewind(dns_t *dns);

// pack an rr to/from dns->ptr.
int
dns_rr_pack(dns_t *dns, int net2host, dns_rr_t *rr);

char*
dns_type_str(int x);

char*
dns_rcode_str(int x);

char*
dns_qtype_str(int x);

char*
dns_class_str(int x);

char*
dns_opcode_str(int x);


char*
dns_rr_str(int x);

char*
dns_fmt(dns_t *dns, char *buf, int len);

// returns 0 iff this is a query for an IN A record. You call
// dns_init() and dns_unpack().
int
dns_check_query_in_a(dns_t *dns, char *hostname, int len);

int
dns_pack_reply_in_a(dns_t *dns_reply, dns_t *dns_query, u32 addr, u32 ttl);

// returns 0 iff this is a query for an IN A record. You call
// dns_init() and dns_unpack().
int
dns_pack_query_in_a(dns_t *dns, char *hostname, int id);

// pack a DNS request for hostname into buf, return length, ready to
// send to your favourite name server
int
dns_resolve_query(char *buf, int buflen, char *hostname, int id);

// parse a query reply from a name server.  hosts[0] is a best guess
// "IN A" record, with addr=0 if none found.  hosts[1..N] are all
// records returned
//
int
dns_resolve_reply(char *buf, int buflen, 
		  int *id, dns_rr_t *hosts, int maxhosts);

#ifdef __cplusplus
}
#endif
#endif // DNS_H_INCLUDED
