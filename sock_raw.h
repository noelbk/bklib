/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

/*
  sock_raw.h - functions for raw socket shenannigans
  Noel Burton-Krahn
  <noel@burton-krahn.com> Feb 07, 2002
  
*/

#ifndef SOCK_RAW_H_INLCUDED
#define SOCK_RAW_H_INLCUDED

#include "sock.h"
#include "netpkt_inet.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int sock_bindtodevice(sock_t sock, char *ifname);
sock_t sock_openraw(char *ifname, int promisc);
int sock_raw_read(sock_t sock, char *buf, int len);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // SOCK_RAW_H_INLCUDED
/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

