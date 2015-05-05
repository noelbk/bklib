#include <stdio.h>
#include <limits.h>

#include "rand.h"

int main() {
    long t[] = {
	1, 1 
	,2, 2
	,2, 2
	,100, 100
	,ULONG_MAX, ULONG_MAX
    };
    int i, n = sizeof(t)/sizeof(*t);

    for(i=0; i<n; i++) {
	printf("rand(%lu)=%lu\n", t[i], rand_u32(t[i]));
    }
    return 0;
}
