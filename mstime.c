#include "configbk.h"

#include <time.h>
#include <sys/timeb.h>
#include "mstime.h"

mstime_t
mstime() {
    struct timeb tb;

    tb.dstflag = -1;
    ftime(&tb);
    return (mstime_t)tb.time + 1.0e-3 * tb.millitm;
}
char *
mstime_fmt(mstime_t t, char *buf, int len) {
    return mstime_fmt2(t, 0, buf, len);
}

char *
mstime_fmt2(mstime_t t, const char *fmt, char *buf, int len) {
    struct tm tm;
    time_t s = (time_t)t;
    tm = *localtime(&s);
    if( !fmt ) {
	fmt = "%Y-%m-%d-%H:%M:%S";
    }
    strftime(buf, len, fmt, &tm);
    return buf;
}


mstime_t
mstime_gmt_offset(mstime_t mst) {
    time_t t = (time_t)mst;

#if OS & OS_WIN32
    mst = mktime(localtime(&t)) - mktime(gmtime(&t));
#endif
#if OS & OS_UNIX
    {
	struct tm tm;
	tm = *localtime(&t);
	mst = tm.tm_gmtoff;
    }
#endif

    return mst;
}

#if OS == OS_UNIX
#include "mstime_unix.c"
#endif

#if OS == OS_WIN32
#include "mstime_win32.c"
#endif


