#include <stdio.h>
#include "bitops.h"

int
main(int argc, char **argv) {
    unsigned char c, x;
    int off, len;

    c = 0x3c; // 00111100
    for(len=1; len<=8; len++) {
	for(off=0; off<=8-len; off++) {
	    x = bit_get(c, len, off);
	    printf("len=%d off=%d c=%02x x=%02x\n", len, off, (unsigned)c, (unsigned)x);
	}
    }

    c = 0;
    for(len=1; len<=8; len++) {
	for(off=0; off<=8-len; off++) {
	    c = 0;
	    c = bit_set(c, len, off, (1<<len)-1);
	    printf("len=%d off=%d c=%02x\n", len, off, (unsigned)c);
	}
    }

    return 0;
}

