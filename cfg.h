#ifndef CFG_H_INLCUDED
#define CFG_H_INLCUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "bklib/str.h"

typedef struct cfg_s cfg_t;

/* add "src\n" to dst, inc dst and dec dst_len */
int
cfg_list_pack(char **dst, int *dst_len, char *src);

/* extract next string in a newline-separated list */
char*
cfg_list_next(char **p, char *buf, int len);

cfg_t*
cfg_new(char *appname);

cfg_t*
cfg_new_rootdir(char *rootdir);

void
cfg_delete(cfg_t* cfg);

int
cfg_get(cfg_t *cfg, const char *key, char *val, int len);

char*
cfg_path(cfg_t *cfg, const char *key, char *path_buf, int path_len);

/* open, read, and close path, returns 0 iff ok */
int
cfg_cat(cfg_t *cfg, char *path, char *val, int len);

typedef enum {
    CFG_PUT_USER,
    CFG_PUT_GLOBAL
} cfg_put_where_t;

    /* uses strlen(val) if len < 0 */
int
cfg_put(cfg_t *cfg, const char *key, char *val, int len, cfg_put_where_t where);

int
cfg_put_int(cfg_t *cfg, const char *key, int val, cfg_put_where_t where);

int
cfg_get_int(cfg_t *cfg, const char *key, int *val);

// put val at the top of the mru
char*
mru(char *mru, char *val, char *dstbuf, int dstlen, int max);

typedef enum {
    CFG_DIR_NONE=0
    ,CFG_DIR_USER /* ~/.appname */
    ,CFG_DIR_GLOBAL /* /etc/appname */
    ,CFG_DIR_VAR /* /var/lib/appname */
    ,CFG_DIR_LOG /* /var/log/appname */
} cfg_dir_where_t;

char*
cfg_dir(cfg_t *cfg, cfg_dir_where_t where, char *val, int len);

struct cfg_enum_s;
typedef struct cfg_enum_s cfg_enum_t;

cfg_enum_t*
cfg_enum_open(cfg_t *cfg, cfg_dir_where_t where, char *subdir);

char*
cfg_enum_read(cfg_enum_t *e, char *buf, int len);

void
cfg_enum_close(cfg_enum_t *e);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif //  CFG_H_INLCUDED
