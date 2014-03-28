# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.2 2012/03/28 17:46:09 paolo Exp $

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

#CFLAGS+=

TARGETS=libisis.a

COMMON=

all: $(TARGETS)

libisis.a: isis.o checksum.o dict.o table.o prefix.o sockunion.o hash.o stream.o thread.o linklist.o isis_circuit.o isis_events.o isis_route.o isis_tlv.o isis_csm.o isis_flags.o isis_misc.o isisd.o isis_adjacency.o isis_dynhn.o isis_spf.o iso_checksum.o isis_lsp.o isis_pdu.o $(COMMON)
	ar rc $@ isis.o checksum.o dict.o table.o prefix.o sockunion.o hash.o stream.o thread.o linklist.o isis_circuit.o isis_events.o isis_route.o isis_tlv.o isis_csm.o isis_flags.o isis_misc.o isisd.o isis_adjacency.o isis_dynhn.o isis_spf.o iso_checksum.o isis_lsp.o isis_pdu.o $(COMMON)
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

