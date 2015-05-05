#include <stdio.h>

#include "sock.h"
#include "ifcfg.h"
#include "warn.h"


/* loop forever to test for memory leaks */
int
test_loop() {
    ifcfg_t *ic;
    int idx[200];
    int i, n, err=-1;

    while(1) {
	ic = ifcfg_new();
	assertb(ic);
	n = ifcfg_enum(ic, 0, idx, sizeof(idx)/sizeof(*idx));
	for(i=0; i<n; i++) {
	    ifcfg_config_t cfg;
	    
	    ifcfg_get(ic, idx[i], &cfg);
#if 0
	    printf("ifcfg_get idx=%d", idx[i]);
	    if( !cfg.fields )  {
		printf("\n");
		continue;
	    }
	    printf(" ip=%s", inet_ntoa(cfg.ipaddr));
	    printf(" net=%s", inet_ntoa(cfg.netmask));
	    printf(" gw=%s", inet_ntoa(cfg.gateway));
	    printf(" dns=%s", inet_ntoa(cfg.dns));
	    printf(" mtu=%d", cfg.mtu);
	    printf(" dhcp=%d", cfg.dhcp);
	    printf(" ether=%02X:%02X:%02X:%02X:%02X:%02X", 
		   (int)cfg.ethaddr[0],
		   (int)cfg.ethaddr[1],
		   (int)cfg.ethaddr[2],
		   (int)cfg.ethaddr[3],
		   (int)cfg.ethaddr[4],
		   (int)cfg.ethaddr[5]);
	    printf(" name=[%s]", cfg.name);
	    printf("\n");
#endif
	}
	ifcfg_delete(ic);
	ic = 0;
	err = 0;
    }
    return err;
}

int
main(int argc, char **argv) {
    ifcfg_t *ic;
    int idx[200];
    ifcfg_config_t cfg;
    char *if_name=0;
    int argi;
    int i, k, n, err=-1;

    do {
	debug_init(DEBUG_INFO+2, 0, 0);
	ifcfg_init();

	argi=1;
 	if( argi<argc ) {
	    if_name = argv[argi++];
	    if( strcmp(if_name, "-loop")==0 ) {
		test_loop();
	    }
	}

	ic = ifcfg_new();
	assertb(ic);
	n = ifcfg_enum(ic, if_name, idx, sizeof(idx)/sizeof(*idx));
	for(i=0; i<n; i++) {
	    ifcfg_get(ic, idx[i], &cfg);
	    printf("ifcfg_get idx=%d", idx[i]);
	    if( !cfg.fields )  {
		printf("\n");
		continue;
	    }
	    printf(" ip=%s", inet_ntoa(cfg.ipaddr));
	    printf(" net=%s", inet_ntoa(cfg.netmask));
	    printf(" gw=%s", inet_ntoa(cfg.gateway));
	    for(k=0; k<cfg.dns_search_count; k++) {
		printf(" dns=%s", inet_ntoa(cfg.dns_search[k]));
	    }
	    printf(" mtu=%d", cfg.mtu);
	    printf(" dhcp=%d", cfg.dhcp);
	    printf(" ether=%02X:%02X:%02X:%02X:%02X:%02X", 
		   (int)cfg.ethaddr[0],
		   (int)cfg.ethaddr[1],
		   (int)cfg.ethaddr[2],
		   (int)cfg.ethaddr[3],
		   (int)cfg.ethaddr[4],
		   (int)cfg.ethaddr[5]);
	    printf(" name=[%s]", cfg.name);
	    printf("\n");
	}

	if( argc <= 2 ) {
	    err = 0;
	    break;
	}

	// configure the first interface like:
	// icfg_t ifname ipaddr netmask
	memset(&cfg, 0, sizeof(cfg));
	
	assertb(n==1);

	if( argi<argc ) {
	    cfg.fields |= IFCFG_CONFIG_IPADDR;
	    cfg.ipaddr.s_addr = inet_addr(argv[argi++]);
	}
	if( argi<argc ) {
	    cfg.fields |= IFCFG_CONFIG_NETMASK;
	    cfg.netmask.s_addr = inet_addr(argv[argi++]);
	}
	
#if 0
	cfg.fields |= IFCFG_CONFIG_GATEWAY;
	cfg.gateway.s_addr = 0;
#endif

	i = ifcfg_set(ic, idx[0], &cfg);
	assertb(i>=0);

	n = ifcfg_enum(ic, if_name, idx, sizeof(idx)/sizeof(*idx));
	err = 0;
    } while(0);
    if( ic ) ifcfg_delete(ic);
    return err;
}
