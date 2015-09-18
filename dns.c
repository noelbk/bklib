// http://www.ietf.org/rfc/rfc1035.txt
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "warn.h"
#include "netpkt.h"
#include "sock.h"
#include "dns.h"
#include "hash.h"
#include "rand.h"
#include "memutil.h"

#define bits2mask(bits) ((1<<(bits))-1)

#define pack_bits(net2host, net, host, bits, shift) \
if( net2host ) { \
    (host) = (((net) >> (shift)) & bits2mask(bits));	\
} \
else { \
    (net) = ((net) & ~(bits2mask(bits)<<(shift))) | (((host) & bits2mask(bits)) << (shift)); \
}

#define pack_u16(net2host, net, host) \
    if( net2host ) { \
	(host) = htons(net); \
    } \
    else { \
	(net) = ntohs(host); \
    }

#define pack_u32(net2host, net, host) \
    if( net2host ) { \
	(host) = htonl(net); \
    } \
    else { \
	(net) = ntohl(host);			\
    }

int
dns_init(dns_t *dns, char *buf, int len) {
    int err=-1;
    do {
	memset(dns, 0, sizeof(*dns));
	dns->buf = buf;
	dns->buflen = len;
	dns->end = buf + len;
	dns->ptr = buf + DNS_HDR_LEN;
	dns->rr_idx = 0;
	assertb(dns->ptr <= dns->end);
	
	err = 0;
    } while(0);
    return err;
}

int
dns_free(dns_t *dns) {
    if( dns->name_hash ) {
	hash_clear((hash_t*)dns->name_hash);
	free(dns->name_hash);
	dns->name_hash = 0;
    }
    memset(dns, 0, sizeof(*dns));
    return 0;
}

// returns length of dns->buf
int
dns_pack(dns_t *dns, int net2host) {
    dns_hdr_t *h = (dns_hdr_t *)dns->buf;
    int err=-1;
    u16 u;

    do {
	pack_u16(net2host, h->id, dns->id);
	
	/* ignore warning C4244: '=' : conversion from 'unsigned long ' to 'unsigned short ', possible loss of data */
	u = 0;
	if( net2host ) {
	    pack_u16(net2host, h->flags, u);
	}
	pack_bits(net2host, u, dns->qr,     1, 15);
	pack_bits(net2host, u, dns->opcode, 4, 11);
	pack_bits(net2host, u, dns->aa,     1, 10);
	pack_bits(net2host, u, dns->tc,     1, 9);
	pack_bits(net2host, u, dns->rd,     1, 8);
	pack_bits(net2host, u, dns->ra,     1, 7);
	pack_bits(net2host, u, dns->x3,     3, 4);
	pack_bits(net2host, u, dns->rcode,  4, 0);
	if( !net2host ) {
	    pack_u16(net2host, h->flags, u);
	}
	
	pack_u16(net2host, h->qrcount, dns->qrcount);
	pack_u16(net2host, h->ancount, dns->ancount);
	pack_u16(net2host, h->nscount, dns->nscount);
	pack_u16(net2host, h->arcount, dns->arcount);

	err = 0;
    } while(0);

    debug_if(DEBUG_INFO+2) {
	char buf[16384];
	debugf("dns_pack net2host=%d err=%d buf=\n%s\n"
	       , net2host, err
	       , memdump(buf, sizeof(buf), dns->buf, dns->buflen)
	       );
    }

    return err ? err : dns->ptr - dns->buf;
}
    
