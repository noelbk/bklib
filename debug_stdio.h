#ifndef DEBUG_STDIO_H_INCLUDED
#define DEBUG_STDIO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include <stdio.h>

int
debug_func_stdio(const char *buf, void *arg);

void*
debug_func_log_init(const char *log_prefix, int files_max, int size_max, FILE *tee_file);

int
debug_func_log(const char *buf, void *arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DEBUG_STDIO_H_INCLUDED
