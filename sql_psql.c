#include "sqlbk.h"
#include "pq.h"

int
sql_quote(char *dst, int len, char *src) {
    char *orig=dst, *end = dst+len-1;
    for(; *src && dst<end ; src++) {
	if( *src == '\'' ) {
	    *(dst++) = '\'';
	    assertb(dst<end);
	    *(dst++) = '\'';
	}
	else {
	    *(dst++) = *src;
	}
    }
    if( dst < end ) {
	*dst = 0;
    }
    return dst<end ? dst-orig : -1;
}


int
sql_snprintf_v(char *dst, int len, char *fmt, va_list va) {
    int i;
    char *orig = dst;

    len--;
    for(; *fmt && len>0; fmt++) {
	if( *fmt != '%' ) {
	    *dst = *fmt;
	    dst++;	    
	    len--;
	    continue;
	}

	fmt++;
	switch(*fmt) {
	case 'q':
	    i = sql_quote(dst, len, va_arg(va, char*));
	    break;

	case 's':
	    i = snprintf(dst, len, "%s", va_arg(va, char*));
	    break;

	case 'f':
	    i = snprintf(dst, len, "%f", va_arg(va, float));
	    break;

	case 'd':
	    i = snprintf(dst, len, "%d", va_arg(va, int));
	    break;
	}

	assertb(i>=0);
	dst += i;
	len -= i;
	
	break;
    }
    if( len > 0 ) {
	*dst = 0;
    }
    
    return len>0 ? dst - orig : -1;
}



int
sql_pg_exec(sql_t *db, 
	    sql_exec_func_t func, void *farg, 
	    char *fmt, ...) {
    int i;
    va_list va;
    PGconn *conn = (PGconn *)db->private;
    PGresult *result=0;
    char *vals=0, *cols=0, *p;
    int row, col, nrows, ncols;

    va_start(va, fmt);

    do {
	assertb(conn);

	i = sql_snprintf_v(buf, sizeof(buf), fmt, va);
	assertb(i>=0);
	
	assertb(conn);
	result = PQexec(conn, buf);
	assetrb(result);
	
	switch(PQresultStatus(result)) {
	case PGRES_COMMAND_OK:
	    break;

	case PGRES_TUPLES_OK:
	    ncols = PQnfields(result);
	    assertb(ncols>0);

	    nrows = PQntuples(result);
	    assertb(ncols>0);

	    cols = (char*)calloc(1, ncols*sizeof(char*));
	    vals = (char*)calloc(1, ncols*sizeof(char*));;
	    for(col=0; col<ncols; col++) {
		p = PQfname(result, col);
		cols[col] = strdup(p);
		assertb(cols[col]);
	    }
	    for(row=0; row<nrows; row++) {
		for(col=0; col<ncols; col++) {
		    if( PQgetisnull(result, row, col) ) {
			continue;
		    }
		    p = PQgetvalue(result, row, col);
		    assertb(p);
		    
		    switch( PQfformat(result, col) ) {
		    case 0:
			vals[col] = strdup(p);
			break;

		    case 1:
			vals[col] = memdup(p, PQgetlength(result, row, col));
			break;

		    default:
			assertb(0);
			break;
		    }
		    assertb(vals[col]);
		}

		i = func(farg, ncols, vals, cols);

		for(col=0; col<ncols; col++) {
		    if( vals[col] ) {
			free(vals[col]);
			vals[col] = 0;
		    }
		}
		
		if( i != 0 ) {
		    break;
		}
	    }
	    break;

	default:
	    db->errstr = PQresultErrorMessage(result);
	    
	    break;
	}
    } while(0);
    va_end(va);

    if( vals ) {
	for(col=0; col<ncols; col++) {
	    if( vals[col] ) {
		free( vals[col]);
	    }
	}
	free(vals);
    }
    if( cols ) {
	for(col=0; col<ncols; col++) {
	    if( cols[col] ) {
		free(col[col]);
	    }
	}
	free(cols);
    }
    if( result ) {
	PQclear(result);
    }
}

sql_t*
sql_pg_new(char *connstr) {
    sql->exec = sql_pg_exec;
}

