#include <stdio.h>
#include <time.h>

#include "config.h"
#include "readable.h"

char *
readable_bytes(double bytes, char *buf, int buflen) {
    return readable_metric(bytes, "b", buf, buflen);
}

char *
readable_metric(double bytes, char *name, char *buf, int buflen) {
    struct {
	double max;
	char *label;
    } units[] = {
	{1e12, "T"},
	{1e9, "G"},
	{1e6, "M"},
	{1e3, "k"},
	{1, ""},
	{1e-3, "m"},
	{1e-6, "u"},
	{1e-9, "n"},
	{0, 0},
    };
    double abs_bytes = bytes < 0 ? -bytes : bytes;
    int i;

    if( bytes == 0 ) {
	snprintf(buf, buflen, "0%s", name);
    }
    else {
	for(i=0; units[i].label; i++) {
	    if( abs_bytes < units[i].max && units[i+1].label ) {
		continue;
	    }
	    snprintf(buf, buflen, "%.3g%s%s", 
		     bytes / units[i].max, units[i].label, name);
	    return buf;
	}
    }
    return buf;
}

char*
readable_date(mstime_t mst, char *buf, int len) {
    struct tm tm;
    time_t t = (time_t)mst;
    tm = *localtime(&t);
    if( !tm.tm_hour && !tm.tm_min && !tm.tm_sec ) {
	strftime(buf, len, "%b %d %Y", &tm);
    }
    else {
	strftime(buf, len, "%b %d %Y %I:%M:%S%p", &tm);
    }

    return buf;
}

char*
readable_duration(mstime_t dur, char *buf, int len, int longfmt) {
    char *orig = buf;
    int started = 0;
    struct {
	int unit;
	char *label;
    } units[] = {
	{1, "sec"}
	,{60, "min"}
	,{60, "hour"}
	,{24, "day"}
	,{0, 0}
    };
    int i, j;
    mstime_t unit;
    int val;
    
    *buf = 0;
    
    unit = 1;
    for(i=0; units[i].unit > 0; i++ ) {
	unit *= units[i].unit;
    }

    // make sure all seconds are 00:00sec
    if( !longfmt ) {
	if( dur < 60 ) {
	    j = snprintf(buf, len, "00:");
	    buf += j;
	    len -= j;
	}
	if( dur < 1 ) {
	    j = snprintf(buf, len, "00");
	    buf += j;
	    len -= j;
	}
    }

    for(i--; i>=0; i--) {
	if( dur >= unit ) {
	    val = (int)(dur/unit);
	    if( longfmt ) {
		j = snprintf(buf, len, "%s%d %s%s"
			     ,(started ? " " : "")
			     ,val
			     ,units[i].label
			     ,(val==1 ? "" : "s")
			     );
	    }
	    else {
		j = snprintf(buf, len, "%s%02d"
			     ,(started ? ":" : "")
			     ,val
			     );
	    }
	    if( j <= 0 ) break;
	    buf += j;
	    len -= j;

	    dur -= val * unit;
	    started = 1;
	}
	else if ( started ) {
	    if( longfmt ) {
		j = snprintf(buf, len, " 0 %ss", units[i].label);
	    }
	    else {
		j = snprintf(buf, len, "%s00"
			     ,(started ? ":" : "")
			     );
	    }
	    if( j <= 0 ) break;
	    buf += j;
	    len -= j;
	}
	unit /= units[i].unit;
    }
    return orig;

}
