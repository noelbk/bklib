#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "cryptbk.h"
#include "debug.h"

int
main(int argc, char **argv) {
    char *buf=0;
    int buflen;
    char md5_buf[1024];
    FILE *f;
    struct stat st;
    int argi;
    int i;
    
    debug_init(DEBUG_INFO, 0, 0);
    cryptbk_init();
	
	
    for(argi=1; argi<argc; argi++) {
	f = 0;
	buf = 0;
	do {
	    i = stat(argv[argi], &st);
	    assertb(i==0);
	    f = fopen(argv[argi], "rb");
	    assertb(f);
	    buflen = st.st_size;
	    buf = (char*)malloc(buflen);
	    assertb(buf);
	    i = fread(buf, buflen, 1, f);
	    assertb(i>0);
	    
	    // test md5
	    i = cryptbk_crypt(0, buf, buflen,
			      md5_buf, sizeof(md5_buf), 
			      CRYPTBK_MODE_ENC 
			      | CRYPTBK_MODE_TEXT
			      | CRYPTBK_MODE_MD5);
	    printf("%s %s\n", md5_buf, argv[argi]);
	    i = cryptbk_crypt(0, buf, buflen,
			      md5_buf, sizeof(md5_buf), 
			      CRYPTBK_MODE_DEC
			      | CRYPTBK_MODE_TEXT
			      | CRYPTBK_MODE_MD5);
	    printf(" md5 check: i=%d\n", i);
	} while(0);
	if( f ) fclose(f);
	if( buf ) free(buf);
    }
    return 0;
}





