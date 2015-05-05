#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "configbk.h"

#if OS == OS_WIN32
#include "bkwin32.h"
#endif

char*
env_get(char *key) {
    char *val=0;

#if OS == OS_WIN32
    char buf[4096];
    int i;
    i = GetEnvironmentVariable(key, buf, sizeof(buf));
    if( i ) {
	val = buf;
    }
#endif

#if OS == OS_UNIX
    val = getenv(key);
#endif

    if( val ) val = strdup(val);
    return val;
}


void
env_free(char *val) {
    if( val ) free(val);
}

/* returns 0 on success.  calls unsetenv(key) if val is null */
int
env_put(const char *key, const char *val) {
    int i;

#if OS == OS_WIN32
    i = SetEnvironmentVariable(key, val);
    i = !i;
#endif

#if OS == OS_UNIX
    if( val ) {
	i = setenv(key, val, 1);
    }
    else {
	unsetenv(key);
	i = 0;
    }
#endif
    return i;
}

