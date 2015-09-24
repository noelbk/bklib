.SUFFIXES: .x .h .c

#---------------------------------------------------------------------
# unix
l = .a
o = .o
x = 
CFLAGS = -Wall -g -I. -I.. -O2 # -pg
LFLAGS = -g -O2 # -pg
LINK_EXE = $(CC) $(LFLAGS) -o $@
#LINK_LIB = ar r $@ 
LINK_LIB = ld -g -r -o $@
MAKE_CMD = $(MAKE) -$(MAKEFILE)
RM = rm -f

# os-specific libbk objects
libbk_objs_os=sock_raw$o sock_send_fd$o ifcfg_unix$o
libbk_libs_os=-lpam -lpthread  -lz -ldl

openssl_libs=/usr/lib/x86_64-linux-gnu/libcrypto.a

default: all

openssl: $(openssl_libs)

include makefile.common

