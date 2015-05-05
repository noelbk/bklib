#include <math.h>
#include "itypes.h"
#include "sock.h"

typedef enum {
    PACK_READ=0
    ,PACK_WRITE=1
} pack_dir_t;

static inline
int
pack_i16(pack_dir_t d, char **buf, int *len, i16 *v) {
    char *orig = *buf;
    do {
	if( *len >= 2 ) {
	    if( d == PACK_READ ) {
		*v = ntohs(*(i16*)*buf);
	    }
	    else {
		*(i16*)*buf = htons(*v);
	    }
	}
	*buf += 2;
	*len -= 2;
    } while(0);
    return *buf-orig;
}

static inline
int
pack_i32(pack_dir_t d, char **buf, int *len, i32 *v) {
    char *orig = *buf;
    do {
	if( *len >= 4 ) {
	    if( d == PACK_READ ) {
		*v = ntohl(*(i32*)*buf);
	    }
	    else {
		*(i32*)*buf = htonl(*v);
	    }
	}
	*buf += 4;
	*len -= 4;
    } while(0);
    return *buf-orig;
}

static inline
int
pack_double(pack_dir_t d, char **buf, int *len, double *v) {
    char *orig = *buf;
    double m;
    i32 mi;
    i32 e;
    
    do {
	if( d == PACK_WRITE ) {
	    m = frexp(*v, &e);
	    mi = (i32)((m - .5) * (double)0xffffffff);
	}
	pack_i32(d, buf, len, &e);
	pack_i32(d, buf, len, &mi);
	if( d == PACK_READ ) {
	    m = (double)mi /(double)0xffffffff + .5;
	    *v = ldexp(m, e);
	}
    } while(0);
    return *buf-orig;
}

static inline
int
pack_buf16(pack_dir_t d, char **buf, int *len, char *v, i16 *vlen, i32 max) {
    char *orig = *buf;
    if( max == 0 ) max = *vlen;

    do {
	pack_i16(d, buf, len, vlen);
	if( *len >= *vlen && *vlen <= max ) {
	    if( d == PACK_READ ) {
		memcpy(v, *buf, *vlen);
		// null-terminate the string if there's room
		if( *vlen < max ) {
		    v[*vlen] = 0;
		}
	    }
	    else {
		memcpy(*buf, v, *vlen);
	    }
	}
	*buf += *vlen;
	*len -= *vlen;
    } while(0);
    return *buf-orig;
}

static inline
int
pack_buf32(pack_dir_t d, char **buf, int *len, char *v, i32 *vlen, i32 max) {
    char *orig = *buf;
    if( max == 0 ) max = *vlen;

    do {
	pack_i32(d, buf, len, vlen);
	if( *len >= (int)*vlen ) {
	    if( d == PACK_READ ) {
		memcpy(v, *buf, *vlen);
		// null-terminate the string if there's room
		if( *vlen < max ) {
		    v[*vlen] = 0;
		}
	    }
	    else {
		memcpy(*buf, v, *vlen);
	    }
	}
	*buf += *vlen;
	*len -= *vlen;
    } while(0);
    return *buf-orig;
}

/*
   gets or sets bits in "buf" from "from" to "to" inclusive, with bits
   numbered right to left like so:

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    0                   1                   2                   3   
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   pack_bits32(PACK_READ, buf, 0, 3,  &version);
   pack_bits32(PACK_READ, buf, 4, 7,  &ihl);
   pack_bits32(PACK_READ, buf, 7, 15, &tos);
*/

static inline
int
pack_bits32(pack_dir_t d, i32 *p, int from, int to, i32 *bits) {
    int len   = from-to+1;
    int shift = 32 - len;
    int mask  = ~(-(1<<len)); // "1" x len

    if( shift < 0 || len < 0 || len > 32 ) {
	return -1;
    }

    if( d == PACK_WRITE ) {
	*p &= ~(mask << shift);
	*p |= (*bits & mask) << shift;
    }
    else {
	*bits = (*p & (mask << shift)) >> shift;
    }

    return len;
}

