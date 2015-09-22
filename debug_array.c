#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "debug.h"
#include "debug_array.h"

int
debug_func_array(const char *buf, void *arg) {
    array_t *arr = (array_t *)arg;
    int i = strlen(buf);
    strncpy(array_add(arr, i), buf, i);
    return 1;
}

