#ifndef TSTAT_H_INCLUDED
#define TSTAT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct tstat tstat_t;

tstat_t*
tstat_new(long window);

void
tstat_delete(tstat_t *stat);

void
tstat_add(tstat_t *stat, long t, double val);

double
tstat_min(tstat_t *stat);

double
tstat_max(tstat_t *stat);

double
tstat_avg(tstat_t *stat);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // TSTAT_H_INCLUDED
