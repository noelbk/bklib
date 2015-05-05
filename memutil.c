#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "bklib/config.h"
#include "bklib/memutil.h"
#include "bklib/warn.h"

// return the index of the first different byte, or -1
int
memdiff(void *va, int na, void *vb, int nb) {
    int i;
    char *pa = (char*)va;
    char *pb = (char*)vb;

    if( na < nb ) return na;
    if( nb < na ) return nb;
    if( !pa && !pb ) return -1;
    if( !pa || !pb ) return 0;
    
    for(i=0; i<na; i++) {
	if( *pa++ != *pb++ ) {
	    return i;
	}
    }
    return -1;
}

#define BUF_ADD(a)  i=a; if(i<0) break; buf += i, len -= i;

// finish a row 
int
memdump_row(char *buf, int len, char *p, int x) {
    int j, r;
    char *q;
    char *orig = buf;
    int i, err=-1;

    do {
	r = x%16;
	if( r == 0 ) r = 16;
	for(j=r; j<16; j++) {
	    if( !(j%2) ) {
		BUF_ADD(snprintf(buf, len, " "));
	    }
	    BUF_ADD(snprintf(buf, len, "  "));
	}

	BUF_ADD(snprintf(buf, len, "  "));
	for(q=p-r; q<p; q++) {
	    BUF_ADD(snprintf(buf, len, "%c", isprint(*q) && !isspace(*q) ? *q : '.'));
	}
	err = 0;
    } while(0);
    return err ? err : buf-orig;
}

char*
memdump(char *buf, int len, void *src, int n) {
    unsigned char *p = (unsigned char *)src;
    int x;
    char *orig = buf;
    int i, err=-1;

    do {
	x = 0;
	if( n == 0 ) {
	    err = 0;
	    break;
	}
	BUF_ADD(snprintf(buf, len, "%4d:", x));

	for(x=0; n>0; x++, n--, p++) {
	    if( x>0 && !(x%16) ) {
		BUF_ADD(memdump_row(buf, len, (char*)p, x));
		BUF_ADD(snprintf(buf, len, "\n%4d:", x));
	    }
	    if( !(x%2) ) {
		BUF_ADD(snprintf(buf, len, " "));
	    }
	    BUF_ADD(snprintf(buf, len, "%02x", *p));
	}
	BUF_ADD(memdump_row(buf, len, (char*)p, x));
	BUF_ADD(snprintf(buf, len, "\n"));
	err=0;
    } while(0);
    return err ? 0 : orig;
}


void*
mem_pack_4(void *src, char a1, char a2, char a3, char a4) {
    char *p = (char*)src;
    *(p++) = a1;
    *(p++) = a2;
    *(p++) = a3;
    *(p++) = a4;
    return src;
}

void *
mem_pack_6(void *buf, char a1, char a2, char a3, char a4, char a5, char a6) {
    char *p = (char*)buf;
    *(p++) = a1;
    *(p++) = a2;
    *(p++) = a3;
    *(p++) = a4;
    *(p++) = a5;
    *(p++) = a6;
    return buf;
}


void* 
memfind(void *haystack, int hay_len, void *needle_in, int needle_len) {
    char *p = haystack, *q, *needle = (char*)needle_in;
    int len = hay_len;

    while(len>=needle_len) {
	q = memchr(p, needle[0], len);
	if( !q ) return 0;
	len -= q-p;
	if( len < needle_len ) return 0;
	p = q;
	if( !memcmp(p, needle, needle_len) ) return p;
	p++;
	len--;
    }
    return 0;
}

