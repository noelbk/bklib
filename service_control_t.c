#include <string.h>
#include "service.h"
#include "debug.h"
#include "debug_stdio.h"
#include "mstime.h"
#include "dir.h"

char *exe;

int
main(int argc, char **argv) {
    char *p, exepath[4096];
    int err=1;
    do {
	p = getcwd(exepath, sizeof(exepath));
	assertb_syserr(p);
    
	strncat(exepath, "/service_t.exe", sizeof(exepath));

	debug_init(DEBUG_INFO, 0, 0);
	service_install(exepath, exepath, SERVICE_START_AUTO);
	service_control(exepath, SERVICE_CTRL_START);
	// sleep for a while
	mssleep(10);
	service_control(exepath, SERVICE_CTRL_STOP);
	service_control(exepath, SERVICE_CTRL_UNINSTALL);
	err = 0;
    } while(0);
    return err;
}
