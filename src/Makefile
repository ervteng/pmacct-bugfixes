# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4-p6 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999, 2001 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/pmacctd
pkglibdir = $(libdir)/pmacctd
pkgincludedir = $(includedir)/pmacctd

top_builddir = ..

ACLOCAL = /home/crossmobile/pmacct-daily/missing aclocal-1.4
AUTOCONF = autoconf
AUTOMAKE = /home/crossmobile/pmacct-daily/missing automake-1.4
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
CC = gcc
EXTRABIN =  pmpgplay
MAKE = make
MAKEINFO = /home/crossmobile/pmacct-daily/missing makeinfo
PACKAGE = pmacctd
PLUGINS =  pgsql_plugin.c sql_common.c sql_handlers.c log_templates.c preprocess.c
RANLIB = ranlib
SERVER_LIBS = -lnfprobe_plugin -Lnfprobe_plugin/ -lsfprobe_plugin -Lsfprobe_plugin/ -lbgp -Lbgp/ -ltee_plugin -Ltee_plugin/ -lisis -Lisis/
THREADS_SOURCES = thread_pool.c
VERSION = 1.5.0rc3-cvs

SUBDIRS = nfprobe_plugin sfprobe_plugin bgp tee_plugin isis 
sbin_PROGRAMS = pmacctd nfacctd sfacctd uacctd
bin_PROGRAMS = pmacct  pmpgplay 
EXTRA_PROGRAMS = pmmyplay pmpgplay
pmacctd_PLUGINS =  pgsql_plugin.c sql_common.c sql_handlers.c log_templates.c preprocess.c thread_pool.c -lnfprobe_plugin -Lnfprobe_plugin/ -lsfprobe_plugin -Lsfprobe_plugin/ -lbgp -Lbgp/ -ltee_plugin -Ltee_plugin/ -lisis -Lisis/
pmacctd_SOURCES = pmacctd.c signals.c util.c strlcpy.c plugin_hooks.c 	server.c acct.c memory.c ll.c cfg.c imt_plugin.c log.c pkt_handlers.c 	cfg_handlers.c net_aggr.c bpf_filter.c print_plugin.c ip_frag.c 	ports_aggr.c addr.c pretag.c pretag_handlers.c ip_flow.c setproctitle.c 	classifier.c regexp.c regsub.c conntrack.c xflow_status.c nl.c 	plugin_common.c

pmacctd_LDFLAGS = $(DEFS) 
pmacctd_LDADD = $(pmacctd_PLUGINS)
nfacctd_SOURCES = nfacctd.c signals.c util.c strlcpy.c plugin_hooks.c         server.c acct.c memory.c cfg.c imt_plugin.c log.c pkt_handlers.c         cfg_handlers.c net_aggr.c bpf_filter.c print_plugin.c pretag.c 	pretag_handlers.c ports_aggr.c nfv8_handlers.c nfv9_template.c addr.c 	setproctitle.c ip_flow.c classifier.c regexp.c regsub.c conntrack.c 	xflow_status.c plugin_common.c

nfacctd_LDFLAGS = $(DEFS)
nfacctd_LDADD = $(pmacctd_PLUGINS)
sfacctd_SOURCES = sfacctd.c signals.c util.c strlcpy.c plugin_hooks.c         server.c acct.c memory.c cfg.c imt_plugin.c log.c pkt_handlers.c         cfg_handlers.c net_aggr.c bpf_filter.c print_plugin.c pretag.c 	pretag_handlers.c ports_aggr.c addr.c ll.c setproctitle.c ip_flow.c 	classifier.c regexp.c regsub.c conntrack.c xflow_status.c 	plugin_common.c sfv5_module.c

