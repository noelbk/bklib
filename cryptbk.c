/* stupid wincrypt.h redfines X509_NAME, but this include order fixes it */

#include "config.h"
#include "cryptbk.h"
#include "debug.h"
#include "sock.h"
#include "defutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/blowfish.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#define assertb_ssl(cond)					\
    if( !(cond) ) {						\
	debug(DEBUG_ASSERT, ("assert(%s) failed\n", #cond));	\
	ERR_print_errors_fp(stderr);				\
        break;                                                  \
    }

const cryptbk_ext_id_t cryptbk_ext_id_subject_alt_name = NID_subject_alt_name;

struct cryptbk_s {
    X509_REQ   *req;
    X509       *cert;
    X509_STORE *store;
    EVP_PKEY   *pkey;
    EVP_CIPHER *cipher;
    char       *seskey;
    int         seskey_len;
};

struct cryptbk_exts_s {
    STACK_OF(X509_EXTENSION) *exts;
};

int
cryptbk_init() {
    mstime_t t;

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    
    t = mstime();
    RAND_seed(&t, sizeof(t));

    return 0;
}

int
cryptbk_fini() {
    return 0;
}


cryptbk_t*
cryptbk_new() {
    return (cryptbk_t*)calloc(1, sizeof(cryptbk_t));
}

cryptbk_t*
cryptbk_new_cert() {
    cryptbk_t *cert=0;
    int err=-1;
    do {
	cert = cryptbk_new();
	assertb(cert);
	cert->cert = X509_new();
	assertb(cert->cert);
	err = 0;
    } while(0);
    if( err ) {
	if( cert ) {
	    cryptbk_delete(cert);
	    cert=0;
	}
    }
    return cert;
}

void
cryptbk_delete(cryptbk_t *cert) {
    if( cert->req ) X509_REQ_free(cert->req);
    if( cert->cert ) X509_free(cert->cert);
    if( cert->store ) X509_STORE_free(cert->store);
    if( cert->pkey ) EVP_PKEY_free(cert->pkey);
    if( cert->seskey ) free(cert->seskey);
    memset(cert, 0, sizeof(*cert));
    free(cert);
}


cryptbk_exts_t*
cryptbk_exts_new() {
    cryptbk_exts_t *exts;
    exts = calloc(1, sizeof(*exts));
    exts->exts = sk_X509_EXTENSION_new_null();
    return exts;
}


void
cryptbk_exts_delete(cryptbk_exts_t* exts) {
    sk_X509_EXTENSION_pop_free(exts->exts, X509_EXTENSION_free);
    free(exts);
}


int
cryptbk_exts_add(cryptbk_exts_t *exts, cryptbk_ext_id_t nid, char *value) {
    int i, err=-1;
    X509_EXTENSION *ex=0;

    do {
	ex = X509V3_EXT_conf_nid(NULL, NULL, nid, value);
	assertb(ex);
	i = sk_X509_EXTENSION_push(exts->exts, ex);
	assertb(i);
	err = 0;
    } while(0);
    return err;
}


cryptbk_ext_id_t
cryptbk_ext_id_new(char *oid, char *shortname, char *longname) {
    cryptbk_ext_id_t nid;
    int i, err=-1;
    do {
	nid = OBJ_create(oid, shortname, longname);
	assertb(nid);
	i = X509V3_EXT_add_alias(nid, NID_netscape_comment);
	assertb(i);
	err = 0;
    } while(0);
    return err ? 0 : nid;
}


int
cryptbk_exts_count(cryptbk_exts_t *exts) {
    return sk_X509_EXTENSION_num(exts->exts);
}

int
cryptbk_exts_info(cryptbk_exts_t *exts, int idx, cryptbk_ext_info_t *info) {
    X509_EXTENSION *ext;
    ASN1_OBJECT *obj;
    char buf[4096];
    int i, err=-1;
    BIO *bio=0;
    BUF_MEM *bptr;

    do {
	memset(info, 0, sizeof(*info));
	
	// shortname?

	// longname
	ext = sk_X509_EXTENSION_value(exts->exts, idx);
	obj = X509_EXTENSION_get_object(ext);
	i2t_ASN1_OBJECT(buf, sizeof(buf), obj);
	info->longname = strdup(buf);

	// value
	bio = BIO_new(BIO_s_mem());
	i = X509V3_EXT_print(bio, ext, 0, 0);
	assertb(i);
	BIO_write(bio, "", 1);
	BIO_get_mem_ptr(bio, &bptr);
	info->value = strdup(bptr->data);

	err = 0;
    } while(0);
    if( bio ) BIO_free(bio);

    return err;
}

int
cryptbk_ext_info_free(cryptbk_ext_info_t *info) {
    if( info->shortname ) free(info->shortname);
    if( info->longname ) free(info->longname);
    if( info->value ) free(info->value);
    memset(info, 0, sizeof(*info));
    return 0;
}

struct cryptbk_cb_arg_s {
    cryptbk_callback_t func;
    void *arg;
    int i;
    char *msg;
};
typedef struct cryptbk_cb_arg_s cryptbk_cb_arg_t;

void
cryptbk_req_gen_cb(int i, int j, void *varg) {
    cryptbk_cb_arg_t *arg =(cryptbk_cb_arg_t *)varg;
    char buf[4096];
    char *spin = "/-\\|";
    char c;
    
    if( ! arg->func ) return;

    
    switch(i) {
    case 1: c = '+'; break;
    case 2: c = 'X'; break;
    case 3: c = '*'; break;
    default:
	c = spin[(arg->i++) % 4];
	break;
    }
    snprintf(buf, sizeof(buf), "%s (%c) i=%d j=%d", arg->msg, c, i, j);
    arg->func(arg->arg, 50, buf);
}

int
x509_name_unpack(X509_NAME *name, char *dn, int len) {
    dn = X509_NAME_oneline(name, dn, len);
    return dn ? 0 : -1;
}

int
x509_name_pack(X509_NAME *name, char *dn) {
    int i, err =-1;
    char *p, *q, *r;
    char keybuf[4096];
    do {
	/* split up dc=home,dc=home,dc=p2p,dn=bkbox,dn=com */
	p = dn;
	while(p && *p) {
	    q = strchr(p, ',');
	    if( q ) *q = 0;
	    
	    r = strchr(p, '=');
	    if( r ) {
		i = r-p;
		assertb(i<sizeof(keybuf)-1);
		strncpy(keybuf, p, i);
		keybuf[i] = 0;
		r++;
		i = X509_NAME_add_entry_by_txt(name, keybuf, MBSTRING_ASC, (unsigned char*)r, 
					       -1, -1, 0);
		assertb(i);
	    }

	    if( q ) {
		*q = ',';
		p = q+1;
	    }
	    else {
		p = 0;
	    }
	}
	assertb(!p || !*p);
	err = 0;
    } while(0);
    return err;
}

int
cryptbk_req_gen(cryptbk_t *cert, int bits, char *dn, cryptbk_exts_t *exts,
	       cryptbk_callback_t func, void *arg) {
    RSA *rsa=0;
    X509_NAME *name = NULL;
    int i, err=-1;
    cryptbk_cb_arg_t varg;

    do {
	memset(&varg, 0, sizeof(varg));
	varg.func = func;
	varg.arg = arg;
	varg.msg = "Generating certificate";
	
	if( cert->pkey ) EVP_PKEY_free(cert->pkey);
	cert->pkey = EVP_PKEY_new();
	assertb(cert->pkey);

	if( cert->req ) X509_REQ_free(cert->req);
	cert->req = X509_REQ_new();
	assertb(cert->req);
	rsa = RSA_generate_key(bits, RSA_F4, cryptbk_req_gen_cb, &varg);
	assertb(rsa);
	i = EVP_PKEY_assign_RSA(cert->pkey, rsa);
	assertb(i);
	rsa = NULL;
	i = X509_REQ_set_pubkey(cert->req, cert->pkey);
	assertb(i);
	
	name = X509_REQ_get_subject_name(cert->req);
	i = x509_name_pack(name, dn);
	assertb(!i);
	X509_REQ_set_subject_name(cert->req, name);
	
	if( exts ) {
	    i = X509_REQ_add_extensions(cert->req, exts->exts);
	    assertb(i);
	}

	i = X509_REQ_sign(cert->req, cert->pkey, EVP_md5());
	assertb(i);
	
	err = 0;
    } while(0);
    if( rsa ) RSA_free(rsa);
    
    if( err ) {
	if( cert->req ) {
	    X509_REQ_free(cert->req);
	    cert->req = 0;
	}
	if( cert->pkey ) {
	    EVP_PKEY_free(cert->pkey);
	    cert->pkey = 0;
	}

    }
    return err;
}

int
cryptbk_set_subject(cryptbk_t *cert, char *dn) {
    int i, err=-1;
    X509_NAME *name=0;
    do {
	assertb(cert->cert);
	name = X509_get_subject_name(cert->cert);
	assertb(name);
	i = x509_name_pack(name, dn);
	assertb(i==0);
	i = X509_set_subject_name(cert->cert, name);
	assertb(i);
	err = 0;
    } while(0);
    return err;
}


int
cryptbk_req_sign(cryptbk_t *req, cryptbk_t *signing_cert, 
		 mstime_t notbefore, mstime_t notafter, int serial) {
    int i, j, err=-1;
    EVP_PKEY *pktmp=0;
    STACK_OF(X509_EXTENSION) *exts = NULL;
    X509 *cert=0;
    X509_NAME *name=0;
    time_t t;

    do {	
	cert = X509_new();
	assertb(cert);

	if( req->req ) {
	    pktmp = X509_REQ_get_pubkey(req->req);
	}
	else if( req->cert ) {
	    pktmp = X509_get_pubkey(req->cert);
	}
	assertb(pktmp);
	i = X509_set_pubkey(cert, pktmp);
	assertb(i);

	i = X509_set_version(cert, 2);
	assertb(i);

	if( req->req ) {
	    name = X509_REQ_get_subject_name(req->req);
	}
	else if( req->cert ) {
	    name = X509_get_subject_name(req->cert);
	}
	assertb(name);
	i = X509_set_subject_name(cert, name);
	assertb(i);

	if( signing_cert->cert ) {
	    name = X509_get_subject_name(signing_cert->cert);
	}
	else if( signing_cert->req ) {
	    name = X509_REQ_get_subject_name(signing_cert->req);
	}
	i = X509_set_issuer_name(cert, name);
	assertb(i);

	i = ASN1_INTEGER_set(X509_get_serialNumber(cert),serial);
	assertb(i);

	t = (time_t)notbefore;
	X509_time_adj(X509_get_notBefore(cert), 0, &t);

	t = (time_t)notafter;
	X509_time_adj(X509_get_notAfter(cert), 0, &t);

	if( req->req ) {
	    exts = X509_REQ_get_extensions(req->req);
	}
	else if( req->cert ) {
	    exts = req->cert->cert_info->extensions;
	}

	if( exts ) {
	    for(i=0; i<sk_X509_EXTENSION_num(exts); i++) {
		X509_EXTENSION *ext;
		ext = sk_X509_EXTENSION_value(exts, i);
		j = X509_add_ext(cert, ext, -1);
		assertb(j);
	    }
	    assertb(i>=sk_X509_EXTENSION_num(exts));
	}

	i = X509_sign(cert, signing_cert->pkey, EVP_md5());
	assertb(i);
	
	err = 0;
    } while(0);
    //if( exts ) 	sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
    //if( pktmp ) EVP_PKEY_free(pktmp);

    if( err ) {
	if( cert ) X509_free(cert);
	cert = 0;
    }
    else {
	if( req->cert ) {
	    X509_free(req->cert);
	}
	req->cert = cert;
    }
    return err;
}    

time_t
utc2time(ASN1_UTCTIME *utc) {
    struct tm tm;
    time_t t = 0;
    int i, err=-1;

    do {
	memset(&tm, 0, sizeof(tm));
	tm.tm_isdst = -1;
	i = sscanf((char*)utc->data, 
		   "%02d%02d%02d%02d%02d%02d"
		   ,&tm.tm_year
		   ,&tm.tm_mon
		   ,&tm.tm_mday
		   ,&tm.tm_hour
		   ,&tm.tm_min
		   ,&tm.tm_sec
		   );
	assertb(i==6);
	tm.tm_mon -= 1;
	if( tm.tm_year < 69 ) {
	    tm.tm_year += 100; /* stupid 2-digit date.  I assume they're all
	    			after 2000 */
	}
	t = mktime(&tm);
	if( t < 0 ) {
	    debug(DEBUG_WARN, 
		  ("utc2time: t=%d, tm={%d,%d,%d,%d,%d,%d} utc->data=[%s]\n"
		   ,t 
		   ,tm.tm_year
		   ,tm.tm_mon
		   ,tm.tm_mday
		   ,tm.tm_hour
		   ,tm.tm_min
		   ,tm.tm_sec
		   ,utc->data
		   ));
	}

	assertb(t>=0);

	// mktime works in the local time zone, so convert back to UTC
	t += (time_t)mstime_gmt_offset(t);
	
	err = 0;
    } while(0);
    return err ? 0 : t;
}

int
cryptbk_info(cryptbk_t *cert, cryptbk_info_t *info, int mode) {
    X509_NAME *name=0;
    int err=-1;
    char buf[4096];
    ASN1_UTCTIME *utc;
    int get_cert=0, get_req=0;
    EVP_PKEY *pkey=0;

    do {
	if( mode == 0 ) {
	    /* CRYPTBK_MODE_PUBKEY is the default when mode == 0 */
	    mode = CRYPTBK_MODE_PUBKEY;
	}

	if( mode & CRYPTBK_MODE_PUBKEY && cert->cert ) {
	    get_cert = 1;
	}
	else if( mode & CRYPTBK_MODE_REQ && cert->req ) {
	    get_req = 1;
	}
	else {
	    err = 0;
	    break;
	}

	memset(info, 0, sizeof(*info));

	if( get_cert ) {
	    name = X509_get_subject_name(cert->cert);
	}
	else if ( get_req ) {
	    name = X509_REQ_get_subject_name(cert->req);
	}
	if( name ) {
	    info->subject = X509_NAME_oneline(name, buf, sizeof(buf));
	    info->subject = strdup(info->subject);
	}

	if( get_cert ) {
	    name = X509_get_issuer_name(cert->cert);
	    info->issuer = X509_NAME_oneline(name, buf, sizeof(buf));
	    info->issuer = strdup(info->issuer);
	    
	    info->serial = ASN1_INTEGER_get(X509_get_serialNumber(cert->cert));

	    utc = X509_get_notBefore(cert->cert);
	    info->notbefore = utc2time(utc);

	    utc = X509_get_notAfter(cert->cert);
	    info->notafter = utc2time(utc);
	}

	info->exts = calloc(sizeof(info->exts), 1);
	assertb(info->exts);
	if( get_cert ) {
	    info->exts->exts = cert->cert->cert_info->extensions;
	}
	else if( get_req ) {
	    info->exts->exts = X509_REQ_get_extensions(cert->req);
	}

	if( get_cert ) {
	    pkey = X509_get_pubkey(cert->cert);
	}
	else if( get_req ) {
	    pkey = X509_REQ_get_pubkey(cert->req);
	}
	if( pkey ) {
	    info->bits = EVP_PKEY_bits(pkey);
	}

	err = mode;
    } while(0);
    return err;
}

int
cryptbk_info_free(cryptbk_info_t *info) {
    if( info->subject ) free(info->subject);
    if( info->issuer ) free(info->issuer);
    if( info->exts ) free( info->exts );
    memset(info, 0, sizeof(*info));
    return 0;
}


char *
cryptbk_fmt(cryptbk_t *crypt, char *buf, int len) {
    cryptbk_info_t info;
    int i, err=-1;
    char buf1[1024], buf2[1024];
    char *orig = buf;
    
    info.subject = 0;
    do {
	i = cryptbk_info(crypt, &info, 0);
	assertb(i>=0);
	
	i = snprintf(buf, len, 
		     "bits=%d"
		     " subject=%s"
		     " issuer=%s"
		     " notbefore=%s"
		     " notafter=%s"
		     ,info.bits
		     ,info.subject
		     ,info.issuer
 		     ,mstime_fmt(info.notbefore, buf1, sizeof(buf1))
		     ,mstime_fmt(info.notafter, buf2, sizeof(buf2))
		     );
	assertb(i>0);
	buf += i;
	len -= i;
	
	err =0;
    } while(0);
    if( info.subject ) {
	cryptbk_info_free(&info);
    }
    return err ? 0 : orig;
}

int
cryptbk_rand_buf(char *buf, int len) {
    RAND_bytes((unsigned char*)buf, len);
    return len;
}

unsigned long
cryptbk_rand_u32(unsigned long max) {
    unsigned long u;
    cryptbk_rand_buf((char*)&u, sizeof(u));
    return u % max;
}

int
cryptbk_seskey_rand(cryptbk_t *cert, int len) {
    int err=-1;
    do {
	if( cert->seskey ) {
	    free(cert->seskey);
	}
	cert->seskey = malloc(len);
	assertb(cert->seskey);
	cryptbk_rand_buf(cert->seskey, len);
	cert->seskey_len = len;
	err = len;
    } while(0);
    return err;
}

char *
cryptbk_seskey_get(cryptbk_t *cert) {
    return cert->seskey;
}

int
cryptbk_seskey_len(cryptbk_t *cert) {
    return cert->seskey_len;
}

int
cryptbk_seskey_set(cryptbk_t *cert, char *buf, int len) {
    int err=-1;
    char *seskey=0;
    
    do {
	seskey = malloc(len);
	assertb(seskey);
	memcpy(seskey, buf, len);
	
	if( cert->seskey ) {
	    free(cert->seskey);
	}
	cert->seskey = seskey;
	cert->seskey_len = len;
	err = 0;
    } while(0);
    return err;
}

int
cryptbk_seskey_swap(cryptbk_t *cert, char **pbuf, int *plen) {
    char *p;
    int n;

    SWAP(*pbuf, cert->seskey, p);
    SWAP(*plen, cert->seskey_len, n);
    return 0;
}


static char
hex_char(int i) {
    static char *hex_chars = "0123456789abcdef";
    return hex_chars[i%16];
}
	    
static int
hex_string(char *dst, int dstlen, int srclen) {
    int i, err=-1;
    do {
	assertb(dstlen >= 2*srclen+1);
	dst[2*srclen] = 0;
	for(i=srclen-1; i>=0; i--) {
	    dst[2*i+1] = hex_char(((dst[i]>>0)&0xf));
	    dst[2*i]   = hex_char(((dst[i]>>4)&0xf));
	}
	err = 2*srclen+1;
    } while(0);
    return err;
}


int
cryptbk_crypt(cryptbk_t *cert, 
	      char *src, int srclen, 
	      char *dst, int dstlen, 
	      int mode) {
    EVP_PKEY *pkey=0;
    int i, err=-1, off, len;
    BIO *bio=0;
    BUF_MEM *bptr=0;

    do {
	if( srclen <= 0 ) {
	    srclen = strlen(src);
	}

	if( mode & (CRYPTBK_MODE_PUBKEY | CRYPTBK_MODE_PRIVKEY) ) {
	    RSA *rsa=0;
	    char *dst_orig = dst;
	    int rsalen=0;

	    if( mode & CRYPTBK_MODE_PUBKEY ) {
		// get the public key
		assertb(cert->cert);
		pkey = X509_get_pubkey(cert->cert);
		assertb(pkey);
		rsa  = pkey->pkey.rsa; 
		assertb(rsa);
	    }
	    else if( mode & CRYPTBK_MODE_PRIVKEY ) {	
		// use the private key
		pkey = cert->pkey;
		assertb(pkey);
		rsa  = pkey->pkey.rsa; 
		assertb(rsa);
	    }

	    if( mode & CRYPTBK_MODE_ENC ) {
		// leave room for padding, see RSA_public_encrypt(3)
		rsalen = RSA_size(rsa) - 11 - 1; 
	    }
	    else if( mode & CRYPTBK_MODE_DEC ) {
		rsalen = RSA_size(rsa);
	    }

	    if( 1 ) {
		// disabled because RSA works in block of around ~256
		// chars.  That's about my message size, so I don't
		// gain a lot by encrypting a key first.  Well... do
		// I?  Let me see...


		// encrypt a random session key with the rsa key, then
		// encrypt the message with the session key
#define RSA_KEYLEN (2*MD5_DIGEST_LENGTH)
		char key[RSA_KEYLEN];
		int keylen = RSA_KEYLEN;
		char *old_seskey = cert->seskey;
		int old_seskey_len = cert->seskey_len;
	    
		assertb(keylen <= rsalen);
		cert->seskey = key;
		cert->seskey_len = keylen;
		if( mode & CRYPTBK_MODE_ENC ) {	
		    off = 0;

		    // encrypt the first rsalen bytes with pub/privkey functions
		    i = MIN(rsalen, srclen);
		    if( mode & CRYPTBK_MODE_PUBKEY ) {	
			off = RSA_public_encrypt(i, (unsigned char*)src, (unsigned char*)dst, 
						 rsa, RSA_PKCS1_PADDING);
		    }
		    else if( mode & CRYPTBK_MODE_PRIVKEY ) {	
			off = RSA_private_encrypt(i, (unsigned char*)src, (unsigned char*)dst, 
						  rsa, RSA_PKCS1_PADDING);
		    }
		    assertb(off>0);
		    
		    // encrypt the rest with an MD5 of the plain and encrypted src
		    if( srclen > i ) {
			// plain
			len = cryptbk_crypt(cert, src, i, key, keylen, 
					    CRYPTBK_MODE_ENC | CRYPTBK_MODE_MD5);
			assertb(len == MD5_DIGEST_LENGTH);
			// encrypted
			len = cryptbk_crypt(cert, key, len, key+MD5_DIGEST_LENGTH, keylen-MD5_DIGEST_LENGTH, 
					    CRYPTBK_MODE_ENC | CRYPTBK_MODE_MD5);
			assertb(len == MD5_DIGEST_LENGTH);
			len = cryptbk_crypt(cert, src+i, srclen-i, dst+off, dstlen-off, 
					    CRYPTBK_MODE_ENC | CRYPTBK_MODE_SESKEY);
			assertb(len >0);
			err = off + len;
		    }
		    else {
			err = off;
		    }
		}
		else if( mode & CRYPTBK_MODE_DEC ) {
		    off = 0;
		    assertb(dstlen >= rsalen);
		    if( mode & CRYPTBK_MODE_PUBKEY ) {	
			off = RSA_public_decrypt(rsalen, (unsigned char*)src, (unsigned char*)dst,
						 rsa, RSA_PKCS1_PADDING);
		    }
		    else if( mode & CRYPTBK_MODE_PRIVKEY ) {	
			off = RSA_private_decrypt(rsalen, (unsigned char*)src, (unsigned char*)dst,
						  rsa, RSA_PKCS1_PADDING);
		    }
		    assertb(off>0);

		    // use two MD5s of the plain and encrypted src as the encryption key for the rest
		    if( srclen > rsalen ) {
			// plain
			len = cryptbk_crypt(cert, dst, off, key, keylen, 
					    CRYPTBK_MODE_ENC | CRYPTBK_MODE_MD5);
			assertb(len == MD5_DIGEST_LENGTH);
			// encrypted
			len = cryptbk_crypt(cert, key, len, key+MD5_DIGEST_LENGTH, keylen-MD5_DIGEST_LENGTH, 
					    CRYPTBK_MODE_ENC | CRYPTBK_MODE_MD5);
			assertb(len == MD5_DIGEST_LENGTH);
			len = cryptbk_crypt(cert, src+rsalen, srclen-rsalen, dst+off, dstlen-off, 
					    CRYPTBK_MODE_DEC | CRYPTBK_MODE_SESKEY);
			assertb(len > 0);
			err = off + len;
		    }
		    else {
			err = off;
		    }
		}
		cert->seskey = old_seskey;
		cert->seskey_len = old_seskey_len;
	    }
	    else {
		// break into hunks <= rsalen
		while(srclen>0) {
		    i = srclen <= rsalen ? srclen : rsalen;
		    err = -1;
		    assertb(dstlen >= RSA_size(rsa));
		    if( mode & CRYPTBK_MODE_PUBKEY ) {
			if( mode & CRYPTBK_MODE_ENC ) {	
			    err = RSA_public_encrypt(i, (unsigned char*)src, (unsigned char*)dst, 
						     rsa, RSA_PKCS1_PADDING);
			}
			else if( mode & CRYPTBK_MODE_DEC ) {	
			    err = RSA_public_decrypt(i, (unsigned char*)src, (unsigned char*)dst, 
						     rsa, RSA_PKCS1_PADDING);
			}
		    }
		    else if( mode & CRYPTBK_MODE_PRIVKEY ) {
			if( mode & CRYPTBK_MODE_ENC ) {	
			    err = RSA_private_encrypt(i, (unsigned char*)src, (unsigned char*)dst,
						      rsa,
						      RSA_PKCS1_PADDING);
			}
			else if( mode & CRYPTBK_MODE_DEC ) {	
			    err = RSA_private_decrypt(i, (unsigned char*)src, (unsigned char*)dst,
						      rsa,
						      RSA_PKCS1_PADDING);
			}
		    }
		    assertb(err > 0);
		    src += i;
		    srclen -= i;
		    dst += err;
		    dstlen -= err;
		}
		assertb_ssl(err > 0);
		err = dst - dst_orig;
	    }
	}
#define CRYPTBK_CIPHER_AES
#ifdef CRYPTBK_CIPHER_AES
	else if( mode & (CRYPTBK_MODE_AES | CRYPTBK_MODE_SESKEY) ) {
	    EVP_CIPHER_CTX ctx;
	    const EVP_CIPHER *cipher=0;
	    char iv[EVP_MAX_IV_LENGTH];
	    int do_encrypt = ((mode & CRYPTBK_MODE_ENC) ? 1 : 0);
	    int outlen;
	    int keylen;

	    memset(iv, 0xdc, sizeof(iv));

	    EVP_CIPHER_CTX_init(&ctx);

	    keylen = cert->seskey_len;

	    if( keylen*8 >= 256 ) {
		cipher = EVP_aes_256_cfb();
		keylen = 256/8;
	    }
	    else if( keylen*8 >= 192 ) {
		cipher = EVP_aes_192_cfb();
		keylen = 192/8;
	    }
	    else if( keylen*8 >= 128 ) {
		cipher = EVP_aes_128_cfb();
		keylen = 128/8;
	    }

	    assertb(cipher);

	    i = EVP_CipherInit_ex(&ctx, cipher, 0, 0, 0, do_encrypt);
	    assertb_ssl(i);
	    i = EVP_CIPHER_CTX_set_key_length(&ctx, keylen);	
	    assertb_ssl(i);
	    i = EVP_CipherInit_ex(&ctx, 0, 0, 
				  (unsigned char*)cert->seskey,
				  (unsigned char*)iv, do_encrypt);
	    assertb_ssl(i);

	    outlen = dstlen;
	    i = EVP_EncryptUpdate(&ctx, (unsigned char*)dst, &outlen, (unsigned char*)src, srclen);
	    assertb(i);
	    assertb(outlen>0 && outlen<=dstlen);
	    
	    i = dstlen-outlen;
	    EVP_CipherFinal(&ctx, (unsigned char*)(dst+outlen), &i);
	    outlen += i;
	    EVP_CIPHER_CTX_cleanup(&ctx);
	    err = outlen;
	}
#endif // CRYPTBK_CIPHER_AES
	else if( mode & CRYPTBK_MODE_SESKEY ) {
	    BF_KEY key;
#define CRYPTBK_IV_LEN 8
	    char iv[CRYPTBK_IV_LEN];
	    
	    assertb(cert->seskey && cert->seskey_len > 0);
	    BF_set_key(&key, cert->seskey_len, (unsigned char*)cert->seskey);

	    i = 0;
	    if( mode & CRYPTBK_MODE_ENC ) {
		assertb(dstlen > srclen+CRYPTBK_IV_LEN);
		cryptbk_rand_buf(iv, CRYPTBK_IV_LEN);
		memcpy(dst, iv, CRYPTBK_IV_LEN);
		BF_cfb64_encrypt((unsigned char*)src, 
				 (unsigned char*)dst+CRYPTBK_IV_LEN, 
				 srclen, 
				 &key, (unsigned char*)iv, &i, BF_ENCRYPT);
		err = srclen + CRYPTBK_IV_LEN;
	    }
	    else if( mode & CRYPTBK_MODE_DEC ) {
		memcpy(iv, src, CRYPTBK_IV_LEN);
		BF_cfb64_encrypt((unsigned char*)(src+CRYPTBK_IV_LEN), 
				 (unsigned char*)dst, 
				 srclen-CRYPTBK_IV_LEN, &key, (unsigned char*)iv, &i, 
				 BF_DECRYPT);
		err = srclen - CRYPTBK_IV_LEN;
	    }
	    assertb_ssl(err >= 0);
	}
	else if( mode & CRYPTBK_MODE_MD5 ) {
	    MD5_CTX md5;
	    
	    assertb(dstlen >= MD5_DIGEST_LENGTH);

	    MD5_Init(&md5);
	    MD5_Update(&md5, src, srclen);
	    
	    if( mode & CRYPTBK_MODE_ENC ) {
		MD5_Final((unsigned char*)dst, &md5);
		err = MD5_DIGEST_LENGTH;

		// convert to hex string (with null)
		if( mode & CRYPTBK_MODE_TEXT ) {
		    err = hex_string(dst, dstlen, MD5_DIGEST_LENGTH);
		    assertb(err>0);
		}
	    }
	    else if( mode & CRYPTBK_MODE_DEC ) {
		char md5buf[2*MD5_DIGEST_LENGTH+1];

		// verify md5
		MD5_Final((unsigned char*)md5buf, &md5);
		if( mode & CRYPTBK_MODE_TEXT ) {
		    err = hex_string(md5buf, sizeof(md5buf), MD5_DIGEST_LENGTH);
		    assertb(err>0);
		    err = strcasecmp(md5buf, dst);
		}
		else {
		    err = memcmp(md5buf, dst, MD5_DIGEST_LENGTH);
		}
		if( err != 0 ) {
		    err = -1;
		}
	    }
	}
	else if( mode & CRYPTBK_MODE_BASE64 ) {
	    BIO *b64, *bio_buf;

	    b64 = BIO_new(BIO_f_base64());
	    if( mode & CRYPTBK_MODE_ENC ) {
		bio_buf = BIO_new(BIO_s_mem());
		bio = BIO_push(b64, bio_buf);

		i = BIO_write(bio, src, srclen);
		assertb(i==srclen);
		i = BIO_flush(bio);

		BIO_get_mem_ptr(bio, &bptr);
		assertb(bptr->data && bptr->length+1 < dstlen)
		err = bptr->length;
		memcpy(dst, bptr->data, bptr->length);
	    }
	    else if( mode & CRYPTBK_MODE_DEC ) {
		int off;
		bio_buf = BIO_new_mem_buf(src, srclen);
		bio = BIO_push(b64, bio_buf);
		off = 0;
		while(off<dstlen) {
		    i = BIO_read(bio, dst+off, dstlen-off);
		    if( i < 0 ) { off = -1; }
		    if( i <=0 ) { break; }
		    off += i;
		}
		err = off;
	    }
	}
    } while(0);

    // null-terminate the dst buffer if there's room
    if( err > 0 && err < dstlen ) {
	dst[err] = 0;
    }

    if( bio ) 	    BIO_free_all(bio);

    return err;
}

static
int
pass_cb(char *buf, int size, int rwflag, void *u) {
    int i = 0;
    if( u ) {
	i = snprintf(buf, size, "%s", (char*)u);
    }
    return i;
}


int
cryptbk_pack(cryptbk_t *cert, char *buf, int len, char *pass, int mode) {
    BIO *bio=0;
    BUF_MEM *bptr;
    int i, err=-1;
    char *p;
    char *orig = buf;
    
    do {
	if( len <= 0 ) {
	    len = strlen(buf);
	}

	if( mode & CRYPTBK_MODE_DEC ) {
	    bio = BIO_new_mem_buf(buf, len);
	    assertb_ssl(bio);
	}
	else if( mode & CRYPTBK_MODE_ENC ) {
	    if( mode & CRYPTBK_MODE_TEXT ) {
		bio = BIO_new(BIO_s_mem());
	    }

	}
	else {
	    assertb(0);
	}

	if( mode & CRYPTBK_MODE_PUBKEY ) {
	    if( mode & CRYPTBK_MODE_DEC ) {
		if( cert->cert ) {
		    X509_free(cert->cert);
		    cert->cert = 0;
		}
		if( mode & CRYPTBK_MODE_TEXT ) {
		    cert->cert = PEM_read_bio_X509(bio, 0, 0, 0);
		    assertb_ssl(cert->cert);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    assertb(len>0);
		    p = buf;
		    cert->cert = d2i_X509(0, (const unsigned char**)&p, len);
		    assertb_ssl(cert->cert);
		    len -= p-buf;
		    buf = p;
		}
	    }
	    else if( mode & CRYPTBK_MODE_ENC ) {
		assertb(cert->cert);
		if( mode & CRYPTBK_MODE_TEXT ) {
		    i = PEM_write_bio_X509(bio, cert->cert);
		    assertb_ssl(i>0);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    i = i2d_X509(cert->cert, 0);
		    assertb(i>0);
		    if( i <= len ) {
			p = buf;
			i = i2d_X509(cert->cert, (unsigned char**)&p);
			assertb(i>0 && i<len);
		    }
		    buf += i;
		    len -= i;
		}
	    }
	}
	if( mode & CRYPTBK_MODE_REQ ) { 
	    if( mode & CRYPTBK_MODE_DEC ) {
		if( cert->req ) {
		    X509_REQ_free(cert->req);
		    cert->req = 0;
		}
		if( mode & CRYPTBK_MODE_TEXT ) {
		    cert->req = PEM_read_bio_X509_REQ(bio, 0, 0, 0);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    assertb(len>0);
		    p = buf;
		    cert->req = d2i_X509_REQ(0, (const unsigned char **)&p, len);
		    len -= p-buf;
		    buf = p;
		}
		assertb_ssl(cert->req);
	    }
	    else if( mode & CRYPTBK_MODE_ENC ) {
		assertb(cert->req);
		if( mode & CRYPTBK_MODE_TEXT ) {
		    i = PEM_write_bio_X509_REQ(bio, cert->req);
		    assertb_ssl(i>0);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    i = i2d_X509_REQ(cert->req, 0);
		    assertb(i>0);
		    if( i <= len ) {
			p = buf;
			i = i2d_X509_REQ(cert->req, (unsigned char**)&p);
			assertb(i>0 && i<len);
		    }
		    buf += i;
		    len -= i;
		}
	    }
	}
	if( mode & CRYPTBK_MODE_PRIVKEY ) { 
	    if( mode & CRYPTBK_MODE_DEC ) {
		if( cert->pkey ) {
		    EVP_PKEY_free(cert->pkey);
		    cert->pkey = 0;
		}
		if( mode & CRYPTBK_MODE_TEXT ) {
		    cert->pkey = PEM_read_bio_PrivateKey(bio, 0, pass_cb, (unsigned char*)pass);
		    assertb_ssl(cert->pkey);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    assertb(len>0);
		    p = buf;
		    cert->pkey = d2i_AutoPrivateKey(0, (const unsigned char **)&p, len);
		    assertb_ssl(cert->pkey);
		    len -= p-buf;
		    buf = p;
		}
	    }
	    else if( mode & CRYPTBK_MODE_ENC ) {
		assertb(cert->pkey);
		if( mode & CRYPTBK_MODE_TEXT ) {
		    if( pass ) {
			i = PEM_write_bio_PrivateKey(bio, cert->pkey, 
						     EVP_des_ede3_cbc(),
						     (unsigned char*)pass, strlen(pass), 
						     0, 0);
		    }
		    else {
			i = PEM_write_bio_PrivateKey(bio, cert->pkey, 
						     0, 0, 
						     0, 0, 0);
		    }
		    assertb_ssl(i>0);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    i = i2d_PrivateKey(cert->pkey, 0);
		    assertb(i>0);
		    if( i <= len ) {
			p = buf;
			i = i2d_PrivateKey(cert->pkey, (unsigned char**)&p);
			assertb(i>0 && i<len);
		    }
		    buf += i;
		    len -= i;
		}
	    }
	}
	if( mode & CRYPTBK_MODE_SESKEY ) {
	    if( mode & CRYPTBK_MODE_DEC ) {
		if( cert->seskey ) {
		    free(cert->seskey);
		    cert->seskey = 0;
		}
		if( mode & CRYPTBK_MODE_TEXT ) {
		    char *name, *data, *hdr;
		    long len;
		    i = PEM_read_bio(bio, &name, &hdr, (unsigned char**)&data, &len);
		    assertb(i>0);
		    assertb(strcasecmp(name, CRYPTBK_SESKEY_PEM_NAME)==0);
		    cert->seskey = malloc(len);
		    cert->seskey_len = len;
		    memcpy(cert->seskey, data, len);
		    free(name);
		    free(hdr);
		    free(data);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    assertb(len >= sizeof(unsigned short));
		    i = ntohs(*(unsigned short*)buf);
		    buf += sizeof(unsigned short);
		    len -= sizeof(unsigned short);
		    assertb(i>0 && i<=len);
		    cert->seskey = malloc(i);
		    assertb(cert->seskey);
		    memcpy(cert->seskey, buf, i);
		    buf += i;
		    len -= i;
		}
	    }
	    else if( mode & CRYPTBK_MODE_ENC ) {
		assertb(cert->seskey);
		if( mode & CRYPTBK_MODE_TEXT ) {
		    i = PEM_write_bio(bio, CRYPTBK_SESKEY_PEM_NAME, "", 
				      (unsigned char*)cert->seskey, cert->seskey_len);
		    assertb(i>0);
		}
		else if( mode & CRYPTBK_MODE_BINARY ) {
		    assertb(len >= (int)sizeof(unsigned short) + cert->seskey_len);
		    *(unsigned short*)buf = ntohs((short)cert->seskey_len);
		    buf += sizeof(unsigned short);
		    len -= sizeof(unsigned short);
		    i = cert->seskey_len;
		    memcpy(buf, cert->seskey, i);
		    buf += i;
		    len -= i;
		}
	    }
	}

	if( mode & CRYPTBK_MODE_TEXT ) {
	    if( mode & CRYPTBK_MODE_DEC ) {
		/* decrypting text (PEM) from bio? count the bytes read */
		i = BIO_tell(bio);
		if( i == 0 ) { 
		    // kludge - BIO_tell() returns 0 instead of the bytes read, so I assume I read them all
		    i = len; 
		}
		buf += i;
		len -= i;
	    }
	    else if( mode & CRYPTBK_MODE_ENC ) {
		/* encrypting text (PEM) to bio? copy text from bio */
		BIO_get_mem_ptr(bio, &bptr);
		i = bptr->length;
		assertb( i <= len );
		memcpy(buf, bptr->data, bptr->length);
		buf += i;
		len -= i;
		if( len > 0 ) {
		    *buf = 0;
		}
	    }
	}

	err = 0;
    } while(0);
    if( bio ) BIO_free(bio);

    if( err ) return err;

    err = buf-orig;

    // null-terminate strings if there's room
    if( ((mode & CRYPTBK_MODE_DEC) || (mode & CRYPTBK_MODE_TEXT))
	&& err > 0 
	&& err < len ) {
	buf[err] = 0;
    }

    return err;
}

int
cryptbk_trust(cryptbk_t *cert, cryptbk_t *trusted) {
    int i, err=-1;
    do {
	if( !cert->store ) {
	    cert->store=X509_STORE_new();
	    assertb_ssl(cert->store);
	}
	i = X509_STORE_add_cert(cert->store, trusted->cert);
	assertb_ssl(i);
	err = 0;
    } while(0);
    return err;
}


int
cryptbk_x509_verify_cb(int ok, X509_STORE_CTX *ctx) {
    char buf[4096];
    int i;
    
    switch(ctx->error) {
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	// probably forgot cryptbk_init()?
	//case X509_V_ERR_CERT_SIGNATURE_FAILURE: 
	ok=1;
    }
    if( ok ) { return ok; }

    X509_NAME_oneline(X509_get_subject_name(ctx->current_cert), buf,
		      sizeof(buf));
    i = strlen(buf);
    snprintf(buf+i, sizeof(buf)-i,
	     " error=%d at depth=%d ok=%d string=%s\n",
	     ctx->error,
	     ctx->error_depth,
	     ok,
	     X509_verify_cert_error_string(ctx->error)
	     );
    warn(("%s", buf));
    ERR_clear_error();
    return ok;
}

int
cryptbk_verify(cryptbk_t *cert, cryptbk_t *dubious) {
    X509_STORE_CTX *store_ctx=0;
    int i, err=-1;

    do {
	if( !cert->store ) {
	    cert->store=X509_STORE_new();
	    assertb_ssl(cert->store);
	}
	X509_STORE_set_verify_cb_func(cert->store, cryptbk_x509_verify_cb);
	store_ctx = X509_STORE_CTX_new();
	assertb_ssl(store_ctx);
	X509_STORE_CTX_init(store_ctx, cert->store, dubious->cert, 0);
	ERR_clear_error();
	i = X509_verify_cert(store_ctx);
	if( i <= 0 ) {
	    switch(store_ctx->error) {
	    case X509_V_ERR_CERT_HAS_EXPIRED: 
		err = CRYPTBK_ERR_EXPIRED; 
		break;
	    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		err = CRYPTBK_ERR_SELFSIGN;
		break;
	    case X509_V_ERR_CERT_SIGNATURE_FAILURE: 
		err = CRYPTBK_ERR_SIGNATURE;
		break;
	    }
	    assertb(0);
	}
	
	err = 0;
    } while(0);
    if( store_ctx ) {
	X509_STORE_CTX_free(store_ctx);
    }
    return err;
}

// makes sure 
int
cryptbk_check_pubkey(cryptbk_t *priv, cryptbk_t *pub) {
    char buf[4096], enc[2*4096], dec[2*4096];
    int i, j, err=-1;
    do {
	i = cryptbk_rand_buf(buf, sizeof(buf));
	assertb(i==sizeof(buf));
	i = cryptbk_crypt(pub, buf, i, enc, sizeof(enc), 
			  CRYPTBK_MODE_ENC | CRYPTBK_MODE_PUBKEY);
	assertb(i>0 && i<sizeof(enc));
	j = cryptbk_crypt(priv, enc, i, dec, sizeof(dec),
			  CRYPTBK_MODE_DEC | CRYPTBK_MODE_PRIVKEY);
	assertb(j>0 && j<sizeof(dec));
	if( j != sizeof(buf) ) {
	    break;
	}
	if( memcmp(buf, dec, j) ) {
	    break;
	}
	err = 0;
    } while(0);
    return err;
}

// makes sure 
int
cryptbk_cmp(cryptbk_t *a, cryptbk_t *b, int mode) {
    char ab[4096], bb[4096];
    int i, j;
    do {
	mode = CRYPTBK_MODE_ENC | CRYPTBK_MODE_BINARY | mode;
	i = cryptbk_pack(a, ab, sizeof(ab), 0, mode);
	assertb(i < sizeof(ab));
	j = cryptbk_pack(b, bb, sizeof(bb), 0, mode);
	assertb(j < sizeof(bb));
	if( i != j ) return i - j;
	return memcmp(ab, bb, i);
    } while(0);
    return -1;
}

int
cryptbk_copy(cryptbk_t *dst, cryptbk_t *src, int mode) {
    char buf[8192];
    int i, err=-1;
    do {
	mode = CRYPTBK_MODE_ENC | CRYPTBK_MODE_BINARY | mode;
	i = cryptbk_pack(src, buf, sizeof(buf), 0,
			 CRYPTBK_MODE_ENC | CRYPTBK_MODE_BINARY | mode);
	assertb(i < sizeof(buf));
	i = cryptbk_pack(dst, buf, i, 0,
			 CRYPTBK_MODE_DEC | CRYPTBK_MODE_BINARY | mode);
	assertb(i < sizeof(buf));
	err = 0;
    } while(0);
    return err;
}

int
cryptbk_has(cryptbk_t *c, int mode) {
    int ret = 0;
    if( mode & CRYPTBK_MODE_SESKEY && c->seskey ) {
	ret |= CRYPTBK_MODE_SESKEY;
    }
    if( mode & CRYPTBK_MODE_PRIVKEY && c->pkey ) {
	ret |= CRYPTBK_MODE_PRIVKEY;
    }
    if( mode & CRYPTBK_MODE_PUBKEY && (c->cert || c->req) ) {
	ret |= CRYPTBK_MODE_PUBKEY;
    }
    return ret;
}

cryptbk_t *
cryptbk_selfsign(char *dn, cryptbk_exts_t *exts, int bits, unsigned long secs) {
    cryptbk_t *cert = 0;
    int i, err=-1;
    cryptbk_info_t info;
    //mstime_t t;
    int free_exts=0;
    do {
	//t = mstime();
	cert = cryptbk_new();
	assertb(cert);
	if( !exts ) {
	    exts = cryptbk_exts_new();
	    assertb(exts);
	    free_exts = 1;
	}
	i = cryptbk_req_gen(cert, bits, dn, exts, 0, 0);
	assertb(i>=0);
	i = cryptbk_req_sign(cert, cert, mstime(), mstime()+secs, 1);
	assertb(i>=0);
	i = cryptbk_info(cert, &info, CRYPTBK_MODE_PUBKEY);
	assertb(i>=0);
	i = cryptbk_info_free(&info);
	err = 0;
    } while(0);
    
    if( free_exts ) {
	cryptbk_exts_delete(exts);
    }
    if( err ) {
	if( cert ) {
	    cryptbk_delete(cert);
	    cert = 0;
	}
    }
    
    return cert;
}

char*
cn(char *p) {
    if( *p == '/' ) p++;
    if( strcasecmp(p, "CN=") ) p += 3;
    return p;
}
