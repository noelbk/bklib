#include <stdio.h>
#include <string.h>

#include "dir.h"
#include "debug.h"

int
main(int argc, char **argv) {
    dir_t *d;
    char *p = ".";
    char *files[1024];
    char buf[16384];
    int i, n;

    do {
	p = "a/b/c/d";
	strncpy(buf, p, sizeof(buf));
	printf("orig=%s dir=%s filename=%s\n", p, buf, dir_filename(buf, 1));
	     

	p = ".";
	if( argc>1 ) p = argv[1];
	printf("\ndir_read(%s)\n", p);
	d = dir_open(p); 
	assertb(d);
	while( dir_read(d, buf, sizeof(buf))>0 ) {
	    printf("%s\n", buf);
	}
	dir_close(d);
	d = 0;

	printf("\ndir_sorted(%s)\n", p);
	n = dir_sorted(p, files, sizeof(files)/sizeof(*files),
		       buf, sizeof(buf));
	assertb(n>=0);
	for(i=0; i<n; i++) {
	    printf("%s\n", files[i]);
	}


    } while(0);
    dir_close(d);
    return 0;
}
