#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

struct state_table_s {
    int      state;
    double   retry_interval; /* time between retries */
    int      retry_max;      /* max numberr of tries */
    int      timeout_next;   /* state to go to after max_retries */
    void     *arg;
};
typedef struct state_table_s state_table_t;

struct state_s {
    state_table_t *table;
    state_table_t *current;

    double     retry_timeout;
    int        retry_count;
} state;
typedef struct state_s state_t;

#define STATE_TABLE_END {0,0,0,0,0}

enum {
    STATE_POLL_NOOP=0,
    STATE_POLL_RETRY,
    STATE_POLL_TIMEOUT
} state_poll_ret;

int
state_init(state_t *state, state_table_t *table);

int
state_get(state_t *state);

int
state_set(state_t *state, int next, double now);

int
state_poll(state_t *state, double now, int timeout_next);

int
state_timeout_next(state_t *state, double now);

#endif /* STATE_H_INCLUDED */
