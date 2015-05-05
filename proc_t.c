#include <stdio.h>

#include "bklib/proc.h"
#include "bklib/debug.h"


int
main(int argc, char **argv) {
    char stdin_buf[4096], stdout_buf[65535];
    int stdin_len, stdout_len;
    char *cmdline = "ipconfig /all";
    int status;
    int i;

    debug_init(DEBUG_INFO, 0, 0);

#if 0
    proc_shellex(0, argv[1], 0);
#endif

#if 0
    p = stdout_buf;
    for(i=1; i<argc; i++) {
        p += sprintf(p, "\"%s\" ", argv[1]);
    }
    fread(stdin_buf, 1, 4096, stdin);
#endif

#if 1
    if( argc > 1 ) {
	cmdline = argv[1];
    }
    stdin_len = 0;
    stdout_len = sizeof(stdout_buf);
    i = proc_run_stdbuf(cmdline, &status, 0,
                        stdin_buf, stdin_len, 
                        stdout_buf, sizeof(stdout_buf));
    printf("proc_run_stdbuf: cmdline=%s i=%d stdout_buf=\n%s\n"
           , cmdline, i, stdout_buf);
#endif

}
