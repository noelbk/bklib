#include "bkwin32.h"

void
mssleep(double secs) {
    Sleep((DWORD)(secs*1000));
}
