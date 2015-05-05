#ifndef IPCONFIG_H_INCLUDED
#define IPCONFIG_H_INCLUDED

#include "bklib/ifcfg.h"

#ifdef __cplusplus
#extern "C" {
#endif // __cplusplus

/* looks like ifcfg doesn't actually get all the interfaces on Win32,
   so this gets a list of interfaces by running ipconfig /all.  This
   doesn't do anything on Unix.
*/

int
ipconfig_get_all(ifcfg_config_t *cfg, int cfg_max);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // IPCONFIG_H_INCLUDED
