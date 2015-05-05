/* included by ipconfig.c */

#include <stdio.h>

#include "bklib/proc.h"
#include "bklib/debug.h"
#include "bklib/sock.h"
#include "bklib/ifcfg.h"
#include "bklib/defutil.h"

int
ipconfig_parse_ip(char *p, struct in_addr *addr) {
    char *orig = p;
    long l;
    int i;
    char buf[128];
    
    p += strspn(p, " \t,");
    i = strspn(p, "0123456789.");
    if( i > sizeof(buf)-1 ) {
	return 0;
    }
    strncpy(buf, p, i);
    buf[i] = 0;
    l = ntohl(inet_addr(buf));
    if( l == INADDR_NONE ) {
	return 0;
    }
    addr->s_addr = htonl(l);
    return p - orig + i;
}

int
ipconfig_parse_ip_list(char *p, struct in_addr *addrs, int max, int *count) {
    char *orig = p;
    int i;

    *count = 0;
    while(*p && *count<max) {
	p += strspn(p, " \t,");
	i = ipconfig_parse_ip(p, addrs);
	if( i<=0 ) {
	    break;
	}
	p += i;
	*count = *count + 1;
	addrs++;
    }
    return p - orig;
}

typedef struct ipconfig_t ipconfig_t;
struct ipconfig_t {
    ifcfg_config_t *cfg;
    int cfg_max;
    int cfg_count;
    int in_dns;
    int value_column;
};


int
ipconfig_parse_label(char **pp, char *label) {
    char *p = *pp;
    int i;

    i = strlen(label);
    if( strncasecmp(p, label, i) != 0 ) {
	return 0;
    }
    
    p += i;
    p += strspn(p, " \t.:");

    *pp = p;

    return 1;
}


int
ipconfig_parse_line(ipconfig_t *ipconfig, char *p) {
    char *orig = p;
    ifcfg_config_t *cfg;
    int i;

    while( p && *p ) {

	/* skip leading whitespace */
	i = strspn(p, " \t");

	if( i == 0 ) {
	    /* if no whitespace, new interface block */

	    if( ipconfig->cfg_count >= ipconfig->cfg_max ) {
		break;
	    }

	    ipconfig->cfg_count++; 

	    cfg = &ipconfig->cfg[ipconfig->cfg_count-1];
	    i = strcspn(p, "\r\n");
	    cfg->fields |= IFCFG_CONFIG_NAME;
	    if( i > sizeof(cfg->name)-1 ) {
		i = sizeof(cfg->name)-1;
	    }
	    strncpy(cfg->name, p, i);
	    if( cfg->name[i-1] == ':' ) {
		i--;
	    }
	    cfg->name[i] = 0;
	    break;
	}
	
	if( ipconfig->cfg_count <= 0 ) {
	    /* no confg block yet, ignore this line */
	    break;
	}

	cfg = &ipconfig->cfg[ipconfig->cfg_count-1];

	/* skip leading whitespace */
	p += i;
	
	/* DNS Servers appear on multiple lines, with IPs starting in the same column */
	if( ipconfig->in_dns && i != ipconfig->value_column ) {
	    ipconfig->in_dns = 0;
	}

	if( ipconfig_parse_label(&p, "DNS Servers") ) {
	    ipconfig->value_column = p - orig;
	    ipconfig->in_dns = 1;
	}
	if( ipconfig->in_dns ) {
	    int count = 0;
	    i = ipconfig_parse_ip_list(p, 
				       &cfg->dns_search[cfg->dns_search_count], 
				       IFCFG_DNS_SEARCH_MAX-cfg->dns_search_count, 
				       &count);
	    if( i>=0 && count > 0 ) {
		cfg->fields |= IFCFG_CONFIG_DNS;
		cfg->dns_search_count += count;
	    }
	}
	else if( ipconfig_parse_label(&p, "Subnet Mask") ) {
	    i = ipconfig_parse_ip(p, &cfg->netmask);
	    if( i > 0 ) {
		cfg->fields |= IFCFG_CONFIG_NETMASK;
	    }
	}
	else if( ipconfig_parse_label(&p, "IP Address") ) {
	    i = ipconfig_parse_ip(p, &cfg->ipaddr);
	    if( i > 0 ) {
		cfg->fields |= IFCFG_CONFIG_IPADDR;
	    }
	}
	else if( ipconfig_parse_label(&p, "Physical Address") ) {
	    unsigned int x[6];
	    i = sscanf(p, "%x-%x-%x-%x-%x-%x"
		       ,&x[0],&x[1],&x[2],&x[3],&x[4],&x[5]);
	    if( i == 6 ) {
		cfg->fields |= IFCFG_CONFIG_ETHADDR;
		for(i=0; i<6; i++) {
		    cfg->ethaddr[i] = x[i];
		}
	    }
	}
	else if( ipconfig_parse_label(&p, "Default Gateway") ) {
	    i = ipconfig_parse_ip(p, &cfg->gateway);
	    if( i > 0 ) {
		cfg->fields |= IFCFG_CONFIG_GATEWAY;
	    }
	}
	else if( ipconfig_parse_label(&p, "Description") ) {
	    cfg->fields |= IFCFG_CONFIG_DESCRIPTION;
	    i = strcspn(p, "\r\n");
	    if( i > sizeof(cfg->description)-1 ) {
		i = sizeof(cfg->description)-1;
	    }
	    strncpy(cfg->description, p, i);
	    cfg->description[i] = 0;
	}
	break;
    }
    return 1;
}

int
ipconfig_get_all(ifcfg_config_t *cfg, int cfg_max) {
    ipconfig_t ipconfig;
    char stdout_buf[65535], *line, *line_end;
    int status;
    int i, err=-1;
    char *p, *q;

    do {
	memset(&ipconfig, 0, sizeof(ipconfig));
	ipconfig.cfg = cfg;
	ipconfig.cfg_max = cfg_max;
    
	i = proc_run_stdbuf("ipconfig /all", 
			    &status, 0,
			    0, 0, 
			    stdout_buf, sizeof(stdout_buf));
	assertb(i>0);

	/* strip '\r' */
	for(p=stdout_buf, q=p; *p; p++) {
	    if( *p == '\r' ) continue;
	    *q = *p;
	    *q++;
	}
	*q = 0;

	debug(DEBUG_INFO, ("ipconfig_get_all: ipcponfig=\n%s\n", stdout_buf));

	line = stdout_buf;
	while(line && *line) {
	    line += strspn(line, "\r\n");
	    line_end = line + strcspn(line, "\r\n");
	    if( !*line_end ) {
		line_end = 0;
	    }
	    else {
		*line_end = 0;
	    }

	    i = ipconfig_parse_line(&ipconfig, line);
	    assertb(i>=0);

	    line = line_end ? line_end+1 : 0;
	}
    } while(0);
    return ipconfig.cfg_count;
}
