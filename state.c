#include "state.h"
#include "warn.h"
#include <string.h>

#define STATE_DEBUG __FILE__

int
state_init(state_t *state, state_table_t *table) {
    memset(state, 0, sizeof(*state));
    state->table = table;
    state->current = table;
    return 0;
}


int
state_poll(state_t *state, double now, int timeout_next) {
    if( now < state->retry_timeout ) {
	return STATE_POLL_NOOP;
    }
    if( state->retry_count <= 0 ) {
	if( timeout_next ) {
	    state_timeout_next(state, now);
	}
	else {
	    return STATE_POLL_TIMEOUT;
	}
    }
    state->retry_count--;
    state->retry_timeout = now + state->current->retry_interval;
    return STATE_POLL_RETRY;
}

int
state_get(state_t *state) {
    return state->current ? state->current->state : 0;
}

int
state_set(state_t *state, int new_state, double now) {
    int i;
    state_table_t *p;
    
    for(i=0, p=state->table; p->retry_max; i++, p++) {
	if( p->state != new_state ) continue;
	state->current = p;
	state->retry_timeout = 0;
	state->retry_count = p->retry_max;
	return 0;
    }
    debug(DEBUG_ERROR, ("state_set: undefined state_new=%d\n", new_state));
    return -1;
}

int
state_timeout_next(state_t *st, double now) {
    return state_set(st, st->current->timeout_next, now);
}
