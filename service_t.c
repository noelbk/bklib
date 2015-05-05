#include "service.h"
#include "debug.h"
#include "debug_stdio.h"
#include "mstime.h"

int
service_main_func(int argc, char **argv) {
    char buf[1024];
    while( !service_exited ) {
	debug(DEBUG_INFO, 
	      ("service: main time=%s\n",
	       mstime_fmt(mstime(), buf, sizeof(buf))
	       ));
	mssleep(1);
    }
    debug(DEBUG_INFO, ("service: stopping\n"));
    return 0;
}

int
main(int argc, char **argv) {
    FILE *f=0;
    int i;

    do {
	f = fopen("c:/tmp/service_t", "w");
	debug_init(DEBUG_INFO, debug_func_stdio, f);

	debug(DEBUG_INFO, 
	      ("service_t main argc=%d argv[0]=%s\n", 
	       argc, argv[0]))

	i = service_init(argc, argv, service_main_func);
    } while(0);
    if( f ) fclose(f);
    return i;
}
