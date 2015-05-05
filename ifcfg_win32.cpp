#define _WIN32_DCOM // to correct ntddk/inc/objbase.h
#define _WIN32_WINNT 0x0400

#include "bklib/bkwin32.h"

#include <objbase.h>
#include <oleauto.h>

#include <wbemcli.h>
#include <wbemidl.h>

#define _CTYPE_DISABLE_MACROS // to correct ntddk/inc/wchar.h
#include <wchar.h>
#include <tchar.h>

#include <stdio.h>

#include "config.h"
#include "ifcfg.h"
#include "variant.h"
#include "warn.h"

struct ifcfg_win32_s {
    IWbemServices *pIWbemServices;
};
typedef struct ifcfg_win32_s ifcfg_win32_t;

// handy defs to reuse bstrs
#define BSTR_FREE(bs)  if( bs ) { SysFreeString(bs); bs = 0; }
#define BSTR_REALLOC(bs, ws)			\
    BSTR_FREE(bs)                               \
    if( ws ) {					\
        bs = SysAllocString(ws);		\
        assertb_syserr(bs);			\
    }						\

int
ifcfg_init() {
    HRESULT hr;
    int err=-1;
    do {
	hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
	// ignore errors if COM already initialized

	err = 0;
    } while(0);
    return err;
}

ifcfg_t*
ifcfg_new() {
    ifcfg_win32_t *ic=0;
    BSTR bs=0;
    HRESULT hr;
    IWbemLocator *pIWbemLocator=0;

    int err=-1;

    do {
	ic = (ifcfg_win32_t*)calloc(sizeof(*ic), 1);
	assertb_syserr(ic);

	// Get a pIWbemServices on localhost
	hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, 
			      IID_IWbemLocator, (LPVOID *)&pIWbemLocator);
	assertb_hresult(hr);
	BSTR_REALLOC(bs, L"\\\\.\\root\\cimv2");
	hr = pIWbemLocator->ConnectServer(bs, 0, 0, 0, 0, 0, 0, 
					  &ic->pIWbemServices);
	assertb_hresult(hr);
	
	// Whatever.  !$@# Windows.
	hr = CoSetProxyBlanket(ic->pIWbemServices,
			       RPC_C_AUTHN_WINNT,
			       RPC_C_AUTHZ_NONE,
			       0,
			       RPC_C_AUTHN_LEVEL_CALL,
			       RPC_C_IMP_LEVEL_IMPERSONATE,
			       0,
			       EOAC_NONE
			       );
	assertb_hresult(hr);

	err = 0;
    } while(0);
    BSTR_FREE(bs);
    if( pIWbemLocator ) pIWbemLocator->Release();
    if( err ) {
	ifcfg_delete(ic);
	ic = 0;
    }
    return (ifcfg_t*)ic;
}

void
ifcfg_delete(ifcfg_t *pni) {
    ifcfg_win32_t *ic = (ifcfg_win32_t*)pni;
    if( ic ) {
	if( ic->pIWbemServices ) {
	    ic->pIWbemServices->Release();
	    ic->pIWbemServices = 0;
	}
	free(ic);
    }
}