sfacctd_LDFLAGS = $(DEFS)
sfacctd_LDADD = $(pmacctd_PLUGINS)
uacctd_SOURCES = uacctd.c signals.c util.c strlcpy.c plugin_hooks.c         server.c acct.c memory.c ll.c cfg.c imt_plugin.c log.c pkt_handlers.c 	cfg_handlers.c net_aggr.c bpf_filter.c print_plugin.c ip_frag.c 	ports_aggr.c addr.c pretag.c pretag_handlers.c ip_flow.c setproctitle.c 	classifier.c regexp.c regsub.c conntrack.c xflow_status.c nl.c 	plugin_common.c

uacctd_LDFLAGS = $(DEFS) 
uacctd_LDADD = $(pmacctd_PLUGINS)
pmacct_SOURCES = pmacct.c strlcpy.c addr.c
pmmyplay_SOURCES = pmmyplay.c strlcpy.c sql_handlers.c log_templates.c addr.c 
pmpgplay_SOURCES = pmpgplay.c strlcpy.c sql_handlers.c log_templates.c addr.c 
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS) $(sbin_PROGRAMS)


DEFS =  -DPACKAGE=\"pmacctd\" -DVERSION=\"1.5.0rc3-cvs\" -DCOMPILE_ARGS=\"\ --enable-pgsql\ --with-pgsql-includes=/usr/include/postgresql/\" -DPROGNAME=1 -DIM_LITTLE_ENDIAN=1 -DHAVE_L2=1 -DHAVE_PCAP_H=1 -DHAVE_LIBPCAP=1 -DPCAP_7=1 -DPCAP_TYPE_linux=1 -DWITH_PGSQL=1 -DHAVE_DLOPEN=1 -DSTDC_HEADERS=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_GETOPT_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_U_INT64_T=1 -DHAVE_U_INT32_T=1 -DHAVE_U_INT16_T=1 -DHAVE_U_INT8_T=1 -DHAVE_64BIT_COUNTERS=1 -DENABLE_THREADS=1 -DRETSIGTYPE=void -DHAVE_VSNPRINTF=1 -DHAVE_MALLOPT=1  -I. -I$(srcdir) 
CPPFLAGS = 
LDFLAGS = -Wl,--export-dynamic 
LIBS = -lpcap  -ldl -lpq -lldap -lkrb5 -lssl -lgssapi_krb5 -lm -lz -lpthread
pmmyplay_OBJECTS =  pmmyplay.o strlcpy.o sql_handlers.o log_templates.o \
addr.o
pmmyplay_LDADD = $(LDADD)
pmmyplay_DEPENDENCIES = 
pmmyplay_LDFLAGS = 
pmpgplay_OBJECTS =  pmpgplay.o strlcpy.o sql_handlers.o log_templates.o \
addr.o
pmpgplay_LDADD = $(LDADD)
pmpgplay_DEPENDENCIES = 
pmpgplay_LDFLAGS = 
pmacct_OBJECTS =  pmacct.o strlcpy.o addr.o
pmacct_LDADD = $(LDADD)
pmacct_DEPENDENCIES = 
pmacct_LDFLAGS = 
pmacctd_OBJECTS =  pmacctd.o signals.o util.o strlcpy.o plugin_hooks.o \
server.o acct.o memory.o ll.o cfg.o imt_plugin.o log.o pkt_handlers.o \
cfg_handlers.o net_aggr.o bpf_filter.o print_plugin.o ip_frag.o \
ports_aggr.o addr.o pretag.o pretag_handlers.o ip_flow.o setproctitle.o \
classifier.o regexp.o regsub.o conntrack.o xflow_status.o nl.o \
plugin_common.o
pmacctd_DEPENDENCIES = 
nfacctd_OBJECTS =  nfacctd.o signals.o util.o strlcpy.o plugin_hooks.o \
server.o acct.o memory.o cfg.o imt_plugin.o log.o pkt_handlers.o \
cfg_handlers.o net_aggr.o bpf_filter.o print_plugin.o pretag.o \
pretag_handlers.o ports_aggr.o nfv8_handlers.o nfv9_template.o addr.o \
setproctitle.o ip_flow.o classifier.o regexp.o regsub.o conntrack.o \
xflow_status.o plugin_common.o
nfacctd_DEPENDENCIES = 
sfacctd_OBJECTS =  sfacctd.o signals.o util.o strlcpy.o plugin_hooks.o \
server.o acct.o memory.o cfg.o imt_plugin.o log.o pkt_handlers.o \
cfg_handlers.o net_aggr.o bpf_filter.o print_plugin.o pretag.o \
pretag_handlers.o ports_aggr.o addr.o ll.o setproctitle.o ip_flow.o \
classifier.o regexp.o regsub.o conntrack.o xflow_status.o \
plugin_common.o sfv5_module.o
sfacctd_DEPENDENCIES = 
uacctd_OBJECTS =  uacctd.o signals.o util.o strlcpy.o plugin_hooks.o \
server.o acct.o memory.o ll.o cfg.o imt_plugin.o log.o pkt_handlers.o \
cfg_handlers.o net_aggr.o bpf_filter.o print_plugin.o ip_frag.o \
ports_aggr.o addr.o pretag.o pretag_handlers.o ip_flow.o setproctitle.o \
classifier.o regexp.o regsub.o conntrack.o xflow_status.o nl.o \
plugin_common.o
uacctd_DEPENDENCIES = 
CFLAGS = -O2 -g -O2  -I/usr/include/postgresql
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
SOURCES = $(pmmyplay_SOURCES) $(pmpgplay_SOURCES) $(pmacct_SOURCES) $(pmacctd_SOURCES) $(nfacctd_SOURCES) $(sfacctd_SOURCES) $(uacctd_SOURCES)
OBJECTS = $(pmmyplay_OBJECTS) $(pmpgplay_OBJECTS) $(pmacct_OBJECTS) $(pmacctd_OBJECTS) $(nfacctd_OBJECTS) $(sfacctd_OBJECTS) $(uacctd_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps src/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

mostlyclean-sbinPROGRAMS:

clean-sbinPROGRAMS:
	-test -z "$(sbin_PROGRAMS)" || rm -f $(sbin_PROGRAMS)

distclean-sbinPROGRAMS:

maintainer-clean-sbinPROGRAMS:

install-sbinPROGRAMS: $(sbin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(sbindir)
	@list='$(sbin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(sbindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(sbindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-sbinPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(sbin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(sbindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

pmmyplay: $(pmmyplay_OBJECTS) $(pmmyplay_DEPENDENCIES)
	@rm -f pmmyplay
	$(LINK) $(pmmyplay_LDFLAGS) $(pmmyplay_OBJECTS) $(pmmyplay_LDADD) $(LIBS)

pmpgplay: $(pmpgplay_OBJECTS) $(pmpgplay_DEPENDENCIES)
	@rm -f pmpgplay
	$(LINK) $(pmpgplay_LDFLAGS) $(pmpgplay_OBJECTS) $(pmpgplay_LDADD) $(LIBS)

pmacct: $(pmacct_OBJECTS) $(pmacct_DEPENDENCIES)
	@rm -f pmacct
	$(LINK) $(pmacct_LDFLAGS) $(pmacct_OBJECTS) $(pmacct_LDADD) $(LIBS)

pmacctd: $(pmacctd_OBJECTS) $(pmacctd_DEPENDENCIES)
	@rm -f pmacctd
	$(LINK) $(pmacctd_LDFLAGS) $(pmacctd_OBJECTS) $(pmacctd_LDADD) $(LIBS)

nfacctd: $(nfacctd_OBJECTS) $(nfacctd_DEPENDENCIES)
	@rm -f nfacctd
	$(LINK) $(nfacctd_LDFLAGS) $(nfacctd_OBJECTS) $(nfacctd_LDADD) $(LIBS)

sfacctd: $(sfacctd_OBJECTS) $(sfacctd_DEPENDENCIES)
	@rm -f sfacctd
	$(LINK) $(sfacctd_LDFLAGS) $(sfacctd_OBJECTS) $(sfacctd_LDADD) $(LIBS)

uacctd: $(uacctd_OBJECTS) $(uacctd_DEPENDENCIES)
	@rm -f uacctd
	$(LINK) $(uacctd_LDFLAGS) $(uacctd_OBJECTS) $(uacctd_LDADD) $(LIBS)

# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.



all-recursive install-data-recursive install-exec-recursive \
installdirs-recursive install-recursive uninstall-recursive  \
check-recursive installcheck-recursive info-recursive dvi-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	rev=''; list='$(SUBDIRS)'; for subdir in $$list; do \
	  rev="$$subdir $$rev"; \
	  test "$$subdir" != "." || dot_seen=yes; \
	done; \
	test "$$dot_seen" = "no" && rev=". $$rev"; \
	target=`echo $@ | sed s/-recursive//`; \
	for subdir in $$rev; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
tags-recursive:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test "$$subdir" = . || (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) tags); \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS: tags-recursive $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
   if test "$$subdir" = .; then :; else \
	    test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
   fi; \
	done; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
	for subdir in $(SUBDIRS); do \
	  if test "$$subdir" = .; then :; else \
	    test -d $(distdir)/$$subdir \
	    || mkdir $(distdir)/$$subdir \
	    || exit 1; \
	    chmod 777 $(distdir)/$$subdir; \
	    (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) top_distdir=../$(top_distdir) distdir=../$(distdir)/$$subdir distdir) \
	      || exit 1; \
	  fi; \
	done
