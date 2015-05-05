#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdlib.h>

#include "warn.h"
#include "sock.h"
#include "ifcfg.h"
#include "defutil.h"

#define MAX_IFRS 1024

struct ifcfg_unix_s {
    int unused;
    struct ifreq ifrs[MAX_IFRS];
};
typedef struct ifcfg_unix_s ifcfg_unix_t;


int
ifcfg_init() {
    return 0;
}

ifcfg_t*
ifcfg_new() {
    ifcfg_unix_t *ic;
    do {
	ic = (ifcfg_unix_t *)malloc(sizeof(*ic));
	assertb(ic);
    } while(0);
    return (ifcfg_t*)ic;
}

void
ifcfg_delete(ifcfg_t *pni) {
    ifcfg_unix_t *ic = (ifcfg_unix_t*)pni;
    do {
	free(ic);
    } while(0);
}

#define DEBUG_FUNC "ifcfg_enum"
int
ifcfg_enum(ifcfg_t *pni, char *if_name, int *idxs, int nidx) {
    ifcfg_unix_t *ic = (ifcfg_unix_t*)pni;
    struct ifconf ifc;
    //struct ifreq ifr;
    int i, j, n=0, err=-1;
    sock_t sock=-1;
    
    do {
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assertb_sockerr(sock>=0);

	if( if_name ) {
	    /* just enumerate one interface */
	    strncpy(ic->ifrs[0].ifr_name, if_name, sizeof(ic->ifrs[0].ifr_name));
	    j = ioctl(sock, SIOCGIFINDEX, &ic->ifrs[0], sizeof(*ic->ifrs));
	    assertb_sockerr(!j);
	    n = 1;
	}
	else {
	    /* get a list of them all */
	    ifc.ifc_req = ic->ifrs;
	    ifc.ifc_len = sizeof(ic->ifrs);
	    i = ioctl(sock, SIOCGIFCONF, &ifc, sizeof(ifc));
	    assertb_sockerr(!i);
	    assertb(ifc.ifc_len <= (int)sizeof(ic->ifrs));
	    n = ifc.ifc_len / sizeof(*ic->ifrs);
	}
	for(i=0; i<n && i<nidx; i++) {
	    idxs[i] = i;
	}
	assertb(i==n);
	err = 0;
    } while(0);
    if( sock >=0 ) {
	sock_close(sock);
    }

    return err ? -1 : n;
}
#undef DEBUG_FUNC

