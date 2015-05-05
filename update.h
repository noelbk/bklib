/*
  update.h - check for and download updates 
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

#ifndef UPDATE_H_INCLUDED
#define UPDATE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../bkxdr/bkxml.h"
#include "mstime.h"
#include "pool.h"

typedef enum update_state_t {
    UPDATE_STATE_INIT
    ,UPDATE_STATE_ERROR
    ,UPDATE_STATE_ASK
    ,UPDATE_STATE_DOWNLOAD
    ,UPDATE_STATE_PROGRESS
    ,UPDATE_STATE_INSTALL
    ,UPDATE_STATE_UPDATED
} update_state_t;

typedef enum update_error_t {
    UPDATE_ERROR_NONE             = 0
    ,UPDATE_ERROR_FILE_NOT_FOUND  = -1
    ,UPDATE_ERROR_FILE_BAD        = -2
} update_error_t;

struct update_t;
typedef struct update_t update_t;

typedef int (*update_func_t)(update_t *update);
struct update_t {
    char                *old_xml_path;
    char                *new_xml_url;

    update_state_t      state;
    update_error_t      error;
    char                error_msg[4096];

    xml_t               *old_xml;
    char                *old_ver;

    xml_t               *new_xml;
    char                *new_ver;
    char                *new_product;
    int                  cmp_newer;
    char                *new_details;
    char                *new_url;
    char                 new_path[2048];

    mstime_t            prog_time_start;
    mstime_t            prog_time_update;
    mstime_t            prog_time_interval;
    int                 prog_percent;
    int                 prog_remain;
    char                prog_remain_buf[1024];
    char                prog_msg[4096];

    update_func_t    func;
    void             *farg;
    pool_t           *pool;
};

int
update(char *old_xml_buf, update_func_t func, void *farg);

int
update_func_stdio(update_t *up);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // UPDATE_H_INCLUDED
