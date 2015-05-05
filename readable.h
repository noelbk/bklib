#ifndef READABLE_H_INCLUDED
#define READABLE_H_INCLUDED

#include "mstime.h"

#ifdef __cplusplus
extern "C" {
#endif

char *
readable_metric(double bytes, char *name, char *buf, int buflen);

char *
readable_bytes(double bytes, char *buf, int buflen);

char *
readable_date(mstime_t t, char *buf, int buflen);

char *
readable_duration(mstime_t t, char *buf, int buflen, int longfmt);

#ifdef __cplusplus
}
#endif

#endif // READABLE_H_INCLUDED
