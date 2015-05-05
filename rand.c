#include <stdlib.h>
#include <limits.h>

#include "rand.h"

int
rand_init(unsigned long seed) {
    srand(seed);
    return 0;
}


unsigned long 
rand_u32(unsigned long max) {
    if( max == 0 ) max = ULONG_MAX;
    
    return (unsigned long)((double)max*rand()/RAND_MAX);
}

double
rand_d(double max) {
    return ((double)max*rand()/(RAND_MAX+1.0));
}

// stupid, but works
int
rand_bytes(char *buf, int len) {
    int i;
    for(i=0; i<len; i++) {
	*(unsigned char*)buf = (unsigned char)rand_u32(0xff);
	buf++;
    }
    return 0;
}
