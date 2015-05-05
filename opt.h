#ifndef OPT_H_INCLUDED
#define OPT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    char  *names;
    char  argtype;
    char  required;
    int   err;
    char  *defval;
    int   argidx;
    char  *argval;
    union {
	double f;
	long   i;
	char  *s;
    } val;
} OptSet;

#define OPT_QUOTED_CHARS " \n\r\t\\\""
int opt_needs_quote(char *str);

// encode a bunch of args with a trailing into a buffer, teriminated by "\n\0"
// call opt_quoted_enc on 
#define OPT_CAT_END ((char*)-1)
#define OPT_CAT_INT ((char*)-2)
int opt_cat(char *buf, int len, ...);

// encode as a C-style string
int opt_quoted_enc(char *src, int srclen, char *dst, int dstmax);

typedef enum {
    OPT_QUOTED_ENC_C
    ,OPT_QUOTED_ENC_PERL
} opt_quoted_enc_style_t;

// encode in C or perl
int opt_quoted_enc_style(char *src, int srclen, char *dst, int dstlen, opt_quoted_enc_style_t style);

int opt_quoted_dec(char **psrc, char *dst, int len, int noslash);



int opt_str2argv(char **str, int nargv, char **argv);

int opt_namev2optv(char **namev, int optc, OptSet *optv);

// returns 0 if not found
OptSet* opt_get(OptSet *optv, char *name);

int opt_setval(OptSet *opt, int *argidx, int argc, char **argv);

int opt_argv2optv(OptSet *optv, int argc, char **argv);

#define OPT_WORD_MAX 127

int opt_verify_word(const char *buf);

#define OPT_ERR_OPT     -1 // unknown option
#define OPT_ERR_ARGREQ  -2 // option's required argument missing
#define OPT_ERR_ARG     -3 // option's argument is bad
#define OPT_ERR_TYPE    -4 // option's type is bad
#define OPT_ERR_OPTREQ  -5 // required option missing

#include <stdlib.h>

typedef enum {
    OPT_ERROR_NONE = 0, /* no error */
    OPT_ERROR_END = -1, /* end of options */
    OPT_ERROR_UNKNOWN = -2,     /* unknown option */
    OPT_ERROR_MISSING = -3, /* missing argument */
    OPT_ERROR_BAD = -4     /* bad argument */
} opt_error_t;

typedef enum {
    OPT_TYPE_FLAG=1,
    OPT_TYPE_INT,
    OPT_TYPE_UINT,
    OPT_TYPE_REAL,
    OPT_TYPE_STR
} opt_type_t;

struct opt_set_s {
    int id;
    char *name;
    opt_type_t type;
};
typedef struct opt_set_s opt_set_t;
#define OPT_SET_END { 0,0,0 }

struct opt_iter_s {
    int id;
    char *name;
    opt_type_t type;

    int argi;
    opt_error_t error;
    char *arg;
    union {
	int i;
	unsigned u;
	char *s;
	double d;
    } val;
};
typedef struct opt_iter_s opt_iter_t;

/* extract one argument from an argv list starting at opt->argi.  sets
   fields in opt, and incements opt->argi.
*/
int
opt_get_argv(opt_set_t *opts, int argc, char **argv, opt_iter_t *opt);

#include <stdio.h>

int
opt_perror(opt_iter_t *opt, FILE *f);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // OPT_H_INCLUDED

