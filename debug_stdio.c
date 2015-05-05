#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "config.h"
#include "dir.h"
#include "debug.h"
#include "debug_stdio.h"
#include "mstime.h"
#include "rand.h"

int
debug_func_stdio(const char *buf, void *arg) {
    fwrite(buf, strlen(buf), 1, (FILE*)arg);
    fflush((FILE*)arg);
    return 1;
}


typedef struct {
    char *log_dir;
    char *log_prefix;
    int   files_max;
    int   size_max;
    FILE *log_file;
    FILE *tee_file;
    int   cur_size;
} debug_func_log_t;


// log_prefix is something like log/foo.  logs will be made log/foo,
// log/foo.1, ..., log/foo.files_max
void*
debug_func_log_init(const char *log_prefix, int files_max, int size_max, FILE *tee_file) {
    debug_func_log_t *f;
    do {
	f = calloc(1, sizeof(*f));
	assertb(f);
	if( log_prefix ) {
	    f->log_dir = strdup(log_prefix);
	    assertb(f->log_dir);
	    f->log_prefix = dir_filename(f->log_dir, 1);
	    f->files_max = files_max;
	    f->size_max = size_max;
	    f->tee_file = tee_file;
	}
    } while(0);
    return f;
}

int
debug_func_log(const char *buf, void *arg) {
    debug_func_log_t *f = (debug_func_log_t *)arg;
    int i, n, count;
    debug_func_t old_func;
    void         *old_arg;

    if( !arg ) return debug_func_stdio(buf, stderr);


    if( f->tee_file ) {	
	debug_func_stdio(buf, f->tee_file);
    }

    if( !f->log_dir ) {
	return 1;
    }
    
    // don't infinite loop in asserts
    old_func = debug_func;
    debug_func = debug_func_stdio;
    old_arg = debug_arg;
    debug_arg = stderr;

    do {
	if( !f->log_file ) {
	    char *files[2048];
	    char files_buf[16384];
	    char path_buf[4096];
	    char date_buf[128];
	    time_t t;
	    struct tm *tm;
	    mstime_t mt;
	    
	    /* rotate logs and open a new file */
	    i = dir_mkpath(f->log_dir, 1);
	    assertb(i>=0);
	    n = dir_sorted(f->log_dir, files, sizeof(files)/sizeof(*files), 
			   files_buf, sizeof(files_buf));
	    assertb(n>=0);
	    count = 0;
	    for(i=n-1; i>=0; i--) {
		if( strncmp(files[i], f->log_prefix, strlen(f->log_prefix)) != 0 ) {
		    continue;
		}
		count++;
		if( count >= f->files_max ) {
		    snprintf(path_buf, sizeof(path_buf), "%s/%s", f->log_dir, files[i]);
		    unlink(path_buf);
		}
	    }

	    // open the new log file
	    mt = mstime();
	    t = (unsigned long)mt;
	    tm = localtime(&t);
	    i = strftime(date_buf, sizeof(date_buf), "%Y%m%d-%H%M%S", tm);
	    assertb(i>0);

	    i = (int)((mt-(int)mt)*10000);
	    i = snprintf(path_buf, sizeof(path_buf), "%s/%s-%s-%04d.log", 
			 f->log_dir, f->log_prefix, date_buf, i);
	    assertb(i>0);
	    f->log_file = fopen(path_buf, "w");
	    f->cur_size = 0;
	    assertb(f->log_file);
	    fprintf(f->log_file, "new log %s\n", path_buf); 
	}

	i = strlen(buf);
	fwrite(buf, 1, i, f->log_file);
	fflush(f->log_file);
	f->cur_size += i;

	/* close current file when it gets full */
	if( i>0 && buf[i-1] == '\n' && f->cur_size >= f->size_max ) {
	    fclose(f->log_file);
	    f->log_file = 0;
	    f->cur_size = 0;
	}
    } while(0);

    // don't infinite loop in asserts
    debug_func = old_func;
    debug_arg = old_arg;

    return 1;
}
