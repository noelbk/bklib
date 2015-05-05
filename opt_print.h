#ifndef OPT_PRINT_H_INCLUDED
#define OPT_PRINT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    OPT_PRINT_FORMAT_TEXT
    ,OPT_PRINT_FORMAT_PERL
    ,OPT_PRINT_FORMAT_XML
} opt_print_format_t;

typedef struct opt_print_t {
    opt_print_format_t format;
    int print_indent;
} opt_print_t;

int
opt_print_format(opt_print_t *opt, char *format);

int
opt_print_begin(opt_print_t *opt);

int
opt_print_end(opt_print_t *opt);

int
opt_print_block_begin(opt_print_t *opt);

int
opt_print_block_end(opt_print_t *opt); 

int
opt_print_array_begin(opt_print_t *opt, char *label);

int
opt_print_array_end(opt_print_t *opt, char *label);

int
opt_print_kv(opt_print_t *opt, char *k, char *v);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // OPT_PRINT_H_INCLUDED
