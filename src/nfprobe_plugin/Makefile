# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.1.1.1 2006/11/19 15:16:07 paolo Exp $

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
libexecdir=${exec_prefix}/libexec
datadir=${prefix}/share
mandir=${prefix}/man
sysconfdir=${prefix}/etc
srcdir=.
top_srcdir=../..
CC=gcc
DEFS= -DPACKAGE=\"pmacctd\" -DVERSION=\"1.5.0rc3-cvs\" -DCOMPILE_ARGS=\"\ --enable-pgsql\ --with-pgsql-includes=/usr/include/postgresql/\" -DPROGNAME=1 -DIM_LITTLE_ENDIAN=1 -DHAVE_L2=1 -DHAVE_PCAP_H=1 -DHAVE_LIBPCAP=1 -DPCAP_7=1 -DPCAP_TYPE_linux=1 -DWITH_PGSQL=1 -DHAVE_DLOPEN=1 -DSTDC_HEADERS=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_GETOPT_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_U_INT64_T=1 -DHAVE_U_INT32_T=1 -DHAVE_U_INT16_T=1 -DHAVE_U_INT8_T=1 -DHAVE_64BIT_COUNTERS=1 -DENABLE_THREADS=1 -DRETSIGTYPE=void -DHAVE_VSNPRINTF=1 -DHAVE_MALLOPT=1 
LDFLAGS=-Wl,--export-dynamic 
CFLAGS=$(DEFS) -I$(srcdir) -I.. -O2 -g -O2  -I/usr/include/postgresql
CPPFLAGS=
LIBS=-lpcap  -ldl -lpq -lldap -lkrb5 -lssl -lgssapi_krb5 -lm -lz -lpthread
INSTALL=/usr/bin/install -c
RANLIB=ranlib 

#CFLAGS+=-DFLOW_RB		# Use red-black tree for flows
CFLAGS+=-DFLOW_SPLAY		# Use splay tree for flows
CFLAGS+=-DEXPIRY_RB		# Use red-black tree for expiry events
#CFLAGS+=-DEXPIRY_SPLAY		# Use splay tree for expiry events

TARGETS=libnfprobe_plugin.a

COMMON=convtime.o strlcat.o

all: $(TARGETS)

libnfprobe_plugin.a: nfprobe_plugin.o netflow1.o netflow5.o netflow9.o $(COMMON)
	ar rc $@ netflow1.o netflow5.o netflow9.o nfprobe_plugin.o $(COMMON)
	$(RANLIB) $@

clean:
	rm -f $(TARGETS) *.o core *.core

realclean: clean
	rm -rf autom4te.cache Makefile config.log config.status

distclean: realclean
	rm -f config.h* configure

strip:
	strip $(TARGETS)

install: all

