#ifndef IFCFG_H_INCLUDED
#define IFCFG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "sock.h"

typedef void ifcfg_t;

int
ifcfg_init();

ifcfg_t*
ifcfg_new();

void
ifcfg_delete(ifcfg_t *pni);

// returns an array of adapters that match if_name, or all adapters if
// if_name is 0
int
ifcfg_enum(ifcfg_t *pni, char *if_name, int *if_idx, int nidx);

enum ifcfg_config_fields_t {
    IFCFG_CONFIG_IPADDR  =1<<0,
    IFCFG_CONFIG_NETMASK =1<<1,
    IFCFG_CONFIG_GATEWAY =1<<2,
    IFCFG_CONFIG_DNS     =1<<3,
    IFCFG_CONFIG_ETHADDR =1<<4,
    IFCFG_CONFIG_NAME    =1<<5,
    IFCFG_CONFIG_DNS_DOMAIN =1<<6,
    IFCFG_CONFIG_MTU     =1<<7,
    IFCFG_CONFIG_BROADCAST =1<<8,
    IFCFG_CONFIG_DHCP =1<<9,
    IFCFG_CONFIG_DESCRIPTION =1<<10,
    IFCFG_CONFIG_CAPTION =1<<11,
};

#define IFCFG_DNS_SEARCH_MAX 100
typedef unsigned long u32_t;
struct ifcfg_config_s {
    int            fields;
    struct in_addr ipaddr;
    struct in_addr netmask;
    struct in_addr broadcast;
    struct in_addr gateway;
    struct in_addr dns_search[IFCFG_DNS_SEARCH_MAX];
    int dns_search_count;
    unsigned char  ethaddr[6];
    char           name[256];
    char           description[1024];
    char           caption[1024];
    char           dns_domain[1024];
    int            mtu;
    int            dhcp;
};
typedef struct ifcfg_config_s ifcfg_config_t;

char*
ifcfg_config_fmt(ifcfg_config_t *cfg, char *buf, int len);

int 
ifcfg_set(ifcfg_t *pni, int if_idx, ifcfg_config_t *cfg);

int 
ifcfg_get(ifcfg_t *pni, int if_idx, ifcfg_config_t *cfg);

#if OS & OS_WIN32 
typedef struct {
    char *name;
    char *val;
} ifcfg_prop_list_t;
	
int 
ifcfg_get_prop_list(ifcfg_t *pni, int if_idx, 
		    ifcfg_prop_list_t *props,
		    char *buf, int len);
int 
ifcfg_get_prop(ifcfg_t *pni, int if_idx, 
	       char *prop_name, 
	       char *prop_val, int prop_val_len);

#endif

int
ifcfg_is_ethertap(ifcfg_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif // IFCFG_H_INCLUDED
