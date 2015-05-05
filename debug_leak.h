#ifndef DEBUG_LEAK_H_INCLUDED
#define DEBUG_LEAK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef DEBUG_LEAK_IGNORE

#define debug_leak_init()      debug_leak_init_f()
#define debug_leak_fini()      debug_leak_fini_f()
#define debug_leak_create(ptr) debug_leak_create_f(ptr, __FILE__, __LINE__)
#define debug_leak_stack(ptr)  debug_leak_stack_f(ptr, __FILE__, __LINE__)
#define debug_leak_delete(ptr) debug_leak_delete_f(ptr, __FILE__, __LINE__)
#define debug_leak_dump()      debug_leak_dump_f()
#define debug_leak_create_assertb(ptr) assertb(ptr); debug_leak_create_f(ptr, __FILE__, __LINE__)
#define debug_leak_by_line()   debug_leak_by_line_f()


#else

#define debug_leak_init()      
#define debug_leak_fini()      
#define debug_leak_create(ptr) 
#define debug_leak_stack(ptr)  
#define debug_leak_delete(ptr) 
#define debug_leak_dump()      
#define debug_leak_create_assertb(ptr) assertb(ptr)
#define debug_leak_by_line()

#endif

int debug_leak_init_f();

void debug_leak_create_f(void *ptr, char *file, int line);

void debug_leak_stack_f(void *ptr, char *file, int line);

void debug_leak_delete_f(void *ptr, char *file, int line);

void debug_leak_dump_f();

void debug_leak_fini_f();

void debug_leak_by_line_f();

extern int debug_leak_counter;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DEBUG_LEAK_H_INCLUDED
