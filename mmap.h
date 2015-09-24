/**
  \file mmap.h
  \brief  
  \author Noel Burton-Krahn <noel@burton-krahn.com>
  \date 2015-09-23

  routines for mmap'ing files.  You may think mmap would be faster
  than seek/read/write, but see mmap_t:

    random read=5017 write=4983 op_read_max=10000
    read: 0.064210s
    mmap: 0.190463s

  Under gcc -O2, seek/read/write is about 3x *faster* than mmap

*/

#ifndef MMAP_H_INCLUDED
#define MMAP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum {
    MMAP_MODE_READ=0 // default: read-only, no create
    ,MMAP_MODE_WRITE=1 // read, write, create
    ,MMAP_MODE_PRIVATE=2 // MAP_PRIVATE
};

typedef struct mmap_s mmap_t;
    
struct mmap_s {
    int mode;
    int fd;
    int close_fd;
    int blocksz;
    void *start;
    size_t max;
    size_t len;
};
    
int
mmap_open(mmap_t *mm, const char *path, int mode, int blocksz);

int
mmap_fd(mmap_t *mm, int fd, int mode, int blocksz);

#define MMAP_APPEND ((size_t)-1)

// requires MMAP_MODE_WRITE off can be MMAP_APPEND
void*
mmap_write(mmap_t *mm, size_t off, size_t len);

// convert a pointer in mmap to an offset 
size_t
mmap_offset(mmap_t *mm, void *ptr);

// convert an in mmap to a pointer (assuming len < mm->max)
void*
mmap_ptr(mmap_t *mm, size_t off);

int
mmap_close(mmap_t *mm);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MMAP_H_INCLUDED
