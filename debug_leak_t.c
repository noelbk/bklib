#include <stdint.h>

#include "debug.h"
#include "debug_leak.h"

int main() {
    intptr_t ptr = 1;
    int i;

    debug_init(DEBUG_INFO, 0, 0);

    debug_leak_init();
   
    ptr++;
    debug(DEBUG_INFO, ("ptr=%p\n", (void*)ptr));
    debug_leak_create((void*)ptr);
    debug_leak_stack((void*)ptr);
    debug_leak_delete((void*)ptr);
    
    ptr++;
    debug(DEBUG_INFO, ("ptr=%p\n", (void*)ptr));
    debug_leak_create((void*)ptr);
    debug_leak_delete((void*)ptr);

    ptr++;
    debug(DEBUG_INFO, ("ptr=%p\n", (void*)ptr));
    debug_leak_create((void*)ptr);
    debug_leak_stack((void*)ptr);
    //debug_leak_delete((void*)ptr);

    ptr++;
    debug(DEBUG_INFO, ("ptr=%p\n", (void*)ptr));
    //debug_leak_create((void*)ptr);
    debug_leak_stack((void*)ptr);
    debug_leak_delete((void*)ptr);

    for(i=0; i<5; i++) {
	if( i >= 0 ) debug_leak_create((void*)ptr++);
	if( i >= 1 ) debug_leak_create((void*)ptr++);
	if( i >= 2 ) debug_leak_create((void*)ptr++);
	debug_leak_by_line();
    }

    debug_leak_dump();
    debug_leak_fini();

    return 0;
}
