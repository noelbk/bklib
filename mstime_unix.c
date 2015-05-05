#include <unistd.h>
#include <math.h>
#include "sock.h"

void
mssleep(double secs) {
    struct timeval tv;
    double d;
    tv.tv_sec = (long)secs;
    tv.tv_usec = (long)(modf(secs, &d) * 1000000);
    select(0, 0, 0, 0, &tv);
}
