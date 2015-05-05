#include "bklib/test_certs.h"
#include "bklib/cryptbk.h"
#include "bklib/debug.h"
#include "bklib/debug.h"

int
main() {
    char raw[4096], enc[4096], dec[4096], buf[4096];
    int rawlen, enclen, declen;
    cryptbk_t *cert=0;
    int i, err=-1;
    int bits=512;

    do {
	debug_init(DEBUG_INFO, 0, 0);
	cryptbk_init();
	
	cert = cryptbk_selfsign("CN=test", 0, bits, 365*24*3600);
	assertb(cert);
	i = cryptbk_pack(cert, buf, sizeof(buf), "secret",
			 CRYPTBK_MODE_ENC 
			 | CRYPTBK_MODE_PRIVKEY
			 | CRYPTBK_MODE_PUBKEY
			 | CRYPTBK_MODE_TEXT);
	debug(DEBUG_INFO, 
	      ("cryptbk_pack(text): bits=%d len=%d cert=\n%s\n\n"
	       ,bits, i, buf));
	cryptbk_delete(cert);

	cert = cryptbk_new();
	assertb(cert);

	i = cryptbk_pack(cert, buf, 0, 0,
			 CRYPTBK_MODE_DEC 
			 | CRYPTBK_MODE_PRIVKEY
			 | CRYPTBK_MODE_PUBKEY
			 | CRYPTBK_MODE_TEXT);
	assertb(i<0);

	i = cryptbk_pack(cert, buf, 0, "noway",
			 CRYPTBK_MODE_DEC 
			 | CRYPTBK_MODE_PRIVKEY
			 | CRYPTBK_MODE_PUBKEY
			 | CRYPTBK_MODE_TEXT);
	assertb(i<0);

	i = cryptbk_pack(cert, buf, 0, "secret",
			 CRYPTBK_MODE_DEC 
			 | CRYPTBK_MODE_PRIVKEY
			 | CRYPTBK_MODE_PUBKEY
			 | CRYPTBK_MODE_TEXT);
	assertb(i>=0);
	
	rawlen = 32;
	
	enclen = cryptbk_crypt(cert, raw, rawlen, enc, sizeof(enc),
			       CRYPTBK_MODE_ENC 
			       | CRYPTBK_MODE_PRIVKEY 
			       | CRYPTBK_MODE_BINARY);
	debug(DEBUG_INFO, 
	      ("cryptbk_crypt(enc privkey): rawlen=%d enclen=%d enc=\n%s\n\n"
	       ,rawlen, enclen, enc));
	      
	declen = cryptbk_crypt(cert, enc, enclen, dec, sizeof(dec),
			       CRYPTBK_MODE_DEC 
			       | CRYPTBK_MODE_PUBKEY
			       | CRYPTBK_MODE_TEXT);
	
	debug(DEBUG_INFO, 
	      ("cryptbk_crypt(dec privkey): enclen=%d declen=%d\n"
	       ,enclen, declen));
	
	err = 0;
    } while(0);
    return err;
}
