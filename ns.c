/* ns.c - copy and use linux namespaces.  See USAGE below

   Noel Burton-Krahn <noel@burton-krahn.com>
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *VERSION = 
    "ns 1.0\n"
    ;

char *USAGE =
    "Usage\n"
    "  ns add [options] [-proc pid] name\n"
    "  ns exec [options] [-proc] name argv ...\n"
    "  ns delete name\n"
    "  ns list\n"
    "  ns list name\n"
    "\n"
    "Actions\n"
    "\n"
    "  add     Creates a new namespace directory in /var/run/ns/<name> and saves\n"
    "          all selected namespaces (except mnt and user)\n"
    "\n"
    "  exec    Uses namespaces in /var/run/ns/<name> or /proc/<name>/ns and\n"
    "          execs the rest of argv\n"
    "\n"
    "  delete  Removes the /var/run/ns/<name> directory\n"
    "\n"
    "  list    Without a name argument lists all namespaces created by add.\n"
    "          With a name argument, it exits 0 if the namespace exists\n"
    "\n"
    "  help    This message\n"
    "\n"
    "Options\n"
    "\n"
    "  set of namespaces to use, defaults to all\n"
    "  -net, -uts\n"
    "  -ipc, -pid\n"
    "  -mnt        \n"
    "\n"
    "  -proc       name is a pid, use namespaces in /proc/<name>/ns\n"
    "\n"
    "  name        by default, namespaces are from /var/run/ns/<name>/ns\n"
    "\n"
    "  --          end of arguments (in case argv starts with a dash)\n"
    "\n"
    "Description\n"
    "\n"
    "This uses unshare(2) to create new namespaces, bind mounts them from\n"
    "/proc/self/ns to /var/run/ns/<name> and uses setns(2) to rejoin them.\n"
    "\n"
    "Examples\n"
    "\n"
    "    Copy all new namespaces to ns1\n"
    "\n"
    "      ns add ns1\n"
    "\n"
    "    Copy only net and uts namespaces to ns1\n"
    "\n"
    "      ns add -net -uts ns1\n"
    "\n"
    "    Copy all namespaces from pid 1234 to ns1\n"
    "\n"
    "      ns add -proc 1234 ns2\n"
    "\n"
    "    Run ifconfig in ns1's namespaces\n"
    "\n"
    "      ns exec ns1 ifconfig\n"
    "\n"
    "    Run ifconfig in pid 1234's namespaces\n"
    "\n"
    "      ns exec -proc 1234 ifconfig\n"
    "\n"
    "    List all created namespaces\n"
    "\n"
    "      ns list\n"
    "\n"
    "    Return true if ns1 exists\n"
    "\n"
    "      ns list ns1\n"
    "\n"
    "    Delete the ns1 directory\n"
    "\n"
    "      ns delete ns1\n"
    "\n"
    "Bugs\n"
    "\n"
    "The mnt namespace cannot be bind mounted, so you must use -proc\n"
    "instead of a name to share one.\n"
    "\n"
    "Creating a new mnt namespace duplicates all exist mounts, which can\n"
    "hold filesystems open even after the they're unmounted in the root\n"
    "namespace\n"
    "\n"
    "Author\n"
    "\n"
    "  Noel Burton-Krahn <noel@burton-krahn.com>\n"
    "\n"
    ;

void
debug_print(char *fmt, ...) {
    if( fmt ) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
    }
}

void
usage(char *fmt, ...) {
    if( fmt ) {
	fprintf(stderr, "ERROR: ");
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n\n");
    }
    fprintf(stderr, "%s", USAGE);
    exit(1);
}

void
die(char *fmt, ...) {
    if( fmt ) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n\n");
    }
    exit(1);
}

#define assertb(cond, msg) \
    if( !(cond) ) { \
	debug_print("ERROR %s:%d: " __FILE__, __LINE__); \
	debug_print msg; \
	exit(1); \
    }

int
mkpath(const char *path, mode_t mode) {
    char *tmppath = 0;
    int r, err=-1;
    char *p, *q;
    do {
	tmppath = strdup(path);
	assertb(tmppath, ("strdup(%s): %s", path, strerror(errno)));
	for(p=tmppath; p && *p; p=q) {
	    q = strchr(p+1, '/');
	    if( q ) { *q = 0; }
	    r = mkdir(tmppath, mode);
	    if( r && errno != EEXIST ) {
		return -1;
	    }
	    if( q ) { *q = '/'; }
	}
	err = 0;
    } while(0);
    if( tmppath ) {
	free(tmppath);
    }
    return err;
}

typedef struct ns_s {
    char *name;
    int clone_flag;
} ns_t;

ns_t ns_avail[] = {
    { "ipc",   CLONE_NEWIPC },
    { "net",   CLONE_NEWNET },
    { "pid",   CLONE_NEWPID },
    { "uts",   CLONE_NEWUTS },
    { "mnt",   CLONE_NEWNS  },
};

const int ns_avail_len = sizeof(ns_avail)/sizeof(*ns_avail);

int ns_used = 0;

ns_t*
get_ns_used(int i) {
    assertb(i<ns_avail_len, ("i=%d, ns_used=%d", i, ns_used));
    return (!ns_used || (ns_used & (1<<i))) ? ns_avail+i : NULL;
}

void
set_ns_used(int i) {
    ns_used |= (1<<i);
}

int
main(int argc, char **argv) {
    int optidx;
    int i, r, nsi, fd;
    char *name_dir = "/var/run/ns";
    int opt_add = 0;
    int opt_delete = 0;
    int opt_exec = 0;
    int opt_list = 0;
    int opt_name_is_pid = 0;
    char *opt_clone_pid = 0;
    char dir[PATH_MAX+1];
    char src_dir[PATH_MAX+1];
    char path[PATH_MAX+1];
    char src_path[PATH_MAX+1];
    char *name = 0;
    struct stat st;

    int ns_dir_exists = 0;
    int got_opt = 0;
    char *opt;

    /* action argument */
    optidx = 1;
    if( argc <= optidx ) {
	usage("missing action argument");
    }
    opt = argv[optidx++];
    while(*opt && *opt == '-') {
	opt++;
    }
    if( strcmp(opt, "add")==0 ) {
	opt_add = 1;
    }
    else if( strcmp(opt, "delete")==0 || strcmp(opt, "del")==0 || strcmp(opt, "rm")==0  ) {
	opt_delete = 1;
    }
    else if( strcmp(opt, "exec")==0 ) {
	opt_exec = 1;
    }
    else if( strcmp(opt, "list")==0 || strcmp(opt, "ls")==0  ) {
	opt_list = 1;
    }
    else if( strcmp(opt, "help")==0 || strcmp(opt, "h")==0 ) {
	printf("%s", USAGE);
	exit(0);
    }
    else if( strcmp(opt, "version")==0 ) {
	printf("%s", VERSION);
	exit(0);
    }
    else {
	usage("unrecognized action argument: %s", opt);
    }

    for(; optidx<argc; optidx++) {
	if( argv[optidx][0] != '-' ) {
	    break;
	}
	opt = argv[optidx]+1;
	if( *opt == '-' ) {
	    opt++;
	}
	if( !*opt ) {
	    // break on --
	    optidx++;
	    break;
	}

	got_opt = 1;
	if( strcmp(opt, "proc")==0 ) {
	    opt_name_is_pid = 1;
	}
	else {
	    /* look for -ns.name */
	    got_opt = 0;
	    for(i=0; i<ns_avail_len; i++) {
		if( strcmp(opt, ns_avail[i].name) == 0 ) {
		    set_ns_used(i);
		    got_opt = 1;
		    break;
		}
	    }
	}
	if( !got_opt ) {
	    usage("unrecognized option: %s", argv[optidx]);
	}
    }
    if( optidx < argc ) {
	name = argv[optidx++];
	if( opt_name_is_pid ) {
	    snprintf(dir, sizeof(dir), "/proc/%s/ns", name);
	}
	else {
	    snprintf(dir, sizeof(dir), "%s/%s", name_dir, name);
	}
	r = stat(dir, &st);
	ns_dir_exists = r == 0;
    }

    /* add namespace */
    got_opt = 0;
    if( opt_add ) {
	got_opt = 1;

	if( opt_name_is_pid ) {
	    if( !name ) {
		usage("missing pid argument");
	    }
	    opt_clone_pid = name;
 	    if( optidx >= argc )  {
		usage("missing name argument");
	    }
	    name = argv[optidx++];	
	    snprintf(src_dir, sizeof(src_dir), "/proc/%s/ns", opt_clone_pid);
	    snprintf(dir, sizeof(dir), "%s/%s", name_dir, name);
	    r = stat(dir, &st);
	    ns_dir_exists = r == 0;
	}
	else {
	    snprintf(src_dir, sizeof(src_dir), "/proc/self/ns");
	}

	if( optidx < argc ) {
	    usage("-add doesn't take extra arguments like %s", argv[optidx]);
	}
	if( !name ) {
	    usage("no netspace name specified");
	}
	r = mkpath(dir, 0755);
 	assertb(!r, ("mkpath(%s): %s", dir, strerror(errno)));
	r = mount("", dir, "none", MS_REC|MS_SHARED, NULL);
	if( r != 0 && errno == EINVAL ) {
	    // retry with a bind mount if necessary.  See iproute2-3.12.0/ip/ipnetns.c:netns_add()
	    r = mount(dir, dir, "none", MS_BIND, NULL);
	    assertb(!r, ("mount(%s, %s, none, MS_BIND, NULL): %s", dir, dir, strerror(errno)));
	    r = mount("", dir, "none", MS_REC|MS_SHARED, NULL);
	}
 	assertb(!r, ("mount('', %s, none, MS_REC|MS_SHARED, NULL): %s", dir, strerror(errno)));

	for(nsi=0; nsi<ns_avail_len; nsi++) {
	    ns_t *ns = get_ns_used(nsi);
	    if( !ns ) { continue; }
	    if( !ns->clone_flag ) {
		continue;
	    }
	    snprintf(path, sizeof(path), "%s/%s", dir, ns->name);
	    fd = open(path, O_RDONLY|O_CREAT|O_EXCL, 0644);
	    assertb(fd>=0, ("open(%s, O_CREAT): %s", path, strerror(errno)));
	    close(fd);
	    snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, ns->name);
	    if( !opt_clone_pid )  {
		unshare(ns->clone_flag);
	    }
	    r = mount(src_path, path, "none", MS_BIND, NULL);
	    if( r != 0 && errno == EINVAL && strcmp(ns->name, "mnt")==0 ) {
		//debug_print("mnt namespace cannot be bind mounted.  Use -proc pid to share a mnt namespace with a running process");
		unlink(path);
		r = 0;
	    }
	    assertb(!r, ("mount(%s, %s, none, MS_BIND, NULL): %s", src_path, path, strerror(errno)));
	}
    }
    
    /* exec in namespace */
    if( opt_exec ) {
	got_opt = 1;
	if( optidx >= argc ) {
	    usage("-exec requires command and arguments to run");
	}
	if( !name ) {
	    usage("no netspace name specified");
	}
	if( !ns_dir_exists ) {
	    die("namespace does not exist: %s", name);
	}
	for(nsi=0; nsi < ns_avail_len; nsi++) {
	    ns_t *ns = get_ns_used(nsi);
	    if( !ns ) { continue; }
	    snprintf(path, sizeof(path), "%s/%s", dir, ns->name);
	    fd = open(path, O_RDONLY);
	    if( fd >= 0 ) {
		assertb(fd>=0, ("open(%s): %s", path, strerror(errno)));
		r = setns(fd, ns->clone_flag);
		assertb(!r, ("setns(%s, %d): %s", path, ns->clone_flag, strerror(errno)));
		close(fd);
	    }
	}
	execvp(argv[optidx], argv+optidx);
	assertb(0, ("exec(%s): %s", argv[optidx], strerror(errno)));
    }

    /* delete namespace */
    if( opt_delete ) {
	got_opt = 1;
	if( optidx < argc ) {
	    usage("-delete doesn't take extra arguments like %s", argv[optidx]);
	}
	if( !name ) {
	    usage("no netspace name specified");
	}
	if( opt_name_is_pid ) {
	    die("name cannot be a pid for delete");
	}
	if( !ns_dir_exists ) {
	    die("namespace does not exist: %s", name);
	}

	/* remove all selected namespace mounts */
	for(nsi=0; nsi < ns_avail_len; nsi++) {
	    ns_t *ns = get_ns_used(nsi);
	    if( !ns ) { continue; }
	    snprintf(path, sizeof(path), "%s/%s", dir, ns->name);
	    r = umount(path);
	    //assertb(!r || errno==ENOENT, ("umount(%s): %s", path, strerror(errno)));
	    r = unlink(path);
	    assertb(!r || errno==ENOENT, ("unlink(%s): %s", path, strerror(errno)));
	}
	/* unmount and remove ns dir */
	r = umount(dir);
	//assertb(!r, ("umount(%s): %s", dir, strerror(errno)));
	r = rmdir(dir);
	assertb(!r, ("rmdir(%s): %s", dir, strerror(errno)));
    }

    /* list namespaces */
    if( opt_list ) {
	got_opt = 1;
	
	/* if name is specified, just test if it exists */
	if( name ) {
	    if( !ns_dir_exists ) {
		exit(1);
	    }
	    printf("%s\n", dir);
	}
	else {
	    DIR *dir;
	    struct dirent *dent;

	    dir = opendir(name_dir);
	    assertb(dir, ("opendir(%s): %s", name_dir, strerror(errno)));
	    while(1) {
		int got_ns = 0;
		dent = readdir(dir);
		if( !dent ) {
		    break;
		}
		name = dent->d_name;
		if( strcmp(name, ".")==0 || strcmp(name, "..")==0 ) {
		    continue;
		}
		printf("%s", name);
		for(nsi=0; nsi < ns_avail_len; nsi++) {
		    ns_t *ns = get_ns_used(nsi);
		    if( !ns ) { continue; }

		    snprintf(path, sizeof(path), "%s/%s/%s", name_dir, name, ns->name);
		    r = stat(path, &st);
		    if( r == 0 ) {
			if( !got_ns ) {
			    got_ns = 1;
			    printf("\t");
			}
			else {
			    printf(",");
			}
			printf("%s", ns->name);
		    }
		}
		printf("\n");
	    }
	}
    }

    if( !got_opt ) {
	usage("no action specified.  use one of -add, -exec, -delete, or -list");
    }

    return 0;
}	
