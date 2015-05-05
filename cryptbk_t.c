#include <stdio.h>
#include <time.h>
#include <string.h>

#include "cryptbk.h"
#include "debug.h"
#include "memutil.h"
#include "readable.h"

int
cb(void *arg, int prog, char *msg) {
    printf("%s: %s\n", (char*)arg, msg);
    return 0;
}

void
print_cert(cryptbk_t *cert, int mode, char *msg) {
    int i; // , err=-1;
    cryptbk_info_t info;
    char buf[4096];
    struct tm *tm;
    time_t t;

    do {
        i = cryptbk_info(cert, &info, mode);
	assertb(i>=0);
	printf("certificate %s\n", msg);
	printf("bits:      %d\n", info.bits);
	printf("subject:   %s\n", info.subject);
	printf("issuer:    %s\n", info.issuer);
	
	t = (time_t)info.notbefore;
	tm = localtime(&t);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", tm);
	printf("notbefore: %s\n", buf);

	t = (time_t)info.notafter;
	tm = localtime(&t);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", tm);
	printf("notafter:  %s\n", buf);	

	for(i=0; i<cryptbk_exts_count(info.exts); i++) {
	    cryptbk_ext_info_t einfo;
	    cryptbk_exts_info(info.exts, i, &einfo);
	    printf("%s: %s\n", einfo.longname, einfo.value);
	    cryptbk_ext_info_free(&einfo);
	}
	printf("\n");
	cryptbk_info_free(&info);
	//err = 0;
    } while(0);
}

int
enc_test(cryptbk_t *enc_cert, cryptbk_t *dec_cert, int mode,
	 char *raw, int rawlen, double repeat_sec, char *msg) {
    char enc[4096*2], dec[4096*2], buf[4096];
    int enclen, declen;
    int i, err=-1;
    mstime_t t;
    double bytes, dt=0;

    do {
	t = mstime();
	bytes = 0;
	do {
	    enclen = cryptbk_crypt(enc_cert, raw, rawlen, enc, sizeof(enc),
				  CRYPTBK_MODE_ENC | mode);
	    assertb(enclen>0);
	    bytes += rawlen;
	    dt = mstime() - t;
	} while(dt < repeat_sec);
	assertb(enclen>0);
	printf("encrypt %s: %s/sec\n", msg, 
	       readable_bytes(bytes/dt, buf, sizeof(buf)));
	
	i = memdiff(raw, rawlen, enc, enclen);
	assertb(i>=0);

	// swap pubkey/privkey for decryption
	if( mode & CRYPTBK_MODE_PUBKEY ) {
	    mode &= ~CRYPTBK_MODE_PUBKEY;
	    mode |= CRYPTBK_MODE_PRIVKEY;
	}
	else if( mode & CRYPTBK_MODE_PRIVKEY ) {
	    mode &= ~CRYPTBK_MODE_PRIVKEY;
	    mode |= CRYPTBK_MODE_PUBKEY;
	}
	
	t = mstime();
	bytes = 0;
	do {
	    declen = cryptbk_crypt(dec_cert, enc, enclen, dec, sizeof(dec),
				   CRYPTBK_MODE_DEC | mode);
	    assertb(declen>0);
	    bytes += declen;
	    dt = mstime() - t;
	} while(dt < repeat_sec);
	assertb(declen>0);
	printf("decrypt %s: %s/sec\n", msg, 
	       readable_bytes(bytes/dt, buf, sizeof(buf)));

	i = memdiff(raw, rawlen, dec, declen);
	assertb(i<0);

	err =0;
    } while(0);
    return err;
}

