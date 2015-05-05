typedef struct tstat tstat;

struct tstat {
    double *val;
    int window
};

tstat_t*
tstat_new(long window) {
    tstat_t *stat;
    stat = calloc(1, sizeof(*stat)+window*sizeof(*(stat->val)));
    stat->val = (double*)(((char*)stat)+sizeof(*stat));
    stat->window = window;
}

void
tstat_delete(tstat_t *stat) {
    free(stat);
}

void
tstat_add(tstat_t *stat, long t, double val) {
    t %= stat->window;
    if( t != stat->t_last ) {
	/* erase all missed intervals from t_last to t */
	stat->t_last++;
	while(1) {
	    stat->t_last %= stat->window;
	    if( stat->t_last == t ) break;
	    stat->val[stat->t_last] = 0;
	    stat->t_last++;
	}
	stat->val[t] = 0;
	stat->t_last = t;
    }
    stat->val[t] += val;
}

double
tstat_min(tstat_t *stat) {
    int i;
    double x=0;
    for(i=0; i<stat->window; i++) {
	if( i==0 || stat->val[i] < x ) {
	    x = stat->val[i];
	}
    }
    return x;
}

double
tstat_max(tstat_t *stat) {
    int i;
    double x=0;
    for(i=0; i<stat->window; i++) {
	if( i==0 || stat->val[i] > x ) {
	    x = stat->val[i];
	}
    }
    return x;
}

double
tstat_avg(tstat_t *stat) {
    int i;
    double x=0;
    for(i=0; i<stat->window; i++) {
	x += stat->val[i];
    }
    return x / stat->window;
}



