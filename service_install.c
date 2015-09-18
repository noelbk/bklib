#include <string.h>
#include "service.h"
#include "debug.h"
#include "debug_stdio.h"
#include "mstime.h"
#include "dir.h"
#include "debug.h"

char *exe;

int
main(int argc, char **argv) {
    char exepath[4096], buf[4096];
    int err = -1;
    char *p;

    do {
	if( argc != 3 ) {
	    fprintf(stderr, 
		    "usage: %s exe <start|stop|uninstall|description>\n"
		    "installs a new service\n"
		    , argv[0]);
	    break;
	}

	p = getcwd(buf, sizeof(buf));
	assertb_syserr(p);
	
	snprintf(exepath, sizeof(exepath), "%s/%s", buf, argv[1]);

	if( strcmp(argv[2], "start")==0 ) {
	    service_control(exepath, SERVICE_CTRL_START);
	}
	else if( strcmp(argv[2], "stop")==0 ) {
	    service_control(exepath, SERVICE_CTRL_STOP);
	}
	else if( strcmp(argv[2], "uninstall")==0 ) {
	    service_control(exepath, SERVICE_CTRL_UNINSTALL);
	}
	else {
	    service_install(exepath, argv[2], SERVICE_START_AUTO);
	}
	err = 0;
    } while(0);
    return err;
}
