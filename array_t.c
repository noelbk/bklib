#include "array.h"
#include <stdio.h>
#include <string.h>

char *
ok(int test) {
    return test ? "ok" : "not ok";
}

void
print_ok(char *name, int test) {
    printf("%-20s %s\n", name, ok(test));
}


#define NELTS 103
#define ELTSZ 81
#define BLOCKSZ 1

int
main() {
    array_t a;
    int i, t;
    char buf[4096], *p;

    array_init(&a, ELTSZ, BLOCKSZ);
    p = (char*)array_add(&a, NELTS);
    for(i=0; i<NELTS; i++) {
	sprintf(p, "%d", i);
	p += ELTSZ;
    }

    print_ok("count", array_count(&a)==NELTS);

    t = 1;
    for(i=0; i<array_count(&a); i++) {
	p = (char*)array_get(&a, i);
	sprintf(buf, "%d", i);
	if( strcmp(p, buf) ) {
	    fprintf(stderr, "error: a(%d)=%s", i, p);
	    t = 0;
	}

    }
    print_ok("get", t && i==NELTS);
	   
    array_clear(&a);
    print_ok("clear", array_count(&a)==0);
    
    t = 1;
    for(i=0; i<NELTS; i++) {
	if( array_get(&a, i) ) t = 0;
    }
    print_ok("get_null", t);

    return 0;
}
