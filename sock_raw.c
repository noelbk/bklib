/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "warn.h"
#include "sock_raw.h"

int
sock_bindtodevice(int sock, char *ifname) {
    struct ifreq ifr;
    int i, err=-1;
    
    do {
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	i = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
		       (char *)&ifr, sizeof(ifr));
	assertb_sockerr(i==0);
	err = 0;
    } while(0);
    return err;
}

int
sock_raw_read(sock_t sock, char *buf, int len) {
    struct sockaddr_ll addr_ll;
    int i, err=-1;

    do {
	i = sizeof(addr_ll);
	err = recvfrom(sock, buf, len, 0, 
		       (struct sockaddr*)&addr_ll, (socklen_t*)&i);
	assertb_sockerr(err>=0);
    } while(0);
    return err;
}

int
sock_openraw(char *ifname, int promisc) {
    struct ifreq ifr;
    int i, err=-1;
    int ifidx;
    int sock;
    struct sockaddr_ll addr;


    do {
	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	assertb_sockerr(sock>=0);

	/* find ifindex for ifname */
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
	i = ioctl(sock, SIOCGIFINDEX, &ifr, sizeof(ifr));
	assertb_sockerr(i==0);
	ifidx = ifr.ifr_ifindex;
	
	/* bind sock to ifname */
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ALL);
	addr.sll_ifindex = ifidx;
	i = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	assertb_sockerr(i==0);

	if( promisc ) {
	    /* make ifname promiscuous */
	    i = ioctl(sock, SIOCGIFFLAGS, &ifr, sizeof(ifr));
	    assertb_sockerr(i==0);
	    ifr.ifr_flags |= IFF_PROMISC;
	    i = ioctl(sock, SIOCSIFFLAGS, &ifr, sizeof(ifr));
	    assertb_sockerr(i==0);
	}
	
	err = 0;
    } while(0);

    if( err ) {
	close(sock);
	sock = -1;
    }

    return sock;
}
/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

