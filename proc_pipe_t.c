#include "proc.h"
#include "debug.h"
#include "sock.h"

int
main() {
    procid_t pid;
    char buf[4096];
    int stdin, stdout;
    int i;

    do {
	debug_init(DEBUG_INFO, 0, 0);

	pid = proc_start_stdpipe(0, "sox -t ossdsp /dev/audio -t ogg -", 0,
				 &stdin, &stdout);
	assertb(pid>0);
	close(stdin);
	
	i = proc_stdpipe_read(stdout, buf, sizeof(buf));
	debug(DEBUG_INFO, ("audio pid=%ld stdout=%d read=%d\n",
			   pid, stdout, i));

	proc_kill(pid, SIGTERM);

    } while(0);
    return 0;
}

    
