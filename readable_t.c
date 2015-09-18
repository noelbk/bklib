#include <stdio.h>
#include <stdlib.h>

#include "readable.h"
#include "debug.h"
#include "defutil.h"

int
main(int argc, char **argv) {
    double d;
    int i;
    char buf[4096], buf1[4096], buf2[4096], *p;

    mstime_t durations[] = {
	1
	,61
	,3601
	,24*3600+1
	,3661
	,24*3600+61
	,122
	,3601*2
	,(24*3600+1)*2
	,3661*2
	,(24*3600+61)*2
	,0
    };
      
    char *argv_default[] = {
	"",
	"3.01234e-12",
	"3.01234e-9",
	"3.01234e-6",
	"3.01234e-2",
	"3.01234e0",
	"3.01234e3",
	"3.01234e6",
	"3.001234e9",
	"30.001234e9",
	"300.01234e9",
	"3.01234e12",
    };

    printf("readable_duration:\n");
    for(i=0; durations[i]; i++) {
	printf("readable_duration(%g) short=%s long=%s\n"
	       ,durations[i]
	       ,readable_duration(durations[i], buf1, sizeof(buf1), 0)
	       ,readable_duration(durations[i], buf2, sizeof(buf2), 1)
	       );
    }
    printf("\n");

    if( argc <=1 ) {
	argv = argv_default;
	argc = NELTS(argv_default);
    }



    for(i=1; i<argc; i++) {
	p = 0;
	d = strtod(argv[i], &p);
	assertb(p>argv[i]);
	
	readable_metric(d, "", buf, sizeof(buf));
	printf("readable=%s d=%g\n", buf, d);
    }

    return 0;
}