#define DEBUG_FUNC "ifcfg_enum"
int
ifcfg_enum(ifcfg_t *pni, char *if_name, int *if_idx, int nidx) {
    ifcfg_win32_t *ic = (ifcfg_win32_t*)pni;

    BSTR bs=0, bs2=0;
    HRESULT hr;
    WCHAR wbuf[4096];
    VARIANT v;

    IEnumWbemClassObject *pEnumAdapters = NULL;
    IWbemClassObject **pAdapter;
    ULONG i, nAdapter=0;

    int err=-1;
    
    do {
	pAdapter = (IWbemClassObject**)calloc(nidx, sizeof(*pAdapter));
	assertb_syserr(pAdapter);

	// find the first interface that matches hwid
	BSTR_REALLOC(bs, L"WQL");
	if( if_name ) {
	    _snwprintf(wbuf, sizeof(wbuf), 
		       L"select * from Win32_NetworkAdapterConfiguration where ServiceName = \"%S\"", if_name);
	}
	else {
	    _snwprintf(wbuf, sizeof(wbuf), 
		       L"select * from Win32_NetworkAdapterConfiguration");
	}
	BSTR_REALLOC(bs2, wbuf);
	hr = ic->pIWbemServices->ExecQuery(bs, bs2, 0, 0, &pEnumAdapters);
	assertb_hresult(hr);
	
	// retrive a whole array
	hr = pEnumAdapters->Next(5000, nidx, pAdapter, &nAdapter);
	assertb_hresult(hr);

	// get the index of each adapter
	for(i=0; i<nAdapter; i++) {
	    BSTR_REALLOC(bs, L"Index");
	    hr = pAdapter[i]->Get(bs, 0, &v, 0, 0);
	    assertb_hresult(hr);
	    if_idx[i] = v.intVal;
	    
	    debug_if(DEBUG_INFO+1) {
		BSTR_FREE(bs);
		hr = pAdapter[i]->GetObjectText(0, &bs);
		assertb_hresult(hr);
		debug(DEBUG_INFO+1,
		      (DEBUG_FUNC ": Adapter[%d] idx=%d: %ws\n"
		       , i, if_idx[i], bs));
	    }
	}
	assertb(i == nAdapter);

	err = 0;
    } while(0);
    BSTR_FREE(bs);
    BSTR_FREE(bs2);
    if( pAdapter ) {
	for(i=0; i<nAdapter; i++) {
	    if( pAdapter[i] ) { pAdapter[i]->Release(); }
	}
	free(pAdapter);
    }
    if( pEnumAdapters ) {
	pEnumAdapters->Release();
    }
    return err ? err : nAdapter;
}
#undef DEBUG_FUNC


// get an instance of a method for wbem_param and wbem_exec
IWbemClassObject*
wbem_method(IWbemClassObject *obj_parent, WCHAR *name) {
    IWbemClassObject *obj_class=0, *obj=0;
    BSTR bs=0;
    int err=-1;
    HRESULT hr;

    do {
	// get the class object first
	BSTR_REALLOC(bs, name);
	hr = obj_parent->GetMethod(bs, 0, &obj_class, 0);
	assertb_hresult(hr);

	// Create an instance of the method from the class
	hr = obj_class->SpawnInstance(0, &obj);
	assertb_hresult(hr);
	err = 0;
    } while(0);
    BSTR_FREE(bs);
    if( obj_class ) {
	obj_class->Release();
    }
    if( err ) {
	if( obj ) {
	    obj->Release();
	    obj = 0;
	}
    }
    return obj;
}

// convert addr to a BSTR ARRAY.  null or 0 addr is an empty array.
// returns 0 iff ok.
int
wbem_param_inaddr(IWbemClassObject *obj, WCHAR *name, struct in_addr *addr, int naddr) {
    BSTR bs=0, *bs_addr=0;
    int k, err=-1;
    HRESULT hr;
    WCHAR wbuf[4096];
    VARIANT v;
    int i;

    do {
	BSTR_REALLOC(bs, name);
	if( !addr || !addr->s_addr || naddr <= 0 ) {
	    i = variant_array_bstrs(&v, 0, 0);
	}
	else {
	    bs_addr = (BSTR*)calloc(naddr, sizeof(BSTR));
	    assertb(bs_addr);
	    for(k=0; k<naddr; k++) {
		_snwprintf(wbuf, sizeof(wbuf), L"%S", inet_ntoa(addr[k]));
		BSTR_REALLOC(bs_addr[k], wbuf);
	    }
	    i = variant_array_bstrs(&v, bs_addr, naddr);
	    assertb(!i);
	}
	hr = obj->Put(bs, 0, &v, 0);    
	if( v.parray ) {
	    SafeArrayDestroy(v.parray);
	    v.parray = 0;
	}
	assertb_hresult(hr);
	err = 0;
    } while(0);
    BSTR_FREE(bs);
    if( bs_addr ) {
	for(k=0; k<naddr; k++) {
	    BSTR_FREE(bs_addr[k]);
	}
	free(bs_addr);
    }
    return err;
}

