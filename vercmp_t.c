#include <stdio.h>
#include "vercmp.h"

struct {
    char *v1, *v2;
} v_t[] = {
    { "1", "2" }
    ,{ "a", "b" }
    ,{ "a", "a-1" }
    ,{ "1.2", "1.3" }
    ,{ "1.2.3", "01.002.0003" }
    ,{ "1.alpha.3", "01.beta.0003" }
    ,{ "a.1-1", "a.1-2" }
    ,{0,0}
};


int
main(int argc, char **argv) {
    int i;
    for(i=0; v_t[i].v1; i++) {
	printf("vercmp(%s, %s)=%d\n"
	       ,v_t[i].v1, v_t[i].v2, vercmp(v_t[i].v1, v_t[i].v2));
	printf("vercmp(%s, %s)=%d\n"
	       ,v_t[i].v2, v_t[i].v1, vercmp(v_t[i].v2, v_t[i].v1));
	printf("vercmp(%s, %s)=%d\n"
	       ,v_t[i].v1, v_t[i].v1, vercmp(v_t[i].v1, v_t[i].v1));
	printf("vercmp(%s, %s)=%d\n"
	       ,v_t[i].v2, v_t[i].v2, vercmp(v_t[i].v2, v_t[i].v2));
    }
    return 0;
}
