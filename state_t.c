#include <stdio.h>
#include "state.h"
#include "mstime.h"

int
main() {
    state_table_t si[] = {
	{0, 1.0, 3, 1, 0},
	{1, 1.0, 3, 2, 0},
	{2, 1.0, 3, 0, 0},
	STATE_TABLE_END
    };
    state_t st = { si };

    int i;
    mstime_t t0, t;
    

    t0 = mstime();
    state_set(&st, 0, 0);
    while(1) {
	t = mstime()-t0;
	i = state_poll(&st, t, 1);
	if( i == STATE_POLL_NOOP ) {
	    //sleep(st->retry_timeout-t);
	    continue;
	}
	else if( i == STATE_POLL_RETRY ) {
	    printf("STATE_POLL_RETRY state=%d t=%g\n",
		   st.current->state, t);
	}
	else if( i == STATE_POLL_TIMEOUT ) {
	    i = (int)st.current->arg;
	    printf("STATE_POLL_TIMEOUT state=%d t=%g next=%d\n",
		   st.current->state, t, st.current->timeout_next);
	    state_timeout_next(&st, t);
	}
    }
}