// convert addr to a BSTR ARRAY.  null or 0 addr is an empty array.
// returns 0 iff ok.
int
wbem_param_str(IWbemClassObject *obj, WCHAR *name, char *val) {
    BSTR bs=0, bs2=0;
    int err=-1;
    HRESULT hr;
    WCHAR wbuf[4096];
    VARIANT v;

    do {
	BSTR_REALLOC(bs, name);
	_snwprintf(wbuf, sizeof(wbuf), L"%S", val);
	BSTR_REALLOC(bs2, wbuf);
	v.vt = VT_BSTR;
	v.bstrVal = bs2;
	hr = obj->Put(bs, 0, &v, 0);    
	assertb_hresult(hr);
	err = 0;
    } while(0);
    BSTR_FREE(bs);
    BSTR_FREE(bs2);
    return err;
}

// convert addr to a BSTR ARRAY.  null or 0 addr is an empty array.
// returns 0 iff ok.
int
wbem_param_u32(IWbemClassObject *obj, WCHAR *name, unsigned long val) {
    BSTR bs=0;
    int err=-1;
    HRESULT hr;
    VARIANT v;

    do {
	BSTR_REALLOC(bs, name);
	v.vt = VT_I4;
	v.lVal = val;
	hr = obj->Put(bs, 0, &v, 0);    
	assertb_hresult(hr);
	err = 0;
    } while(0); 
    BSTR_FREE(bs);
    return err;
}

// exec a method from wbem_method after wbem_param has been
// set. method_out gets a result object if not NULL.  presult gets the
// result code if not null.  returns 0 iff ok.
#define DEBUG_FUNC "wbem_exec"
int
wbem_exec(IWbemServices *pIWbemServices, 
	  BSTR  objpath,
	  WCHAR *method, 
	  IWbemClassObject *method_in,
	  IWbemClassObject **pmethod_out,
	  int *presult
	  ) {
    HRESULT hr;
    BSTR bs=0;
    VARIANT v;
    IWbemClassObject *method_out=0;
    int err=-1;

    do {
	BSTR_REALLOC(bs, method);
	debug_if(DEBUG_INFO) {
	    BSTR bs3=0;
	    hr = method_in->GetObjectText(0, &bs3);
	    assertb_hresult(hr);
	    debug(DEBUG_INFO+1,
		  (DEBUG_FUNC ": ExecMethod(objpath=%ws, method=%ws) method_in=%ws\n", 
		   objpath, bs, bs3));
	    BSTR_FREE(bs3);
	}
	hr = pIWbemServices->ExecMethod(objpath, bs, 0, 0, 
					method_in, &method_out, 0);
	assertb_hresult(hr);
	
	debug_if(DEBUG_INFO) {
	    BSTR bs3=0;
	    hr = method_out->GetObjectText(0, &bs3);
	    assertb_hresult(hr);
	    debug(DEBUG_INFO+1,
		  (DEBUG_FUNC ": ExecMethod(objpath=%ws, method=%ws) method_out=%ws\n", 
		   objpath, bs, bs3));
	    BSTR_FREE(bs3);
	}

	// get the return value
	if( presult ) {
	    BSTR_REALLOC(bs, L"ReturnValue");
	    hr = method_out->Get(bs, 0, &v, 0, 0);
	    *presult = v.intVal;
	    assertb_hresult(hr);
	}

	err = 0;
    } while(0);
    BSTR_FREE(bs);
    if( err ) {
	if( method_out ) {
	    method_out->Release();
	    method_out = 0;
 	}
    }

    if( pmethod_out ) {
	*pmethod_out = method_out;
    }
    else if ( method_out ) {
	method_out->Release();
    }
    
    return err;
}
#undef DEBUG_FUNC

