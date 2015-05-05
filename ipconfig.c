#include "bklib/ipconfig.h"

#if OS & OS_UNIX
/* noop  on UNIX */
int
ipconfig_get_all(ifcfg_config_t *cfg, int cfg_max) {
    return 0;
}
#endif // OS & OS_WIN32

#if OS & OS_WIN32
#include "bklib/ipconfig_win32.c"
#endif // OS & OS_WIN32

