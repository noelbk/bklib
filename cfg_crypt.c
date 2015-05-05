#include <stdio.h>
#include <string.h>

#include "cfg_crypt.h"
#include "debug.h"
#include "configbk.h"

/* load or generate a certificate. */
cryptbk_t*
cfg_load_crypt(cfg_t *cfg, char *cfg_name
	       ,char *hostname, int hostnamelen
	       ,int bits, int expire) {
    int i, err=-1;
    cryptbk_info_t info;
    cryptbk_t *crypt = 0;
    char buf[4096], *p;
    int crypt_changed=0;

    info.subject = 0;
    do {
	/* try to load certificate */
	i = cfg_get(cfg, cfg_name, buf, sizeof(buf));
	if( i <= 0 ) {
	    break;
	}
	crypt = cryptbk_new();
	assertb(crypt);
	i = cryptbk_pack(crypt, buf, i, 0,
			 CRYPTBK_MODE_DEC 
			 | CRYPTBK_MODE_TEXT 
			 | CRYPTBK_MODE_PUBKEY
			 | CRYPTBK_MODE_PRIVKEY);
	assertb(i>=0);
	i = cryptbk_info(crypt, &info, CRYPTBK_MODE_PUBKEY);
	assertb(i>=0);

	/* make sure the subject is what I expect, or copy the
	   hostname from the cert */
	p = info.subject;
	if( *p == '/' ) p++;
	assertb(strncmp(p, "CN=", 3)==0);
	p += 3;
	if( *hostname ) {
	    assertb(strcasecmp(p, hostname)==0);
	}
	else {
	    strncpy(hostname, p, hostnamelen);
	}

	/* renew the cert if necessary */
	if( info.notafter < mstime() ) {
	    i = cryptbk_req_sign(crypt, crypt, 
				 mstime(), mstime()+expire, info.serial++);
	    assertb(i>=0);
	    crypt_changed = 1;
	}

	err = 0;
    } while(0);
    
    if( info.subject ) {
	cryptbk_info_free(&info);
    }
    if( err ) {
	if( crypt ) {
	    cryptbk_delete(crypt);
	    crypt = 0;
	}
    }

    /* make a new cert if necessary */
    do {
	err = -1;
	if( !crypt ) {
	    if( !hostname || !*hostname ) {
		err = 0;
		break;
	    }
	    snprintf(buf, sizeof(buf), "CN=%s", hostname);
	    crypt = cryptbk_selfsign(buf, 0, bits, expire);
	    assertb(crypt);
	    crypt_changed = 1;
	}

	/* save the cert if changed */
	if( crypt_changed ) {
	    i = cryptbk_pack(crypt, buf, sizeof(buf), 0,
			     CRYPTBK_MODE_ENC
			     | CRYPTBK_MODE_TEXT 
			     | CRYPTBK_MODE_PUBKEY
			     | CRYPTBK_MODE_PRIVKEY);
	    assertb(i>=0);
	    i = cfg_put(cfg, cfg_name, buf, i, CFG_PUT_GLOBAL);
	    assertb(i>=0);
	} 
	err = 0;
    } while(0);

    if( err ) {
	if( crypt ) {
	    cryptbk_delete(crypt);
	    crypt = 0;
	}
    }
    return crypt;
}