// returns length of dst
int
dns_pack_name(dns_t *dns, char *src, char *dst, int dstlen) {
    int err=-1;
    char *p, *dst_orig=dst;
    int i, n;
    int srclen = strlen(src);
    hash_node_t *node;

    while(1) {
	if( srclen <= 0 ) {
	    *dst++ = 0;
	    dstlen--;
	    err = 0;
	    break;
	}
	assertb(srclen < DNS_NAME_MAX);

	err = -1;

	// look for strings to compress
	if( !dns->name_hash ) {
	    dns->name_hash = malloc(sizeof(hash_t));
	    assertb(dns->name_hash);
	    i = hash_init((hash_t*)dns->name_hash, 
			  hash_hash_str, hash_cmp_str,
			  DNS_NAME_MAX, 0, 0);
	    assertb(!i);
	}
	node = hash_get((hash_t*)dns->name_hash, src);
	if( node ) {
	    // put a pointer to the previous string and quit
	    intptr_t i = (intptr_t)node->node_val;
	    assertb( i < (DNS_NAME_PTR<<8) );
	    assertb(dstlen>=2);
	    *dst++ = (u8)(((i>>8) & 0xff) | (DNS_NAME_PTR));
	    *dst++ = (u8)(i & 0xff);
	    dstlen -= 2;
	    err = 0;
	    break;
	}
	else {
	    // save a pointer to the current position
	    assertb(dst>=dns->buf && dst<dns->end);
	    i = (dst - dns->buf);
	    node = hash_put((hash_t*)dns->name_hash, src, (void*)(intptr_t)i);
	    assertb(node);
	    strncpy(node->node_key, src, DNS_NAME_MAX);
	}

	// split at "."
	p = strchr(src, '.');
	n = p ? (u8)(p-src) : srclen;
	
	// copy len, src to dst
	assertb(dstlen > n+1);
	*dst = n;
	dst++;
	dstlen--;
	strncpy(dst, src, n);
	src += n;
	srclen -= n;
	if( p ) {
	    // skip over trailing "."
	    src++;
	    srclen--;
	}
	dst += n;
	dstlen -= n;
    }
    return err ? err : dst-dst_orig;
}					       

		
// returns length of dst.  sets *src_end to next byte after src.
int
dns_unpack_name(dns_t *dns, char *src_in, int srclen, char *dst_in, int dstlen,
		char **src_end) {
    char *src = src_in;
    char *dst = dst_in;
    char *dst_orig = dst;
    int n, err=-1;
    int ptr_max = 100;
    
    *src_end = 0;
    while(srclen>0) {
	n = *(unsigned char*)src;
	src++;
	srclen--;
	if( !n ) {
	    err = 0;
	    break;
	}

	if( n & DNS_NAME_PTR ) {
	    // jump to another name in the orig buffer
	    
	    // don't loop forever
	    assertb(--ptr_max > 0);

	    // two-byte offset
	    assertb(srclen>0);
	    n &= ~DNS_NAME_PTR;
	    n = (n<<8) | *src++; 
	    assertb(n>0);
	    
	    // remember where the name ended
	    if( !*src_end ) {
		*src_end = src;
	    }

	    // jump into orig
	    src = dns->buf + n;
	    srclen = (dns->end - dns->buf) - n;
	    continue;
	}
	
	if(dst > dst_orig) {
	    assertb(dstlen>1);
	    *dst++ = '.';
	    dstlen--;
	}
	
	// just a plain copy
	assertb(dstlen > n);
	strncpy(dst, src, n);
	src += n;
	srclen -= n;
	dst += n;
	dstlen -= n;
    }
    if( !err ) {
	*dst++ = 0;
	if( !*src_end ) {
	    *src_end = src;
	}
    }
    return err ? err : dst-dst_orig;
}
    
int
dns_rr_rewind(dns_t *dns) {
    dns->ptr = dns->buf + DNS_HDR_LEN;
    dns->rr_idx = 0;
    return 0;
}