int
main() {
    cryptbk_t *server_cert=0, *client_cert=0, *new_cert=0;
    cryptbk_exts_t *exts;
    cryptbk_ext_id_t id;
    int i;
    char buf[4096];
    int buflen;
    char raw[16];
    int rawlen;
    
    char enc1_buf[4096], enc2_buf[4096];
    int enc1_len, enc2_len;

    do {
	debug_init(DEBUG_INFO, 0, 0);
	cryptbk_init();

	// my custom extension for ethernet addresses
	id = cryptbk_ext_id_new("1.2.3.4", "EtherAddr", 
			       "P2PVPN Ethernet Address");

	// make a self-signed server cert
	server_cert = cryptbk_new();
	exts = cryptbk_exts_new();
	cryptbk_exts_add(exts, cryptbk_ext_id_subject_alt_name, 
			"email:noel@bkbox.com");
	cryptbk_exts_add(exts, id, "ff:ee:dd:bb:ee:aa");
	cryptbk_req_gen(server_cert, 2048, "CN=server", exts, cb, "server");
	cryptbk_exts_delete(exts);
	cryptbk_req_sign(server_cert, server_cert, mstime(), mstime()+365*3600*24, 1);
	print_cert(server_cert, CRYPTBK_MODE_REQ, "server req");
	print_cert(server_cert, CRYPTBK_MODE_PUBKEY, "server");
	
	
	// encrypt a random message twice to see if encrypting the
	// same message always results in the same ciphertext
	buflen = 128;
	cryptbk_rand_buf(buf, buflen);

	// test md5
	enc1_len = cryptbk_crypt(server_cert, buf, buflen,
				 enc1_buf, sizeof(enc1_buf), 
				 CRYPTBK_MODE_ENC 
				 | CRYPTBK_MODE_TEXT
				 | CRYPTBK_MODE_MD5);
	printf("md5: %s\n", enc1_buf);
	i = cryptbk_crypt(server_cert, buf, buflen,
			  enc1_buf, sizeof(enc1_buf), 
			  CRYPTBK_MODE_DEC
			  | CRYPTBK_MODE_TEXT
			  | CRYPTBK_MODE_MD5);
	printf("md5 check: i=%d\n", i);



	enc1_len = cryptbk_crypt(server_cert, buf, buflen,
				 enc1_buf, sizeof(enc1_buf), 
				 CRYPTBK_MODE_ENC | CRYPTBK_MODE_PUBKEY);
	enc2_len = cryptbk_crypt(server_cert, buf, buflen,
				 enc2_buf, sizeof(enc2_buf), 
				 CRYPTBK_MODE_ENC | CRYPTBK_MODE_PUBKEY);
	i = memdiff(enc1_buf, enc1_len, enc2_buf, enc2_len);
	printf("diff in encryption: i=%d\n", i);
	memdump(buf, sizeof(buf), enc1_buf, enc1_len);
	printf("enc1:\n%s\n", buf);
	memdump(buf, sizeof(buf), enc2_buf, enc2_len);
	printf("enc2:\n%s\n", buf);
	
	buflen = 128;
	cryptbk_rand_buf(buf, buflen);
	cryptbk_seskey_rand(server_cert, 512);
	enc1_len = cryptbk_crypt(server_cert, buf, buflen,
				 enc1_buf, sizeof(enc1_buf), 
				 CRYPTBK_MODE_ENC | CRYPTBK_MODE_SESKEY);
	enc2_len = cryptbk_crypt(server_cert, buf, buflen,
				 enc2_buf, sizeof(enc2_buf), 
				 CRYPTBK_MODE_ENC | CRYPTBK_MODE_SESKEY);
	i = memdiff(enc1_buf, enc1_len, enc2_buf, enc2_len);
	printf("diff in encryption: i=%d\n", i);
	memdump(buf, sizeof(buf), enc1_buf, enc1_len);
	printf("enc1:\n%s\n", buf);
	memdump(buf, sizeof(buf), enc2_buf, enc2_len);
	printf("enc2:\n%s\n", buf);
	
	


	// make a client cert signed by server
	client_cert = cryptbk_new();
	exts = cryptbk_exts_new();
	cryptbk_exts_add(exts, cryptbk_ext_id_subject_alt_name, 
			"email:client@bkbox.com");
	cryptbk_exts_add(exts, id, "ff:ee:dd:bb:ee:aa");
	cryptbk_req_gen(client_cert, 2048, "CN=client", exts, cb, "client");
	cryptbk_exts_delete(exts);
	cryptbk_req_sign(client_cert, server_cert, mstime(), mstime()+365*3600*24, 1);
	print_cert(client_cert, CRYPTBK_MODE_PUBKEY, "client");

	// get another signed cert from the server from the first
	// client's signed cert (not a request)
	new_cert = cryptbk_new();
	assertb(new_cert);
	i = cryptbk_copy(new_cert, client_cert, CRYPTBK_MODE_PUBKEY);
	assertb(i>=0);
	i = cryptbk_req_sign(new_cert, server_cert, mstime(), mstime()+365*3600*24, 2);
	assertb(i>=0);
	i = cryptbk_check_pubkey(client_cert, new_cert);
	assertb(i>=0);
	i = cryptbk_copy(client_cert, new_cert, CRYPTBK_MODE_PUBKEY);
	assertb(i==0);
	print_cert(new_cert, CRYPTBK_MODE_PUBKEY, "new cert");

	// save and restore the client cert
	buflen = cryptbk_pack(client_cert, buf, sizeof(buf), 0, CRYPTBK_MODE_ENC
			     //| CRYPTBK_MODE_TEXT 
			     | CRYPTBK_MODE_BINARY
			     | CRYPTBK_MODE_PUBKEY 
			     | CRYPTBK_MODE_PRIVKEY);
	assertb(buflen>0 && buflen<sizeof(buf));
	printf("client cert packed len=%d\n", buflen);
	printf("%s\n", buf);
	cryptbk_delete(client_cert);
	client_cert = cryptbk_new();
	i = cryptbk_pack(client_cert, buf, buflen, 0, CRYPTBK_MODE_DEC 
			 //| CRYPTBK_MODE_TEXT 
			 | CRYPTBK_MODE_BINARY
			 | CRYPTBK_MODE_PUBKEY 
			 | CRYPTBK_MODE_PRIVKEY);
	assertb(i>=0);
	print_cert(client_cert, CRYPTBK_MODE_PUBKEY, "client unpacked");

	// verify should fail
	i = cryptbk_verify(client_cert, client_cert);
	assertb(i<0);

	// verify the client cert using the server cert
	i = cryptbk_trust(client_cert, server_cert);
	assertb(i>=0);
	i = cryptbk_verify(client_cert, client_cert);
	assertb(i>=0);

	// crypt tests: pubkey, privkey, seskey
	rawlen = cryptbk_rand_buf(raw, sizeof(raw));
	assertb(rawlen>0);

#define ENC_TEST_REPEAT 2 // number of seconds to repeat tests to get average bytes/sec

	enc_test(server_cert, server_cert, CRYPTBK_MODE_PUBKEY,
		 raw, rawlen, ENC_TEST_REPEAT, "pubkey");
	enc_test(client_cert, client_cert, CRYPTBK_MODE_PRIVKEY,
		 raw, rawlen, ENC_TEST_REPEAT, "privkey");

	for(i=0; i<3; i++) {
	    int keylen, keylens[3] = { 128, 192, 256 };
	    char msg[4096];

	    keylen = keylens[i];
	    sprintf(msg, "aes-%d", keylen);

	    buflen = cryptbk_rand_buf(buf, keylen/8);
	    cryptbk_seskey_set(client_cert, buf, buflen);
	    cryptbk_seskey_set(server_cert, buf, buflen);
	    
	    enc_test(client_cert, server_cert, CRYPTBK_MODE_SESKEY, 
		     raw, rawlen, ENC_TEST_REPEAT, msg);
	}

	buflen = cryptbk_rand_buf(buf, sizeof(buf));
	enc_test(client_cert, server_cert, CRYPTBK_MODE_BASE64, 
		 raw, rawlen, ENC_TEST_REPEAT, "base64");

    } while(0);
    cryptbk_fini();
    return 0;
}