/* returns <0 on error, 1 iff reboot required */
#define DEBUG_FUNC "ifcfg_config"
int 
ifcfg_set(ifcfg_t *pni, int if_idx, ifcfg_config_t *cfg) {
    ifcfg_win32_t *ic = (ifcfg_win32_t*)pni;

    BSTR bs=0, bs2=0, objpath=0;
    HRESULT hr;
    WCHAR wbuf[4096];
    int reboot=0;
    int result;
    IWbemClassObject *pAdapterConfiguration = NULL;
    IWbemClassObject *method = NULL;                            
    
    int i;
    int err = -1;

    do {
	// get a NetworkAdapterConfiguration object
	BSTR_REALLOC(bs, L"Win32_NetworkAdapterConfiguration");
	hr = ic->pIWbemServices->GetObject(bs, 0, 0, &pAdapterConfiguration, 0);
	assertb_hresult(hr);
	
	// the absolute path for invoking methods
	_snwprintf(wbuf, sizeof(wbuf), L"Win32_NetworkAdapterConfiguration.Index=%d", if_idx);
	BSTR_REALLOC(objpath, wbuf);

	if( cfg->fields & IFCFG_CONFIG_IPADDR ) {
	    // must set both ipaddr and netmask
	    assertb(cfg->fields & IFCFG_CONFIG_NETMASK);
	    method = wbem_method(pAdapterConfiguration, L"EnableStatic");
	    assertb(method);
	    i = wbem_param_inaddr(method, L"IPAddress", &cfg->ipaddr, 1);
	    assertb(!i);
	    i = wbem_param_inaddr(method, L"SubnetMask", &cfg->netmask, 1);
	    assertb(!i);
	    i = wbem_exec(ic->pIWbemServices, objpath, L"EnableStatic", 
			  method, 0, &result);
	    assertb(!i);
	    reboot = reboot || (result == 1);
	    method->Release();
	    method = 0;
	}

	// SetGateways 
	if( cfg->fields & IFCFG_CONFIG_GATEWAY ) {
	    method = wbem_method(pAdapterConfiguration, L"SetGateways");
	    assertb(method);
	    i = wbem_param_inaddr(method, L"DefaultIPGateway", &cfg->gateway, 1);
	    assertb(!i);
	    i = wbem_exec(ic->pIWbemServices, objpath, L"SetGateways", 
			  method, 0, &result);
	    assertb(!i);
	    reboot = reboot || (result == 1);
	    method->Release();
	    method = 0;
	}
	
	if( cfg->fields & IFCFG_CONFIG_DNS ) {
	    method = wbem_method(pAdapterConfiguration, L"SetDNSServerSearchOrder");
	    assertb(method);
	    i = wbem_param_inaddr(method, L"DNSServerSearchOrder", 
				  cfg->dns_search, cfg->dns_search_count);
	    assertb(!i);
	    i = wbem_exec(ic->pIWbemServices, objpath, L"SetDNSServerSearchOrder",
			  method, 0, &result);
	    assertb(!i);
	    reboot = reboot || (result == 1);
	    method->Release();
	    method = 0;
	}

	if( cfg->fields & IFCFG_CONFIG_DNS_DOMAIN ) {
	    method = wbem_method(pAdapterConfiguration, L"SetDNSDomain");
	    assertb(method);
	    i = wbem_param_str(method, L"DNSDomain", cfg->dns_domain);
	    assertb(!i);
	    i = wbem_exec(ic->pIWbemServices, objpath, L"SetDNSDomain",
			  method, 0, &result);
	    assertb(!i);
	    reboot = reboot || (result == 1);
	    method->Release();
	    method = 0;
	}
	
	if( cfg->fields & IFCFG_CONFIG_DHCP ) {
	    if( cfg->dhcp ) {
		method = wbem_method(pAdapterConfiguration, L"EnableDHCP");
		assertb(method);
		i = wbem_exec(ic->pIWbemServices, objpath, L"EnableDHCP",
			      method, 0, &result);
		assertb(!i);
		reboot = reboot || (result == 1);
		method->Release();
		method = 0;
	    }
	}

	/* todo - figure how to set MTU */
	if( 0 && cfg->fields & IFCFG_CONFIG_MTU ) {
	    method = wbem_method(pAdapterConfiguration, L"SetMTU");
	    assertb(method);
	    i = wbem_param_u32(method, L"MTU", cfg->mtu);
	    assertb(!i);
	    i = wbem_exec(ic->pIWbemServices, objpath, L"SetMTU",
			  method, 0, &result);
	    assertb(!i);
	    reboot = reboot || (result == 1);
	    method->Release();
	    method = 0;
	}

	err = 0;
    } while(0);

    BSTR_FREE(bs);
    BSTR_FREE(bs2);

    if( method ) method->Release();
    if( pAdapterConfiguration ) pAdapterConfiguration->Release();

    return err ? err : reboot;
}
#undef DEBUG_FUNC

