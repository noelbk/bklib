#include <stdio.h>

#include "bklib/debug.h"
#include "bklib/defutil.h"
#include "bklib/ipconfig.h"

int
main(int argc, char **argv) {
    int i, n, err=-1;
    ifcfg_config_t cfg[128];
    char buf[4096];

    debug_init(DEBUG_INFO, 0, 0);

    do {
	n  = ipconfig_get_all(cfg, NELTS(cfg));
	for(i=0; i<n; i++) {
	    printf("cfg: %s\n", ifcfg_config_fmt(cfg+i, buf, sizeof(buf)));
	}
	err = 0;
    } while(0);
    return err;
}
