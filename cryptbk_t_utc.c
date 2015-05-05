/*
  cl -Zip -W3 -o t_gmt.exe t_gmt.c
 */

#include <stdio.h>


int
main() {
    time_t t = 0, t_local, t_gmt;

    t = 0;
    t_local    = mktime(localtime(&t));
    t_gmt      = mktime(gmtime(&t));
    
    printf("t_local-t_gmt=%d\n", t_local-t_gmt);
}