// pack an rr to/from dns->ptr.
int
dns_rr_pack(dns_t *dns, int net2host, dns_rr_t *rr) {
    int i, err=-1, ok=1;
    char *p;

    do {
	if( net2host ) {
	    memset(rr, 0, sizeof(*rr));
	    if( dns->rr_idx < dns->qrcount ) {
		rr->section = DNS_RR_QUESTION;
	    }
	    else if( dns->rr_idx < dns->qrcount + dns->ancount ) {
		rr->section = DNS_RR_ANSWER;
	    }
	    else if( dns->rr_idx < dns->qrcount + dns->ancount + dns->nscount) {
		rr->section = DNS_RR_AUTHORITY;
	    }
	    else if( dns->rr_idx < dns->qrcount + dns->ancount + dns->nscount + dns->arcount) {
		rr->section = DNS_RR_ADDITIONAL;
	    }
	    else {
		/* end of rr's */
		err = 0;
		ok = 0;
		break;
	    }
	    i = dns_unpack_name(dns, dns->ptr, dns->end-dns->ptr, 
				rr->name, DNS_NAME_MAX, 
				&p);
	}
	else {
	    switch(rr->section) {
	    case DNS_RR_QUESTION:   dns->qrcount++; break;
	    case DNS_RR_ANSWER:     dns->ancount++; break;
	    case DNS_RR_AUTHORITY:  dns->nscount++; break;
	    case DNS_RR_ADDITIONAL: dns->arcount++; break;
	    default: assertb(0);
	    }
	    i = dns_pack_name(dns, rr->name, dns->ptr, dns->end - dns->ptr);
	    p = dns->ptr + i;
	}
	if( i <= 0 ) {
	    char buf[16384];
	    debug(DEBUG_WARN, 
		  ("dns_rr_pack i=%d net2host=%d dns->ptr=%ld buf=\n%s\n"
		   , i, net2host, dns->ptr-dns->buf
		   , memdump(buf, sizeof(buf), dns->buf, dns->end - dns->buf)
		   ));
	}
	assertb(i>0);
	dns->rr_idx++;

	pack_u16(net2host, *(u16*)p, rr->type);
	p += 2;
	pack_u16(net2host, *(u16*)p, rr->class);
	p += 2;
	
	if( rr->section == DNS_RR_QUESTION ) {
	    err = 0;
	    dns->ptr = p;
	    break;
	}

	pack_u32(net2host, *(u32*)p, rr->ttl);
	p += 4;
	pack_u16(net2host, *(u16*)p, rr->rdata_len);
	p += 2;

	if( net2host ) {
	    rr->rdata_ptr = p;
	}
	
	switch(rr->type) {
	case DNS_TYPE_A:
	    if( net2host ) {
		assertb(rr->rdata_len == 4);
		rr->rdata.a.addr = *(u32*)p;
	    }
	    else {
		*(u32*)p = rr->rdata.a.addr;
		rr->rdata_len = 4;
		pack_u16(net2host, *(u16*)(p-2), rr->rdata_len);
	    }
	    break;

	case DNS_TYPE_NS:
	case DNS_TYPE_CNAME:
	case DNS_TYPE_PTR:
	    if( net2host ) {
		i = dns_unpack_name(dns, 
				    p, rr->rdata_len, 
				    rr->rdata.ptr.name, DNS_NAME_MAX, &p);
	    }
	    else {
		i = dns_pack_name(dns, 
				  rr->rdata.ptr.name, 
				  p, DNS_NAME_MAX);
		rr->rdata_len = i;
		pack_u16(net2host, *(u16*)(p-2), rr->rdata_len);
	    }
	    assertb(i>=0);
	    break;
	default:
	    if( net2host ) {
	    }
	    else {
		assertb(p-dns->end < rr->rdata_len);
		if( rr->rdata_ptr ) {
		    memcpy(p, rr->rdata_ptr, rr->rdata_len);
		}
	    }
	    break;
	}

	rr->rdata_ptr = p;
	p += rr->rdata_len;

	dns->ptr = p;

	err = 0;
    } while(0);
    return err ? err : ok;
}

char*
default_int_str(int x) {
    static char buf[100];
    snprintf(buf, sizeof(buf), "%d", x);
    return buf;
}

char*
dns_type_str(int x) {
    char *p;
    switch (x) {
    case DNS_TYPE_A: p="A"; break;
    case DNS_TYPE_NS: p="NS"; break;
    case DNS_TYPE_MD: p="MD"; break;
    case DNS_TYPE_MF: p="MF"; break;
    case DNS_TYPE_CNAME: p="CNAME"; break;
    case DNS_TYPE_SOA: p="SOA"; break;
    case DNS_TYPE_MB: p="MB"; break;
    case DNS_TYPE_MG: p="MG"; break;
    case DNS_TYPE_MR: p="MR"; break;
    case DNS_TYPE_NULL: p="NULL"; break;
    case DNS_TYPE_WKS: p="WKS"; break;
    case DNS_TYPE_PTR: p="PTR"; break;
    case DNS_TYPE_HINFO: p="HINFO"; break;
    case DNS_TYPE_MINFO: p="MINFO"; break;
    case DNS_TYPE_MX: p="MX"; break;
    case DNS_TYPE_TXT: p="TXT"; break;
    default: p = default_int_str(x); break;
    }
    return p;
}

