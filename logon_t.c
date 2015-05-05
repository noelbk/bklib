#include <stdio.h>
#include "logon.h"
#include "debug.h"

int
main(int argc, char **argv) {
    char buf[4096];
    int i;
    char *user, *pass, *domain=0;

    if( argc < 3 ) {
	fprintf(stderr, "usage: %s user pass [domain]\n", argv[0]);
	return -1;
    }
    user = argv[1];
    pass = argv[2];
    if( argc > 3 ) {
	domain = argv[2];
    }

    debug_init(DEBUG_INFO, 0, 0);

    i = logon_checkpass(user, pass, domain, buf, sizeof(buf));
    if( i != 0 ) {
	fprintf(stderr, "logon failed: %s\n", buf);
    }
    return i;
}
