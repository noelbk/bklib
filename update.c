/*
  update.c - check for and download updates 
  Noel Burton-Krahn
  Feb 8, 2005

  i = update(old_xml_path, new_xml_url, func, farg);

  Downloads new_xml_url and compares it to old_xml_path.  If newer,
  ask the user to install, then download and run setup.

  the xml files are like this:

  <version>
    <product></product>
    <version></version>
    <version_url></version_url>
    <install_url></install_url>
    <update_details></update_details>
  </version>

  Versions are lists of integers or strings, separated by "." or "-",
  
    1.02-03 == 1.2.3
    1.02-a  < 1.2-b

  func is called at the following steps in processing, and may modify
  the update_info_t struct.  update stops if func returns nonzero.

  UPDATE_STATE_ERROR
    an error occurred.

  UPDATE_STATE_ASK
    up->cmp_newer is true iff the new update is newer.  return 0 to download.

  UPDATE_STATE_DOWNLOAD
    new file will be saved to up->new_path.  func can change new_path.

  UPDATE_STATE_PROGRESS
    display up->prog_msg, up->prog_percent, and up->prog_remain.

  UPDATE_STATE_INSTALL
    up->new_path is complete, and will be installed.

  UPDATE_STATE_UPDATED
    up->new_path is executing.  The app should close now to allow update.

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "update.h"

#include "http_req.h"
#include "mstime.h"
#include "readable.h"
#include "vercmp.h"
#include "debug.h"
#include "proc.h"
#include "dir.h"
#include "sock.h"

typedef struct http_req_buf_t {
    update_t *update;
    char *buf;
    int len;
    int max;
} http_req_buf_t;

int
http_req_buf(http_req_t *req, char *buf, int len, void *varg) {
    http_req_buf_t *arg = (http_req_buf_t*)varg;
    update_t *up = (update_t*)arg->update;
    int err=-1;

    err = 0;
    switch( req->req_state ) {
    case HTTP_REQ_ERROR: {
	up->state = UPDATE_STATE_ERROR;
	up->func(up);
	up->error = 0;
	err = -1;
	break;
    }
    case HTTP_REQ_BODY: {
	assertb(len < (arg->max - arg->len));
	memcpy(arg->buf + arg->len, buf, len);
	arg->len += len;
	arg->buf[arg->len] = 0;
	break;
    }
    default: break;
    }
    return err;
}

typedef struct http_req_prog_t {
    update_t *update;
    FILE *fd;
    size_t len;
} http_req_prog_t;

int
http_req_prog(http_req_t *req, char *buf, int len, void *varg) {
    http_req_prog_t *arg = (http_req_prog_t*)varg;
    update_t *up = (update_t*)arg->update;
    int i, err=-1;
    char buf1[1024];

    err = 0;
    switch( req->req_state ) {
    case HTTP_REQ_ERROR: {
	up->state = UPDATE_STATE_ERROR;
	up->func(up);
	up->error = 0;
	err = -1;
	break;
    }
    case HTTP_REQ_BODY: {
	mstime_t t;
	i = fwrite(buf, 1, len, arg->fd);
	assertb(i==len);
	arg->len += len;

	t = mstime();
	if( t - up->prog_time_update > up->prog_time_interval ) {
	    up->prog_time_update = t;
	    if( req->reply_hdr->content_len > 0 ) {
		up->prog_percent = 100 * arg->len / req->reply_hdr->content_len;
	    }
	    up->prog_remain_buf[0] = 0;
	    if( up->prog_percent > 0 ) {
		up->prog_remain = (int)
		    ((t - up->prog_time_start) 
		     * (100.0 / up->prog_percent - 1));
		snprintf(up->prog_remain_buf, sizeof(up->prog_remain_buf),
			 "%d%% complete, %s remaining"
			 ,up->prog_percent
			 ,readable_duration(up->prog_remain, buf1, sizeof(buf1), 1)
			 );
	    }
	    up->state = UPDATE_STATE_PROGRESS;
	    i = up->func(up);
	    if( i ) {
		err = i;
		break;
	    }
	}
	
	break;
    }
    default: break;
    }
    return err;
}

int
update(char *old_xml_buf, update_func_t func, void *farg) {
    int i, err=-1;
    char   buf[65536];
    update_t _up, *up=&_up;
    
    http_req_buf_t new_xml_req;
    http_req_prog_t new_download_req;
    url_t url;
    struct stat st;

    memset(up, 0, sizeof(*up));
    up->error = UPDATE_ERROR_NONE;
    do {
	up->pool = pool_new();
	assertb(up->pool);
	
	up->func = func;
	up->farg = farg;

	// get old version info from disk
	up->error = UPDATE_ERROR_FILE_BAD;
	snprintf(up->error_msg, sizeof(up->error_msg),
		 "There was an error reading the version information"
		 " for your product.  Please check the website for the"
		 " latest updates.");
	up->old_xml = xml_new();
	assertb(up->old_xml);
	i = xml_parse(up->old_xml, old_xml_buf, strlen(old_xml_buf), 1);
	assertb(i>=0);
	up->old_ver = xml_find_val(up->old_xml, "/version/version");
	assertb(up->old_ver);
	up->new_xml_url = xml_find_val(up->old_xml, "/version/version_url");
	assertb(up->new_xml_url);

	// get new version info from web
	snprintf(up->error_msg, sizeof(up->error_msg),
		 "There was an error downloading the update information"
		 " for your product.  Please check again later.");
	up->error = UPDATE_ERROR_FILE_NOT_FOUND;
	memset(&new_xml_req, 0, sizeof(&new_xml_req));
	new_xml_req.buf = buf;
	new_xml_req.max = sizeof(buf);
	new_xml_req.len = 0;
	new_xml_req.update = up;
	i = http_get(up->new_xml_url, http_req_buf, &new_xml_req);
	assertb(i>=0);
	up->new_xml = xml_new();
	assertb(up->new_xml);
	up->error = UPDATE_ERROR_FILE_BAD;
	i = xml_parse(up->new_xml, buf, new_xml_req.len, 1);
	assertb(i>=0);
	up->new_ver = xml_find_val(up->new_xml, "/version/version");
	assertb(up->new_ver);
	up->new_product = xml_find_val(up->new_xml, "/version/product");
	assertb(up->new_product);
	up->new_details = xml_find_val(up->new_xml, "/version/update_details");
	assertb(up->new_details);
	up->new_url = xml_find_val(up->new_xml, "/version/install_url");
	i = url_parse(&url, up->new_url);
	assertb(i>=0);
	dir_basename(url.path, up->new_path, sizeof(up->new_path));

	// ask to update, let func decide based on user and cmp_newer
	up->error = 0;
	up->cmp_newer = vercmp(up->old_ver, up->new_ver) < 0;
	
	snprintf(up->prog_msg, sizeof(up->prog_msg),
		 "A newer version of %s is available."
		 "  Would you like to install it now?"
		 ,up->new_product);
	up->state = UPDATE_STATE_ASK;
	i = up->func(up);
	if( i ) {
	    err = i;
	    break;
	}
	
	//  download file
	snprintf(up->prog_msg, sizeof(up->prog_msg),
		 "Downloading file...");
	up->prog_time_interval = .2;
	up->prog_time_start = mstime();
	up->state = UPDATE_STATE_DOWNLOAD;
	i = up->func(up);
	if( i ) {
	    err = i;
	    break;
	}

	up->error = UPDATE_ERROR_FILE_NOT_FOUND;
	snprintf(up->error_msg, sizeof(up->error_msg),
		 "There was an error downloading the new version"
		 " of your product.  Please try again later.");
	memset(&new_download_req, 0, sizeof(new_download_req));
	new_download_req.update = up;
	new_download_req.fd     = fopen(up->new_path, "wb");
	assertb_syserr(new_download_req.fd);

	i = http_get(up->new_url, http_req_prog, &new_download_req);
	fclose(new_download_req.fd);
	new_download_req.fd = 0;
	if( i != 0 ) {
	    if( i > 0 ) {
		up->error = 0;
	    }
	    err = i;
	    break;
	}

	up->state = UPDATE_STATE_INSTALL;
	i = up->func(up);
	if( i ) {
	    err = i;
	    break;
	}

	// unpack and run update
	up->error = UPDATE_ERROR_FILE_BAD;
	snprintf(up->error_msg, sizeof(up->error_msg),
		 "There was an error installing the new version"
		 " of your product.  Please try again later.");
	if( strcasecmp(&up->new_path[strlen(up->new_path)-strlen(".tar.gz")], ".tar.gz") == 0 ) {
	    // on unix, untar and execute up->new_path/install.sh
	    snprintf(buf, sizeof(buf), "tar zxf \'%s\'", up->new_path);
	    i = proc_run(0, buf, 0);
	    assertb(i>=0);
	    up->new_path[strlen(up->new_path)-strlen(".tar.gz")] = 0;
	    i = chdir(up->new_path);
	    assertb(i>=0);
	    if( stat("INSTALL.sh", &st) == 0 ) {
		i = proc_start(0, "sh ./INSTALL.sh", 0);
	    }
	    else if( stat("install.sh", &st) == 0 ) {
		i = proc_start(0, "sh ./install.sh", 0);
	    }
	    i = chdir("..");
	}
	else {
	    // on Win32, execute the downloaded file
	    snprintf(buf, sizeof(buf), "\"./%s\"", up->new_path);
	    i = proc_start(0, buf, 0);
	}
	assertb(i>=0);
	up->error = 0;

	up->state = UPDATE_STATE_UPDATED;
	err = up->func(up);
    } while(0);
    if( up->error ) {
	up->state = UPDATE_STATE_ERROR;
	err = up->func(up);
    }
    if( up->pool ) {
	pool_delete(up->pool);
    }
    return err;
}

int
update_func_stdio(update_t *up) {
    int ret = 0;
    char buf[4096];

    switch(up->state) {
    case UPDATE_STATE_ASK: {
	if( !up->cmp_newer ) {
	    ret = 1;
	    break;
	}
	while(1) {
	    printf("A newer version of %s is available.  Would you like to install it now? (Y)es, (N)o, (D)etails: ", up->new_product);
	    fgets(buf, sizeof(buf), stdin);
	    switch(tolower(*buf)) {
	    case 'y':
		return 0;
	    case 'n':
		return -1;
	    case 'd':
		printf("\n"
		       "New Version: %s\n"
		       "Old Version: %s\n"
		       "Details:\n"
		       "%s\n"
		       "\n"
		       ,up->new_ver, up->old_ver, up->new_details
		       );
		break;
	    default:
		break;
	    }
	}
	break;
    }

    case UPDATE_STATE_PROGRESS: {
	printf("\r%s %s        ", up->prog_msg, up->prog_remain_buf);
	break;
    }
    case UPDATE_STATE_ERROR: {
	printf("Error downloading update: %s\n"
	       ,up->error_msg ? up->error_msg : "" );
	break;
    }
    default: break;
    }
    return ret;
}