/* returns <0 on error, 1 iff reboot required */
extern "C"
#define DEBUG_FUNC "ifcfg_get_prop"
int 
ifcfg_get_prop_list(ifcfg_t *pni, int if_idx, 
		    ifcfg_prop_list_t *props,
		    char *buf, int len) {
    ifcfg_win32_t *ic = (ifcfg_win32_t*)pni;

    BSTR bs=0, bs2=0, objpath=0;
    HRESULT hr;
    WCHAR wbuf[4096];
    IEnumWbemClassObject *pQuery = NULL;
    IWbemClassObject *pQueryResult = NULL;
    ULONG nQueryResult;
    VARIANT v;
    ifcfg_prop_list_t *pr;
    char *p, *q;
    int i, n, err=-1;

    do {
	// concatenate list of fields to fetch
	for(p=buf, n=len, pr=props; pr->name; pr++) {
	    err = -1;
	    if( pr > props ) {
		i = snprintf(p, n, ",");
		assertb(i>0);
		p += i;
		n -= i;
	    }
	    i = snprintf(p, n, "%s", pr->name);
	    assertb(i>0);
	    p += i;
	    n -= i;
	    err = 0;
	}
	assertb(!err);
	err = -1;

	// use WQL to get the property
	BSTR_REALLOC(bs, L"WQL");
	_snwprintf(wbuf, sizeof(wbuf), 
		   L"select %S from Win32_NetworkAdapterConfiguration"
		   L" where Index = %d", buf, if_idx);
	BSTR_REALLOC(bs2, wbuf);
	hr = ic->pIWbemServices->ExecQuery(bs, bs2, 0, 0, &pQuery);
	assertb_hresult(hr);
	
	// execute the query
	hr = pQuery->Next(5000, 1, &pQueryResult, &nQueryResult);
	assertb_hresult(hr);
	assertb(nQueryResult==1);

	for(p=buf, n=len, pr=props; pr->name; pr++) {
	    err = -1;
	    // extract the property from the query
	    _snwprintf(wbuf, sizeof(wbuf), L"%S", pr->name);
	    BSTR_REALLOC(bs, wbuf);
	    hr = pQueryResult->Get(bs, 0, &v, 0, 0);
	    assertb_hresult(hr);
	
	    pr->val = p;
	    q = variant_fmt(&v, p, n);
	    assertb(q);
	    VariantClear(&v);

	    i = strlen(p)+1;
	    p += i;
	    n -= i;
	    assertb(n>0);
	    err = 0;
	}
	assertb(!err);
	err = -1;

	err = 0;
    } while(0);

    BSTR_FREE(bs);
    BSTR_FREE(bs2);

    if( pQuery ) pQuery->Release();
    if( pQueryResult ) pQueryResult->Release();

    return err;
}
#undef DEBUG_FUNC

int 
ifcfg_get_prop(ifcfg_t *pni, int if_idx, 
	       char *prop_name, 
	       char *prop_val, int prop_val_len) {
    ifcfg_prop_list_t prop[] = { 
	{prop_name, 0}, 
	{0,0} 
    };
    return ifcfg_get_prop_list(pni, if_idx, prop, prop_val, prop_val_len);
}

