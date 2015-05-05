#ifndef DIR_H_INCLUDED
#define DIR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "config.h"
#if OS & OS_UNIX
#include <unistd.h>
#endif

#if OS & OS_WIN32
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#endif


#define DIR_MAX_PATH 4096

struct dir_s;
typedef struct dir_s dir_t;

dir_t*
dir_open(char*path);

// read all entries, including . and ..
int
dir_read_dots(dir_t *d, char *buf, int len);

// read all entries, skipping . and .. returns 0 on eof, >0 on read,
// <0 on error
int
dir_read(dir_t *d, char *buf, int len);

int
dir_close(dir_t *d);

// return a sorted array of names
int
dir_sorted(char *path, char **names, int namelen, char *buf, int buflen);

// make one directory
int
dir_mkdir(char *dir);

// make all directories in path, except for the last component unless
// last is true
int
dir_mkpath(char *dir, int last);

char*
dir_filename(char *dir, int truncate);

char*
dir_where_exe(char *exename, char *fullpath, int fullpathlen);

char*
dir_basename(char *path, char *basename, int len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DIR_H_INCLUDED
