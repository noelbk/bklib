#include "configbk.h"

static inline 
int
array_count(array_t *self) { 
    return self->m_count; 
}

static inline 
int
array_set_count(array_t *self, int len) { 
    if( len > self->m_count ) {
	array_add(self, len);
    }
    else {
	self->m_count = len;
    }
    return self->m_count = len; 
}

static inline 
void*
array_get(array_t *self, int i) {
    if( i < 0 ) {
	i = self->m_count+i;
    }
    if( !self->m_array || self->m_eltsz<=0 || i<0 || i>=self->m_count ) {
	return 0;
    }
    return self->m_array + (i * self->m_eltsz);
}