char*
dns_rcode_str(int x) {
    char *p;
    switch(x) {
    case DNS_RCODE_OK: p="OK"; break;
    case DNS_RCODE_FORMAT: p="FORMAT"; break;
    case DNS_RCODE_SERVER: p="SERVER"; break;
    case DNS_RCODE_NAME: p="NAME"; break;
    case DNS_RCODE_NOTIMPL: p="NOTIMPL"; break;
    case DNS_RCODE_REFUSED: p="REFUSED"; break;
    default: p = default_int_str(x); break;
    }
    return p;
};

char*
dns_qtype_str(int x) {
    char *p;
    switch(x) {
    case DNS_QTYPE_AXFR: p="AXFR"; break;
    case DNS_QTYPE_MAILB: p="MAILB"; break;
    case DNS_QTYPE_MAILA: p="MAILA"; break;
    case DNS_QTYPE_ALL: p="ALL"; break;
    default: p = default_int_str(x); break;
    }
    return p;
};

char*
dns_class_str(int x) {
    char *p;
    switch(x) {
    case DNS_CLASS_IN: p="IN"; break;
    case DNS_CLASS_CS: p="CS"; break;
    case DNS_CLASS_CH: p="CH"; break;
    case DNS_CLASS_HS: p="HS"; break;
    default: p = default_int_str(x); break;
    }
    return p;
};

char*
dns_opcode_str(int x) {
    char *p;
    switch(x) {
    case DNS_OPCODE_QUERY: p="QUERY"; break;
    case DNS_OPCODE_IQUERY: p="IQUERY"; break;
    case DNS_OPCODE_STATUS: p="STATUS"; break;
    default: p = default_int_str(x); break;
    }
    return p;
};


char*
dns_rr_str(int x) {
    char *p;
    switch(x) {
    case DNS_RR_QUESTION:   p="QR"; break;
    case DNS_RR_ANSWER:     p="AN"; break;
    case DNS_RR_AUTHORITY:  p="NS"; break;
    case DNS_RR_ADDITIONAL: p="AR"; break;
    default: p = default_int_str(x); break;
    }
    return p;
};

#define BUF_ADD(a)  i=a; if(i<0) break; buf += i, len -= i;

char*
dns_fmt(dns_t *dns, char *buf, int len) {
    char *ptr;
    int i, rr_idx;
    dns_rr_t rr;
    char *orig = buf;
    int err=-1;
    

    do {
	BUF_ADD(snprintf(buf, len, "dns:"));
	BUF_ADD(snprintf(buf, len, " id=%d", dns->id));
	BUF_ADD(snprintf(buf, len, " qr=%d", dns->qr));
	BUF_ADD(snprintf(buf, len, " aa=%d", dns->aa));
	BUF_ADD(snprintf(buf, len, " tc=%d", dns->tc));
	BUF_ADD(snprintf(buf, len, " rd=%d", dns->rd));
	BUF_ADD(snprintf(buf, len, " ra=%d", dns->ra));
	BUF_ADD(snprintf(buf, len, " x3=%d", dns->x3));
	BUF_ADD(snprintf(buf, len, " opcode=%s", dns_opcode_str(dns->opcode)));
	BUF_ADD(snprintf(buf, len, " rcode=%s", dns_rcode_str(dns->rcode)));
	
	ptr = dns->ptr;
	dns->ptr = dns->buf + DNS_HDR_LEN;
	rr_idx = dns->rr_idx;
	dns->rr_idx = 0;

	while(1) {
	    i = dns_rr_pack(dns, PACK_NET2HOST, &rr);
	    assertb(i>=0);
	    if( i == 0 ) break;

	    BUF_ADD(snprintf(buf, len, " %s:{name=%s", dns_rr_str(rr.section), rr.name));
	    BUF_ADD(snprintf(buf, len, " class=%s", dns_class_str(rr.class)));
	    BUF_ADD(snprintf(buf, len, " type=%s", dns_type_str(rr.type)));
	    BUF_ADD(snprintf(buf, len, " ttl=%d", rr.ttl));
	    if( rr.section != DNS_RR_QUESTION ) {
		switch(rr.type) {
		case DNS_TYPE_A:
		    BUF_ADD(snprintf(buf, len, " addr=%s", netpkt_ntoa(rr.rdata.a.addr, 0)));
		    break;

		case DNS_TYPE_CNAME:
		case DNS_TYPE_PTR:
		case DNS_TYPE_NS:
		    BUF_ADD(snprintf(buf, len, " name=%s", rr.rdata.ptr.name));
		    break;
		}
	    }
	    BUF_ADD(snprintf(buf, len, "}"));

	}

	dns->buflen = dns->ptr - dns->buf;
	dns->ptr = ptr;
	dns->rr_idx = rr_idx;

	err =0;
    } while(0);

    return err ? 0 : orig;
}

