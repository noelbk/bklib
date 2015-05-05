#include <stdio.h>
#include "cryptbk.h"
#include "debug.h"

int 
main(int argc, char **argv) { 
    char *dn = "CN=test_cert";
    int i, err=-1;
    cryptbk_t *crypt;
    char buf[4096];

    do {
	debug_init(DEBUG_INFO, 0, 0);
	cryptbk_init();

	if( argc > 1 ) {
	    dn = argv[1];
	}
	crypt = cryptbk_selfsign(argv[1], 0, 2048, 365*24*3600);
	assertb(crypt);
	i = cryptbk_pack(crypt, buf, sizeof(buf), 0,
			 CRYPTBK_MODE_ENC
			 | CRYPTBK_MODE_PUBKEY 
			 | CRYPTBK_MODE_PRIVKEY 
			 | CRYPTBK_MODE_TEXT);
	assertb(i>0);
	printf("%s\n", buf);
	err = 0;
    } while(0);

    cryptbk_fini();
	
    return err;
}
