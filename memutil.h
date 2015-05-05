#ifndef MEMDUMP_H_INCLUDED
#define MEMDUMP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// return the index of the first different byte, or -1
int memdiff(void *pa, int na, void *pb, int nb); 

// convert src to a nicely printable string in buf, returns len(buf) or <0
char* memdump(char *buf, int len, void *src, int n);

void* memfind(void *haystack, int hay_len, void *needle, int needle_len);

void* mem_pack_4(void *buf, char a1, char a2, char a3, char a4);
void* mem_pack_6(void *buf, char a1, char a2, char a3, char a4, char a5, char a6);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MEMDUMP_H_INCLUDED