char *
dns_buf(dns_t *dns) {
    return dns->buf;
}


int
dns_buflen(dns_t *dns) {
    char *ptr;
    int i, rr_idx;
    dns_rr_t rr;

    ptr = dns->ptr;
    rr_idx = dns->rr_idx;
    dns_rr_rewind(dns);

    while(1) {
	i = dns_rr_pack(dns, PACK_NET2HOST, &rr);
	assertb(i>=0);
	if( i == 0 ) break;
    }
    dns->buflen = dns->ptr - dns->buf;

    dns->ptr = ptr;
    dns->rr_idx = rr_idx;
    return dns->buflen;
}

int
dns_check_query_in_a(dns_t *dns, char *hostname, int len) {
    int i, err=-1;
    dns_rr_t rr;

    do {
	*hostname = 0;

	i = dns_pack(dns, PACK_NET2HOST);
	assertb(i>0);

	if( dns->opcode != DNS_OPCODE_QUERY ) break;
	assertb(!dns->qr);
	
	dns_rr_rewind(dns);
	while(1) {
	    // read each RR
	    i = dns_rr_pack(dns, PACK_NET2HOST, &rr);
	    assertb(i>=0);
	    if( i==0 ) break;
	
	    if( rr.section != DNS_RR_QUESTION ) break;
	    if( rr.class != DNS_CLASS_IN ) continue;
	    if( rr.type != DNS_TYPE_A 
		&& rr.type != DNS_TYPE_PTR 
		&& rr.type != DNS_TYPE_AAAA
		) continue;
	    strncpy(hostname, rr.name, len);
	    err = 0;
	    break;
	}
    } while(0);
    return err;
}

// pack a reply to an IN A query.  You must have already called
// dns_init(dns_reply)
int
dns_pack_reply_in_a(dns_t *dns_reply, dns_t *dns_query, u32 addr, u32 ttl) {
    char hostname[DNS_NAME_MAX+1];
    dns_rr_t rr;
    int i, err=-1;

    do {
	i = dns_check_query_in_a(dns_query, hostname, sizeof(hostname));
	assertb(i>=0);

	dns_reply->opcode = dns_query->opcode;
	dns_reply->id = dns_query->id; /* copy query id */
	dns_reply->qr = 1;      /* query response */
	dns_reply->rcode = dns_query->rcode;   /* no error */
	dns_reply->aa = 0;      /* authoritative */

	dns_rr_rewind(dns_reply);

	/* repeat the question */
	memset(&rr, 0, sizeof(rr));
	rr.section = DNS_RR_QUESTION;
	strncpy(rr.name, hostname, DNS_NAME_MAX);
	rr.class = DNS_CLASS_IN;
	rr.type  = DNS_TYPE_A;
	i = dns_rr_pack(dns_reply, PACK_HOST2NET, &rr);

	/* the answer */
	memset(&rr, 0, sizeof(rr));
	rr.section = DNS_RR_ANSWER;
	strncpy(rr.name, hostname, DNS_NAME_MAX);
	rr.class = DNS_CLASS_IN;
	rr.type  = DNS_TYPE_A;
	rr.ttl   = ttl;
	rr.rdata.a.addr = addr;
	i = dns_rr_pack(dns_reply, PACK_HOST2NET, &rr);

	i = dns_pack(dns_reply, PACK_HOST2NET);
	
	err = 0;
    } while(0);
    return err ? err : i;
}

