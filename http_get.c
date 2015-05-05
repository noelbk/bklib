#include <stdio.h>
#include "sock.h"
#include "http.h"
#include "http_req.h"
#include "debug.h"

int
http_req_func(http_req_t *req, char *buf, int len, void *arg) {
    char buf1[1024];
    printf("%s len=%d\n"
	   ,http_req_state_fmt(req->req_state, buf1, sizeof(buf1))
	   ,len);
    fwrite(buf, 1, len, stdout);
    printf("\n");
    return 0;
}


int
main(int argc, char **argv) {
    do {
	debug_init(DEBUG_INFO, 0, 0);
	sock_init();

	assertb(argc>1);
	http_get(argv[1], http_req_func, 0);
    } while(0);
    return 0;
}
