/*
  cl -Zip -W3 -o t_gmt.exe t_gmt.c
 */

#include <stdio.h>
#include <time.h>
#include "configbk.h"
#include "mstime.h"

int
main() {
    time_t t = 0, t_local, t_gmt;
    struct tm tm_local, tm_gmt;
    char buf[4096];

    time(&t);
    tm_local = *localtime(&t);
    t_local  = mktime(&tm_local);
    tm_gmt   = *gmtime(&t);
    t_gmt    = mktime(&tm_gmt);

    printf("t=%lu t_local-t_gmt=%ld timezone=%ld\n", 
	   t, 
	   (t_local-t_gmt)/3600, 
	   timezone/3600);

    strftime(buf, sizeof(buf), "%z", localtime(&t));
    printf("strftime(%%z)=%s\n", buf);

#if OS & OS_UNIX
    printf("tm_local.tm_gmtoff=%ld\n", tm_local.tm_gmtoff/3600);
#endif

    printf("mstime_gmt_offset=%ld\n", (long)mstime_gmt_offset(t)/3600);
    
    return 0;
}
