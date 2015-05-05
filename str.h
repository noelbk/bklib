#ifndef STR_H_INCLUDED
#define STR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define CHARS_NUMBER "0123456789"
#define CHARS_LOWER   "abcdefghijklmnopqrstuvwxyz"
#define CHARS_UPPER   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CHARS_ALNUM   CHARS_NUMBER CHARS_LOWER CHARS_UPPER
#define CHARS_CWORD   CHARS_NUMBER CHARS_LOWER CHARS_UPPER "_"
#define CHARS_FILENAME CHARS_ALNUM "-_."

char*
str_trim(char *str);

char*
str_collapse(char *str, char *chars, char *buf, int len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // STR_H_INCLUDED
