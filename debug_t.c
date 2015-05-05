#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dir.h"
#include "debug.h"
#include "debug_stdio.h"

int
debug_out(char *buf, void *arg) {
    fprintf(stderr, "%s(%s)", (char*)arg, buf);
    return 1;
}

int
main() {
    int i;
    char buf[4096];

    getcwd(buf, sizeof(buf));
    strncat(buf, "/log/debug_t", sizeof(buf));
    
    debug_init(DEBUG_MAX, debug_func_log, 
	       debug_func_log_init(buf, 3, 1000, stderr) );
    do {
	assertb("this assert should work" == 0);
    } while(0);
    debugf("Here's some debug_v text\n");
    debug(DEBUG_ERROR, ("An error message\n"));

    debug(DEBUG_INFO, ("Now to test the log files\n"));

    for(i=0; i<10000; i++) {
	debug(DEBUG_INFO, ("log line #%d\n", i));
    }
    return 0;
}
