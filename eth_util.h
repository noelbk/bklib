/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#include "sock.h"
#include "netpkt_inet.h"

#define MAX_PKT_LEN 4096

typedef union {
    char a[4];
    int  i;
} ipaddr_t;

typedef union {
    char a[6];
} ethaddr_t;

char *
dot_addr_str(const unsigned char *addr, char *buf, int buflen,
	     int dots, char *templ, char *sep);

#define ETH_ADDR_STR_MAX (6*2+5+1)
char *
eth_addr_str(const void *addr, char *buf, int buflen);


#define IP_ADDR_STR_MAX (6*2+5+1)
char *
ip_addr_str(const void *addr, char *buf, int buflen);


/* from tcpdump-3.3 */
int
in_cksum(const netpkt_ip *ip);


#define ETH_TYPE_STR_MAX (100)
char *
eth_type_str(unsigned int ether_type, char *buf, int buflen);


/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