info-am:
info: info-recursive
dvi-am:
dvi: dvi-recursive
check-am: all-am
check: check-recursive
installcheck-am:
installcheck: installcheck-recursive
install-exec-am: install-binPROGRAMS install-sbinPROGRAMS
install-exec: install-exec-recursive

install-data-am:
install-data: install-data-recursive

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-recursive
uninstall-am: uninstall-binPROGRAMS uninstall-sbinPROGRAMS
uninstall: uninstall-recursive
all-am: Makefile $(PROGRAMS)
all-redirect: all-recursive
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs: installdirs-recursive
installdirs-am:
	$(mkinstalldirs)  $(DESTDIR)$(bindir) $(DESTDIR)$(sbindir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-binPROGRAMS mostlyclean-sbinPROGRAMS \
		mostlyclean-compile mostlyclean-tags \
		mostlyclean-generic

mostlyclean: mostlyclean-recursive

clean-am:  clean-binPROGRAMS clean-sbinPROGRAMS clean-compile clean-tags \
		clean-generic mostlyclean-am

clean: clean-recursive

distclean-am:  distclean-binPROGRAMS distclean-sbinPROGRAMS \
		distclean-compile distclean-tags distclean-generic \
		clean-am

distclean: distclean-recursive

maintainer-clean-am:  maintainer-clean-binPROGRAMS \
		maintainer-clean-sbinPROGRAMS maintainer-clean-compile \
		maintainer-clean-tags maintainer-clean-generic \
		distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-recursive

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-sbinPROGRAMS distclean-sbinPROGRAMS clean-sbinPROGRAMS \
maintainer-clean-sbinPROGRAMS uninstall-sbinPROGRAMS \
install-sbinPROGRAMS mostlyclean-compile distclean-compile \
clean-compile maintainer-clean-compile install-data-recursive \
uninstall-data-recursive install-exec-recursive \
uninstall-exec-recursive installdirs-recursive uninstalldirs-recursive \
all-recursive check-recursive installcheck-recursive info-recursive \
dvi-recursive mostlyclean-recursive distclean-recursive clean-recursive \
maintainer-clean-recursive tags tags-recursive mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir info-am info \
dvi-am dvi check check-am installcheck-am installcheck install-exec-am \
install-exec install-data-am install-data install-am install \
uninstall-am uninstall all-redirect all-am all installdirs-am \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
