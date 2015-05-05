#include <stdio.h>
#include <ctype.h>

#include "configbk.h"
#include "itypes.h"
#include "ifcfg.h"
#include "array.h"
#include "debug.h"
#include "defutil.h"

int
ifcfg_is_ethertap(ifcfg_config_t *cfg) {
    return 0 
	|| ((cfg->fields & IFCFG_CONFIG_DESCRIPTION) 
	    && strncasecmp(cfg->description, "EtherTap", 8)==0)
	|| (strncasecmp(cfg->name, "ethertap", 8)==0)
	|| (strncasecmp(cfg->name, "tap", 3)==0 && isdigit(cfg->name[3]))
	;
}

// get local DNS addrs from ifcfg_enum and resolv.conf (unix).  Adds
// dns addrs to an array of u32 addrs in host order (not network
// order).  The caller must call 
//
// array_init(dns_addrs, sizeof(u32), 0);
//
int
ifcfg_get_dns_addrs(array_t *dns_addrs) {
    FILE *resolv=0;
    int i, j, k, n, err=-1;
    ifcfg_t *ic=0;
    int idx[200];
    u32 *pip, ip;

#if OS & OS_UNIX
    char buf[4096], addr_buf[4096], *p;
#endif // OS & OS_UNIX

    do {
	// add all dns addressses by network interface (windows)
	ic = ifcfg_new();
	assertb(ic);
	n = ifcfg_enum(ic, 0, idx, NELTS(idx));
	for(i=0; i<n; i++) {
	    ifcfg_config_t cfg;
	    j = ifcfg_get(ic, idx[i], &cfg);
	    assertb(!j);
	    for(k=0; k<cfg.dns_search_count; k++) {
		ip = ntohl(cfg.dns_search[k].s_addr);
		if( !(cfg.fields & IFCFG_CONFIG_DNS) || !ip ) continue;
		pip = (u32*)array_add(dns_addrs, 1);
		assertb(pip);
		*pip = ip;
	    }
	}

#if OS & OS_UNIX
	resolv = fopen("/etc/resolv.conf", "r");
	if( !resolv ) {
	    err = 0;
	    break;
	}
	while(1) {
	    p = fgets(buf, sizeof(buf), resolv);
	    if( !p ) {
		err = 0;
		break;
	    }
	    i = sscanf(buf, "nameserver %s", addr_buf);
	    if( i!=1 ) {
		continue;
	    }

	    ip = inet_addr(addr_buf);
	    assertb(ip && ip != INADDR_NONE);
	    pip = (u32*)array_add(dns_addrs, 1);
	    assertb(pip);
	    *pip = ip;
	}
	assertb(!err);
	err = -1;
	
	fclose(resolv);
	resolv=0;

#endif // OS & OS_UNIX

	err = 0;
    } while(0);
    if( resolv ) fclose(resolv);
    if( ic ) ifcfg_delete(ic);
    return err;
}

#if 0

// enumerate all local addresses, skip loopback and my ethertap address
int
ifcfg_get_local_addrs(array_t *local_addrs, int *pmin_mtu) {
    int i, err=-1;
    ifcfg_t *ic=0;
    int idx[200];
    u32 *pip, ip;
    char buf1[4096];
    int min_mtu;
    int ifcfg_idx, ifcfg_n;
    ifcfg_config_t cfg;

    do {
	array_init(local_addrs, sizeof(u32), 0);

	min_mtu = 1500;

	ic = ifcfg_new();
	assertb(ic);
	ifcfg_n = ifcfg_enum(ic, 0, idx, NELTS(idx));
	assertb(ifcfg_n>0);
	for(ifcfg_idx=0; ifcfg_idx<ifcfg_n; ifcfg_idx++) {
	    
	    i = ifcfg_get(ic, idx[ifcfg_idx], &cfg);
	    assertb(i==0);

	    // skip ethertap and non-ip interfaces
	    if( !cfg.fields || !cfg.ipaddr.s_addr )  continue;
	    //if( ifcfg_is_ethertap(&cfg) ) continue;

	    // find lowest mtu
	    if(cfg.fields & IFCFG_CONFIG_MTU
	       && cfg.mtu > 0
	       && cfg.mtu < min_mtu) {
		min_mtu = cfg.mtu;
	    }

	    // skip ethertap and loopback interfaces
	    ip = ntohl(cfg.ipaddr.s_addr);
	    if( !ip
		|| ipaddr_netbits(ip, IADDR(127,0,0,1), 8)
		) {
		continue;
	    }

	    debug(DEBUG_INFO,
		  ("ifcfg_get_local_addrs: local_addr=%s\n",
		   iaddr_ntoa(htonl(ip), buf1)
		   ));

	    pip = (u32*)array_add(local_addrs, 1);
	    assertb(pip);
	    *pip = ip;
	}
	err = 0;
    } while(0);
    if( ic ) ifcfg_delete(ic);
    return err;
}

#endif

char*
ifcfg_config_fmt(ifcfg_config_t *cfg, char *buf, int len) {
    char *p = buf;
    int   n = len;
    int i; //, err=-1;
    do {
	if( cfg->fields & IFCFG_CONFIG_IPADDR ) {
	    i = snprintf(p, n, " ip=%s", inet_ntoa(cfg->ipaddr));
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_NETMASK ) {
	    i = snprintf(p, n, " net=%s", inet_ntoa(cfg->netmask));
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_GATEWAY ) {
	    i = snprintf(p, n, " gw=%s", inet_ntoa(cfg->gateway));
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_DNS ) {
	    int idx;
	    i = snprintf(p, n, " dns=[");
	    BUF_ADD(p, n, i);
	    for(idx=0; idx<cfg->dns_search_count; idx++) {
		i = snprintf(p, n, "%s%s", (idx>0 ? ", ": ""), inet_ntoa(cfg->dns_search[idx]));
		BUF_ADD(p, n, i);
	    }
	    i = snprintf(p, n, "]");
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_MTU ) {
	    i = snprintf(p, n, " mtu=%d", cfg->mtu);
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_DHCP ) {
	    i = snprintf(p, n, " dhcp=%d", cfg->dhcp);
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_IPADDR ) {
	    i = snprintf(p, n, " ether=%02X:%02X:%02X:%02X:%02X:%02X", 
			 (int)cfg->ethaddr[0],
			 (int)cfg->ethaddr[1],
			 (int)cfg->ethaddr[2],
			 (int)cfg->ethaddr[3],
			 (int)cfg->ethaddr[4],
			 (int)cfg->ethaddr[5]);
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_NAME ) {
	    i = snprintf(p, n, " name=[%s]", cfg->name);
	    BUF_ADD(p, n, i);
	}
	if( cfg->fields & IFCFG_CONFIG_DESCRIPTION ) {
	    i = snprintf(p, n, " description=[%s]", cfg->description);
	    BUF_ADD(p, n, i);
	}
	//err = 0;
    } while(0);
    buf[len-1] = 0;
    return buf;
}
