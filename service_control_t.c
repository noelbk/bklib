#include <string.h>
#include "service.h"
#include "debug.h"
#include "debug_stdio.h"
#include "mstime.h"
#include "dir.h"

char *exe;

int
main(int argc, char **argv) {
    char exepath[4096];
    getcwd(exepath, sizeof(exepath));
    strncat(exepath, "/service_t.exe", sizeof(exepath));

    debug_init(DEBUG_INFO, 0, 0);
    service_install(exepath, exepath, SERVICE_START_AUTO);
    service_control(exepath, SERVICE_CTRL_START);
    // sleep for a while
    mssleep(10);
    service_control(exepath, SERVICE_CTRL_STOP);
    service_control(exepath, SERVICE_CTRL_UNINSTALL);
    return 0;
}
