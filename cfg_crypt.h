#ifndef CFG_CRYPT_H_INLCUDED
#define CFG_CRYPT_H_INLCUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "cryptbk.h"
#include "cfg.h"

/* load or generate a certificate. */
cryptbk_t*
cfg_load_crypt(cfg_t *cfg, char *cfg_name
	       ,char *hostname, int hostnamelen
	       ,int bits, int expire);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //  CFG_CRYPT_H_INLCUDED
