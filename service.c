#include <string.h>
#include "config.h"
#include "service.h"

char*
path2name(char *path, char *name, int len) {
    char *p;

    
    // copy last component after last [\\/]
    for(p=path+strlen(path)-1; p>path && *p != '/' && *p != '\\'; p--);
    if( *p == '/' || *p == '\\' ) p++;
    if( !*p ) return 0;
    strncpy(name, p, len);

    // strip file extension
    for(p=name+strlen(name)-1; p>name && *p != '.'; p--);
    if( *p == '.' ) *p = 0;

    return name;
}


#if OS == OS_UNIX
#include "service_unix.c"
#endif

#if OS == OS_WIN32
#include "service_win32.c"
#endif

