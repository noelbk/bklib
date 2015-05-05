#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "warn.h"
#include "defutil.h"

int
main(int argc, char **argv) {
    cfg_t *cfg=0;
    char buf[4096], *p;
    int i, n;
    int where;
    char *argv_def[] = {
	"k0",
	"k1=v1",
	"k2=v2",
	"k3=v3",
    };

    do {
	cfg = cfg_new("cfg_t");
	assertb(cfg);
    
	cfg_dir(cfg, CFG_DIR_USER, buf, sizeof(buf));
	printf("CFG_DIR_USER=%s\n", buf);
	cfg_dir(cfg, CFG_DIR_GLOBAL, buf, sizeof(buf));
	printf("CFG_DIR_GLOBAL=%s\n", buf);
	cfg_dir(cfg, CFG_DIR_VAR, buf, sizeof(buf));
	printf("CFG_DIR_VAR=%s\n", buf);

	if(argc <= 1) {
	    argv = argv_def;
	    argc = NELTS(argv_def);
	}

	for(where=0; where<2; where++) {
	    for(i=1; i<argc; i++) {
		char *arg = argv[i];
		p = strchr(arg, '=');
		if( p ) {
		    *p++ = 0;
		    n = cfg_put(cfg, arg, p, -1, where == 0 ? CFG_PUT_GLOBAL : CFG_PUT_USER);
		    printf("cfg_put n=%d key=[%s] val=[%s]\n", n, arg, p);
		}
		else {
		    n = cfg_get(cfg, arg, buf, sizeof(buf));
		    printf("cfg_get n=%d key=[%s] val=[%s]\n", n, arg, buf);
		}
		assertb(n>=0);
	    }
	}
    } while(0);
    
    if( cfg ) {
	cfg_delete(cfg);
    }
}