int 
ifcfg_get(ifcfg_t *pni, int if_idx, ifcfg_config_t *cfg) {
    ifcfg_unix_t *ic = (ifcfg_unix_t*)pni;
    int i, err=-1;
    struct ifreq ifr;
    sock_t sock =-1;
    do {
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assertb_sockerr(sock>=0);

	memset(cfg, 0, sizeof(*cfg));
	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, ic->ifrs[if_idx].ifr_name, sizeof(ifr.ifr_name));

	/* name */
	strncpy(cfg->name, ic->ifrs[if_idx].ifr_name, sizeof(cfg->name));
	cfg->fields |= IFCFG_CONFIG_NAME;

	/* flags */
	i = ioctl(sock, SIOCGIFFLAGS, &ifr, sizeof(ifr));
	assertb_sockerr(!i);
	if( !(ifr.ifr_flags & IFF_UP) ) {
	    //err = 0;
	    //break;
	}

	/* ipaddr */
	i = ioctl(sock, SIOCGIFADDR, &ifr, sizeof(ifr));
	if( i == 0 ) {
	    assertb(ifr.ifr_addr.sa_family == AF_INET);
	    cfg->ipaddr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
	    cfg->fields |= IFCFG_CONFIG_IPADDR;
	}

	/* netmask */
	i = ioctl(sock, SIOCGIFNETMASK, &ifr, sizeof(ifr));
	if( i == 0 ) {
	    assertb(ifr.ifr_addr.sa_family == AF_INET);
	    cfg->netmask = ((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr;
	    cfg->fields |= IFCFG_CONFIG_NETMASK;
	}

	/* broadcast? */
	i = ioctl(sock, SIOCGIFBRDADDR, &ifr, sizeof(ifr));
	if( i == 0 ) {
	    assertb(ifr.ifr_addr.sa_family == AF_INET);
	    cfg->broadcast = ((struct sockaddr_in*)&ifr.ifr_broadaddr)->sin_addr;
	    cfg->fields |= IFCFG_CONFIG_BROADCAST;
	}

	/* dns? */
	//cfg->fields |= IFCFG_CONFIG_DNS;	
	
	/* gateway? */
	//cfg->fields |= IFCFG_CONFIG_GATEWAY;

	/* dhcp? */
	//cfg->fields |= IFCFG_CONFIG_DHCP;

	/* ethaddr */
	i = ioctl(sock, SIOCGIFHWADDR, &ifr, sizeof(ifr));
	if( i == 0 ) {
	    //assertb(ifr.ifr_hwaddr.sa_family == AF_ETHER);
	    memcpy(cfg->ethaddr, ifr.ifr_hwaddr.sa_data, sizeof(cfg->ethaddr));
	    cfg->fields |= IFCFG_CONFIG_ETHADDR;
	}
	
	/* mtu */
	i = ioctl(sock, SIOCGIFMTU, &ifr, sizeof(ifr));
	if( i == 0 ) {
	    cfg->mtu = ifr.ifr_mtu;
	    cfg->fields |= IFCFG_CONFIG_MTU;
	}

	err = 0;
    } while(0);
    if( sock >=0 ) {
	sock_close(sock);
    }
    return err;
}

/* returns <0 on error, 1 iff reboot required */
#define DEBUG_FUNC "ifcfg_config"
int 
ifcfg_set(ifcfg_t *pni, int if_idx, ifcfg_config_t *cfg) {
    ifcfg_unix_t *ic = (ifcfg_unix_t*)pni;

    int i;
    int err = -1;
    sock_t sock = -1;
    struct ifreq ifr;
    int flags=0;
    int got_flags=0;

    do {
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assertb_sockerr(sock>=0);

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ic->ifrs[if_idx].ifr_name, sizeof(ifr.ifr_name));
	
	// save flags
	i = ioctl(sock, SIOCGIFFLAGS, &ifr, sizeof(ifr));
	assertb_sockerr(!i);
	flags = ifr.ifr_flags;
	got_flags = 1;
	
	// bring the interface down
	ifr.ifr_flags &= ~IFF_UP;
	i = ioctl(sock, SIOCSIFFLAGS, &ifr, sizeof(ifr));
	assertb_sockerr(!i);

	if( cfg->fields & IFCFG_CONFIG_IPADDR ) {
	    flags |= IFF_UP;
	    ifr.ifr_addr.sa_family = AF_INET;
	    ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr = cfg->ipaddr;
	    i = ioctl(sock, SIOCSIFADDR, &ifr, sizeof(ifr));
	    assertb_sockerr(!i);

	    ifr.ifr_netmask.sa_family = AF_INET;
	    ((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr = cfg->netmask;
	    i = ioctl(sock, SIOCSIFNETMASK, &ifr, sizeof(ifr));
	    assertb_sockerr(!i);
	}
	else {
	    flags &= ~IFF_UP;
	}

	if( cfg->fields & IFCFG_CONFIG_BROADCAST ) {
	    ifr.ifr_broadaddr.sa_family = AF_INET;
	    ((struct sockaddr_in*)&ifr.ifr_broadaddr)->sin_addr = cfg->broadcast;
	    i = ioctl(sock, SIOCSIFBRDADDR, &ifr, sizeof(ifr));
	    assertb_sockerr(!i);
	}

	if( cfg->fields & IFCFG_CONFIG_GATEWAY ) {
	    // todo - route
	}
	
	if( cfg->fields & IFCFG_CONFIG_DNS ) {
	    // todo - rewrite /etc/resolv.conf
	}

	if( cfg->fields & IFCFG_CONFIG_DNS_DOMAIN ) {
	    // todo - rewrite /etc/resolv.conf
	}

	/* mtu */
	if( cfg->fields & IFCFG_CONFIG_MTU ) {
	    ifr.ifr_mtu = cfg->mtu;
	    i = ioctl(sock, SIOCSIFMTU, &ifr, sizeof(ifr));
	    assertb_sockerr(!i);
	}

	err = 0;
    } while(0);
    if( got_flags ) {
	do {
	    // bring it back up
	    ifr.ifr_flags = flags;
	    i = ioctl(sock, SIOCSIFFLAGS, &ifr, sizeof(ifr));
	    assertb_sockerr(!i);
	} while(0);
    }
    if( sock >= 0 ) {
	sock_close(sock);
    }
    return err ;
}
#undef DEBUG_FUNC

