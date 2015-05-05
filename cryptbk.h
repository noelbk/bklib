#ifndef CRYPTBK_H_INCLUDED
#define CRYPTBK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mstime.h"

#define CRYPTBK_SESKEY_PEM_NAME "CRYTPTBK_SESKEY"

enum cryptbk_mode_t {
    CRYPTBK_MODE_ENC     = 1<<0,
    CRYPTBK_MODE_DEC     = 1<<1,

    CRYPTBK_MODE_TEXT    = 1<<2,
    CRYPTBK_MODE_BINARY  = 1<<3,

    CRYPTBK_MODE_SIGN    = 1<<4,
    CRYPTBK_MODE_MD5     = 1<<5,
    CRYPTBK_MODE_BASE64  = 1<<6,

    CRYPTBK_MODE_PUBKEY  = 1<<7,
    CRYPTBK_MODE_SESKEY  = 1<<8,
    CRYPTBK_MODE_PRIVKEY = 1<<9,
    CRYPTBK_MODE_REQ     = 1<<10,

    CRYPTBK_MODE_AES     = 1<<11,
};

enum cryptbk_err_t {
    CRYPTBK_ERR_NONE          = 0
    ,CRYPTBK_ERR_EXPIRED      = 1
    ,CRYPTBK_ERR_SELFSIGN     = 1<<1
    ,CRYPTBK_ERR_SIGNATURE    = 1<<2
};


#define CRYPTBK_MD5LEN (16)

#define CRYPTBK_SESKEY_LEN_MIN (128)
#define CRYPTBK_SESKEY_LEN_MAX (256)

// the max amount of room requitred to encrypt len bytes in PUKEY,
// PRIVKEY, or SESKEY modes.  MD5 is 16 bytes.  BASE64 is 9/8 size.  I
// think.
#define CRYPTBK_ENC_SIZE(len) (2*(len))

struct cryptbk_s;
typedef struct cryptbk_s cryptbk_t;

struct cryptbk_exts_s;
typedef struct cryptbk_exts_s cryptbk_exts_t;
typedef unsigned long cryptbk_ext_id_t;

extern const cryptbk_ext_id_t cryptbk_ext_id_subject_alt_name;

struct cryptbk_info_s {
    char *subject;
    char *issuer;
    int serial;
    mstime_t notbefore;
    mstime_t notafter;
    cryptbk_exts_t *exts;
    int bits;
};
typedef struct cryptbk_info_s cryptbk_info_t;

struct cryptbk_ext_info_s {
    char *shortname;
    char *longname;
    char *oidname;
    char *value;
    cryptbk_ext_id_t id;
};
typedef struct cryptbk_ext_info_s cryptbk_ext_info_t;

typedef int (*cryptbk_callback_t)(void *arg, int prog, char *msg);

int
cryptbk_init();

int
cryptbk_fini();

char *
cryptbk_fmt(cryptbk_t *crypt, char *buf, int buflen);

// create a new empty cert
cryptbk_t*
cryptbk_new();

// create a new cert with empty public key.  You call cryptbk_copy and
// cryptbk_set_subject to set the pubkey and subject, then
// cryptbk_req_sign to sign it.
cryptbk_t*
cryptbk_new_cert();

// set the subject name of a cert or request
int
cryptbk_set_subject(cryptbk_t *cert, char *dn);

void
cryptbk_delete(cryptbk_t *cert);

cryptbk_exts_t*
cryptbk_exts_new();

void
cryptbk_exts_delete(cryptbk_exts_t* exts);

int
cryptbk_exts_add(cryptbk_exts_t *exts, cryptbk_ext_id_t nid, char *value);

cryptbk_ext_id_t
cryptbk_ext_id_new(char *oid, char *shortname, char *longname);

int
cryptbk_exts_count(cryptbk_exts_t *exts);

int
cryptbk_exts_info(cryptbk_exts_t *exts, int idx, cryptbk_ext_info_t *info);

int
cryptbk_ext_info_free(cryptbk_ext_info_t *info);

int
cryptbk_req_gen(cryptbk_t *cert, int bits, char *dn, cryptbk_exts_t *exts,
		cryptbk_callback_t func, void *arg);

int
cryptbk_req_sign(cryptbk_t *req, cryptbk_t *signing_cert, 
		 mstime_t notbefore, mstime_t notafter, int serial);

// A cert may contain both a signed public certificate and a
// certificate request.  This gets info for either one, depending on
// the mode.
//
// CRYPTBK_MODE_PUBKEY - get info for public certificate
// CRYPTBK_MODE_REQ    - get info for certificate request
//
// returns:  >0: got info, =0: no info present, <0: error getting info
int
cryptbk_info(cryptbk_t *cert, cryptbk_info_t *info, int mode);

int
cryptbk_info_free(cryptbk_info_t *info);

int
cryptbk_rand_buf(char *buf, int len);

int
cryptbk_seskey_rand(cryptbk_t *cert, int len);

int
cryptbk_seskey_set(cryptbk_t *cert, char *buf, int len);

char*
cryptbk_seskey_get(cryptbk_t *cert);

int
cryptbk_seskey_len(cryptbk_t *cert);

int
cryptbk_seskey_swap(cryptbk_t *cert, char **pbuf, int *plen);

int
cryptbk_crypt(cryptbk_t *cert, 
	     char *src, int srclen, 
	     char *dst, int dstlen, 
	     int mode);

int
cryptbk_pack(cryptbk_t *cert, char *buf, int len, char *pass, int mode);

int
cryptbk_trust(cryptbk_t *cert, cryptbk_t *trusted);

    // returns 0 iff ok, cryptbk_err_t on error
int
cryptbk_verify(cryptbk_t *cert, cryptbk_t *dubious);

int
cryptbk_check_pubkey(cryptbk_t *priv, cryptbk_t *pub);

int
cryptbk_cmp(cryptbk_t *a, cryptbk_t *b, int mode);

int
cryptbk_copy(cryptbk_t *dst, cryptbk_t *src, int mode);

int
cryptbk_has(cryptbk_t *c, int mode);

cryptbk_t *
cryptbk_selfsign(char *dn, cryptbk_exts_t *exts, 
		 int bits, unsigned long secs);

char*
cn(char *dn);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CRYPTBK_H_INCLUDED