struct in_addr
ifcfg_inaddr(char *buf) {
    struct in_addr a;
    buf[strspn(buf, "0123456789.")] = 0;
    a.s_addr = inet_addr(buf);
    return a;
}

u32_t
ifcfg_inaddr_list(char **ptr) {
    u32_t a=0;
    char *p = *ptr;
    int i;
    char buf[1024];

    do {
	/* copy inet addr */
	i = strspn(p, "0123456789.");
	assertb(i<sizeof(buf)-1 );
	memcpy(buf, p, i);
	buf[i] = 0;
	a = inet_addr(buf);
    
	/* skip ", " */
	p += i;
	p += strspn(p, ", ");
	*ptr = p;
    } while(0);

    return a;
}

int 
ifcfg_get(ifcfg_t *pni, int if_idx, ifcfg_config_t *cfg) {
    int i, err=-1;
    char buf[4096], *p;
    unsigned int pi[6];
    ifcfg_prop_list_t props[] = {
	{"IPEnabled", 0},
	{"ServiceName", 0},
	{"MACAddress", 0},
	{"IPAddress", 0},
	{"IPSubnet", 0},
	{"DefaultIPGateway", 0},
	{"DNSServerSearchOrder", 0},
	{"MTU", 0},
	{"DHCPEnabled", 0},
	{"Description", 0},
	{"Caption", 0},
	{0, 0},
    };

    do {
	memset(cfg, 0, sizeof(*cfg));

	i = ifcfg_get_prop_list(pni, if_idx, props, buf, sizeof(buf));
	assertb(!i);
	
	/* ip enabled */
	i = strtoul(props[0].val, &p, 0);
	if( !i ) {
	    err = 0;
	    break;
	}
	
	snprintf(cfg->name, sizeof(cfg->name),
		 "%s%d", props[1].val, if_idx);
	cfg->fields |= IFCFG_CONFIG_NAME;

	i = sscanf(props[2].val, "%x:%x:%x:%x:%x:%x", 
		   &pi[0],&pi[1],&pi[2],&pi[3],&pi[4],&pi[5]);
	assertb(i==6);
	cfg->fields |= IFCFG_CONFIG_ETHADDR;
	cfg->ethaddr[0] = pi[0];
	cfg->ethaddr[1] = pi[1];
	cfg->ethaddr[2] = pi[2];
	cfg->ethaddr[3] = pi[3];
	cfg->ethaddr[4] = pi[4];
	cfg->ethaddr[5] = pi[5];

	cfg->ipaddr = ifcfg_inaddr(props[3].val);
	cfg->fields |= IFCFG_CONFIG_IPADDR;
	
	cfg->netmask = ifcfg_inaddr(props[4].val);
	cfg->fields |= IFCFG_CONFIG_NETMASK;

	cfg->gateway = ifcfg_inaddr(props[5].val);
	cfg->fields |= IFCFG_CONFIG_GATEWAY;

	p = props[6].val;
	for(i=0; i<IFCFG_DNS_SEARCH_MAX; i++) {
	    cfg->dns_search[i].s_addr = ifcfg_inaddr_list(&p);
	    if( !cfg->dns_search[i].s_addr ) break;
	}
	cfg->dns_search_count = i;
	cfg->fields |= IFCFG_CONFIG_DNS;

	cfg->mtu = strtoul(props[7].val, &p, 0);
	cfg->fields |= IFCFG_CONFIG_MTU;

	cfg->dhcp = strtoul(props[8].val, &p, 0);
	cfg->fields |= IFCFG_CONFIG_DHCP;

	snprintf(cfg->description, sizeof(cfg->description),
		 "%s", props[9].val);
	cfg->fields |= IFCFG_CONFIG_DESCRIPTION;

	snprintf(cfg->caption, sizeof(cfg->caption),
		 "%s", props[10].val);
	cfg->fields |= IFCFG_CONFIG_CAPTION;

	err = 0;
    } while(0);
    return err;
}