// pack a reply to an IN A query.  You must have already called
// dns_init(dns_reply)
int
dns_pack_query_in_a(dns_t *dns_query, char *hostname, int id) {
    dns_rr_t rr;
    int i, err=-1;

    do {
	dns_query->opcode = DNS_OPCODE_QUERY;
	dns_query->id = id;     /* copy query id */
	dns_query->qr = 0;      /* query */
	dns_query->rcode = 0;   /* no error */
	dns_query->aa = 0;      /* authoritative */
	dns_query->rd = 1;      /* recursion desired */

	dns_rr_rewind(dns_query);

	/* the question */
	memset(&rr, 0, sizeof(rr));
	rr.section = DNS_RR_QUESTION;
	strncpy(rr.name, hostname, DNS_NAME_MAX);
	rr.class = DNS_CLASS_IN;
	rr.type  = DNS_TYPE_A;
	i = dns_rr_pack(dns_query, PACK_HOST2NET, &rr);
	assertb(!i);
	err = dns_pack(dns_query, PACK_HOST2NET);
    } while(0);
    return err;
}

int
dns_resolve_query(char *buf, int buflen, char *hostname, int id) {
    dns_t dns;
    int i, err=-1;
    do {
	dns_init(&dns, buf, buflen);
	i = dns_pack_query_in_a(&dns, hostname, id);
	err = i;
    } while(0);
    dns_free(&dns);
    return err;
}


// hosts[0] is a best guess "IN A" record, with addr=0 if none found.
// hosts[1..N] are all records returned
//
int
dns_resolve_reply(char *buf, int buflen, 
		  int *id, dns_rr_t *hosts, int maxhosts) {
    dns_t dns;
    int i, j, n, err=-1;
    int nhosts=0, qidx;
    dns_rr_t rr;

    do {
	memset(hosts, 0, maxhosts*sizeof(*hosts));
	
	i = dns_init(&dns, buf, buflen);
	i = dns_pack(&dns, PACK_NET2HOST);
	assertb(i>=0);
	if( !dns.qr ) {
	    /* not a reply, so ignore it */
	    break;
	}
	*id = dns.id;

	/* copy all query and answer rrs into hosts */
	nhosts = 2;
	assertb(nhosts<maxhosts);
	while(1) {
	    i = dns_rr_pack(&dns, PACK_NET2HOST, &rr);
	    assertb(i>=0);
	    if( i == 0 ) break;
	    
	    if( !(rr.section == DNS_RR_ANSWER
		  || rr.section == DNS_RR_QUESTION) ) {
		continue;
	    }


	    hosts[nhosts-1] = rr;
	    nhosts++;
	    assertb(nhosts<maxhosts);
	}
	assertb(nhosts<maxhosts);

	/* Try to convert all CNAME and PTR names to "IN A" names.
	   Also set qidx, the index of the queried host */
	qidx = -1;
	for(i=1; i<nhosts; i++) {
	    if( qidx < 0 && hosts[i].section == DNS_RR_QUESTION ) {
		qidx = i;
	    }
	    if( !(hosts[i].section == DNS_RR_ANSWER
		  && hosts[i].type == DNS_TYPE_CNAME) ) {
		continue;
	    }
	    for(j=1; j<nhosts; j++) {
		if( !(hosts[j].section == DNS_RR_ANSWER
		      && hosts[j].type == DNS_TYPE_A
		      && strcasecmp(hosts[j].name,
				    hosts[i].rdata.cname.name)==0) ) {
		    continue;
		}
		/* found an "IN A" record for host[i]'s CNAME */
		assertb(nhosts < maxhosts);
		n = nhosts++;
		hosts[n] = hosts[i];
		hosts[n].type = DNS_TYPE_A;
		hosts[n].rdata.a = hosts[j].rdata.a;
		break;
	    }
	}
	assertb(qidx>0);

	/* choose a random "IN A" record that answers hosts[qidx] and
           put it in hosts[0] */
	n = 0;
	for(i=1; i<nhosts; i++) {
	    if( hosts[i].section == DNS_RR_ANSWER
		&& hosts[i].type == DNS_TYPE_A
		&& strcasecmp(hosts[i].name, hosts[qidx].name) == 0 ) {
		n++;
	    }
	}
	n = rand_u32(n+1);
	for(i=1; i<nhosts; i++) {
	    if( !(hosts[i].section == DNS_RR_ANSWER
		  && hosts[i].type == DNS_TYPE_A) ) {
		continue;
	    }
	    if( --n > 0 ) continue;
	    
	    /* reached the randomly selected host record.  Copy it to
               hosts[0] */
	    hosts[0] = hosts[i];

	    break;
	    
	}
	
	err = 0;
    } while(0);
    dns_free(&dns);
    return err ? err : nhosts;
}

