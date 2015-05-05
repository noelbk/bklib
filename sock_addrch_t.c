#include "bklib/debug.h"
#include "bklib/fdselect.h"
#include "bklib/sock.h"
#include "bklib/defutil.h"

int
addr_change(fdselect_t *sel, fd_t fd, int state, void *arg) {
    debug(DEBUG_INFO, ("addr_change\n"));
    return 0;
}

int main() {
    int i, err=-1;
    int sock;
    fdselect_t fdselect;
    
    do {
 	debug_init(DEBUG_INFO, 0, 0);
 	sock_init();
 	fdselect_init(&fdselect);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(sock>=0);
	i = sock_nonblock(sock, 1);
	assertb(i==0);
	
// 	dw = 0;
// 	i = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, 0, 0, 0, 0, &dw, 0, 0);
// 	i = WSAGetLastError();
// 	p = sock_err2str(i);

 	i = fdselect_set(&fdselect, sock, FDSELECT_ADDRESS_LIST_CHANGE, 
 			 addr_change, 0);
 	assertb(i>=0);

 	while(1) {
 	    fdselect_select(&fdselect, 0);
 	}
	err = 0;
    } while(0);
    return err;
}
