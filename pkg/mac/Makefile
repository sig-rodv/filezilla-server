# Makefile.in generated by automake 1.16.5 from Makefile.am.
# pkg/mac/Makefile.  Generated from Makefile.in by configure.

# Copyright (C) 1994-2021 Free Software Foundation, Inc.

# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.




am__is_gnu_make = { \
  if test -z '$(MAKELEVEL)'; then \
    false; \
  elif test -n '$(MAKE_HOST)'; then \
    true; \
  elif test -n '$(MAKE_VERSION)' && test -n '$(CURDIR)'; then \
    true; \
  else \
    false; \
  fi; \
}
am__make_running_with_option = \
  case $${target_option-} in \
      ?) ;; \
      *) echo "am__make_running_with_option: internal error: invalid" \
              "target option '$${target_option-}' specified" >&2; \
         exit 1;; \
  esac; \
  has_opt=no; \
  sane_makeflags=$$MAKEFLAGS; \
  if $(am__is_gnu_make); then \
    sane_makeflags=$$MFLAGS; \
  else \
    case $$MAKEFLAGS in \
      *\\[\ \	]*) \
        bs=\\; \
        sane_makeflags=`printf '%s\n' "$$MAKEFLAGS" \
          | sed "s/$$bs$$bs[$$bs $$bs	]*//g"`;; \
    esac; \
  fi; \
  skip_next=no; \
  strip_trailopt () \
  { \
    flg=`printf '%s\n' "$$flg" | sed "s/$$1.*$$//"`; \
  }; \
  for flg in $$sane_makeflags; do \
    test $$skip_next = yes && { skip_next=no; continue; }; \
    case $$flg in \
      *=*|--*) continue;; \
        -*I) strip_trailopt 'I'; skip_next=yes;; \
      -*I?*) strip_trailopt 'I';; \
        -*O) strip_trailopt 'O'; skip_next=yes;; \
      -*O?*) strip_trailopt 'O';; \
        -*l) strip_trailopt 'l'; skip_next=yes;; \
      -*l?*) strip_trailopt 'l';; \
      -[dEDm]) skip_next=yes;; \
      -[JT]) skip_next=yes;; \
    esac; \
    case $$flg in \
      *$$target_option*) has_opt=yes; break;; \
    esac; \
  done; \
  test $$has_opt = yes
am__make_dryrun = (target_option=n; $(am__make_running_with_option))
am__make_keepgoing = (target_option=k; $(am__make_running_with_option))
pkgdatadir = $(datadir)/filezilla-server
pkgincludedir = $(includedir)/filezilla-server
pkglibdir = $(libdir)/filezilla-server
pkglibexecdir = $(libexecdir)/filezilla-server
am__cd = CDPATH="$${ZSH_VERSION+.}$(PATH_SEPARATOR)" && cd
install_sh_DATA = $(install_sh) -c -m 644
install_sh_PROGRAM = $(install_sh) -c
install_sh_SCRIPT = $(install_sh) -c
INSTALL_HEADER = $(INSTALL_DATA)
transform = $(program_transform_name)
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
build_triplet = x86_64-pc-linux-gnu
host_triplet = x86_64-pc-linux-gnu
subdir = pkg/mac
ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
am__aclocal_m4_deps = $(top_srcdir)/m4/ax_append_flag.m4 \
	$(top_srcdir)/m4/ax_check_link_flag.m4 \
	$(top_srcdir)/m4/ax_cxx_compile_stdcxx.m4 \
	$(top_srcdir)/m4/check_libc++.m4 \
	$(top_srcdir)/m4/fz_check_pugixml.m4 $(top_srcdir)/m4/pkg.m4 \
	$(top_srcdir)/m4/wxwin.m4 $(top_srcdir)/configure.ac
am__configure_deps = $(am__aclocal_m4_deps) $(CONFIGURE_DEPENDENCIES) \
	$(ACLOCAL_M4)
DIST_COMMON = $(srcdir)/Makefile.am $(dist_noinst_DATA) \
	$(am__DIST_COMMON)
mkinstalldirs = $(install_sh) -d
CONFIG_HEADER = $(top_builddir)/src/config.hpp
CONFIG_CLEAN_FILES = Info.plist server.dist component.plist
CONFIG_CLEAN_VPATH_FILES =
AM_V_P = $(am__v_P_$(V))
am__v_P_ = $(am__v_P_$(AM_DEFAULT_VERBOSITY))
am__v_P_0 = false
am__v_P_1 = :
AM_V_GEN = $(am__v_GEN_$(V))
am__v_GEN_ = $(am__v_GEN_$(AM_DEFAULT_VERBOSITY))
am__v_GEN_0 = @echo "  GEN     " $@;
am__v_GEN_1 = 
AM_V_at = $(am__v_at_$(V))
am__v_at_ = $(am__v_at_$(AM_DEFAULT_VERBOSITY))
am__v_at_0 = @
am__v_at_1 = 
SOURCES =
DIST_SOURCES =
am__can_run_installinfo = \
  case $$AM_UPDATE_INFO_DIR in \
    n|no|NO) false;; \
    *) (install-info --version) >/dev/null 2>&1;; \
  esac
DATA = $(dist_noinst_DATA)
am__tagged_files = $(HEADERS) $(SOURCES) $(TAGS_FILES) $(LISP)
am__DIST_COMMON = $(srcdir)/../files.mkinc $(srcdir)/Info.plist.in \
	$(srcdir)/Makefile.in $(srcdir)/component.plist.in \
	$(srcdir)/server.dist.in
DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)
ACLOCAL = ${SHELL} '/home/kali/demo/filezilla-server-1.7.3/config/missing' aclocal-1.16
AMTAR = $${TAR-tar}
AM_DEFAULT_VERBOSITY = 1
AR = ar
AUTOCONF = ${SHELL} '/home/kali/demo/filezilla-server-1.7.3/config/missing' autoconf
AUTOHEADER = ${SHELL} '/home/kali/demo/filezilla-server-1.7.3/config/missing' autoheader
AUTOMAKE = ${SHELL} '/home/kali/demo/filezilla-server-1.7.3/config/missing' automake-1.16
AWK = gawk
CC = gcc
CCDEPMODE = depmode=gcc3
CFBUNDLEIDSUFFIX = 
CFLAGS = -g -O2
CPPFLAGS = 
CPPUNIT_CFLAGS = 
CPPUNIT_LIBS = 
CSCOPE = cscope
CTAGS = ctags
CXX = g++ -std=c++17
CXXCPP = g++ -E -std=c++17
CXXDEPMODE = depmode=gcc3
CXXFLAGS = -g -O2
CYGPATH_W = echo
DEB_TARGET_ARCH = amd64
DEFS = -DHAVE_CONFIG_H
DEPDIR = .deps
DLLTOOL = false
DPKG_ARCHITECTURE = /usr/bin/dpkg-architecture
DPKG_DEB = /usr/bin/dpkg-deb
DSYMUTIL = 
DUMPBIN = 
ECHO_C = 
ECHO_N = -n
ECHO_T = 
EGREP = /usr/bin/grep -E
ETAGS = etags
EXEC_OBJDIR = .
EXEEXT = 
FGREP = /usr/bin/grep -F
FILECMD = file
FZ_BITS = 64
GREP = /usr/bin/grep
HAS_MAKENSIS = 
HAVE_CXX17 = 1
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = ${INSTALL}
INSTALL_STRIP_PROGRAM = $(install_sh) -c -s
LD = /usr/bin/ld -m elf_x86_64
LDFLAGS = 
LIBFILEZILLA_CFLAGS = -I/usr/include/p11-kit-1 
LIBFILEZILLA_LIBS = -lfilezilla -lpthread 
LIBOBJS = 
LIBS = 
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LIPO = 
LN_S = ln -s
LTLIBOBJS = 
LT_SYS_LIBRARY_PATH = 
MAKEINFO = ${SHELL} '/home/kali/demo/filezilla-server-1.7.3/config/missing' makeinfo
MAKENSIS = 
MANIFEST_TOOL = :
MKDIR_P = /usr/bin/mkdir -p
NM = /usr/bin/nm -B
NMEDIT = 
OBJCXX = g++
OBJCXXDEPMODE = depmode=gcc3
OBJCXXFLAGS = -g -O2
OBJDUMP = objdump
OBJEXT = o
OTOOL = 
OTOOL64 = 
PACKAGE = filezilla-server
PACKAGE_BUGREPORT = Tim Kosse <tim.kosse@filezilla-project.org>
PACKAGE_NAME = FileZilla Server
PACKAGE_NAME_WITHOUT_SPACES = FileZilla_Server
PACKAGE_NAME_WITH_ESCAPED_SPACES = FileZilla\ Server
PACKAGE_STRING = FileZilla Server 1.7.3
PACKAGE_STRING_WITHOUT_SPACES = FileZilla_Server_1.7.3
PACKAGE_TARNAME = filezilla-server
PACKAGE_URL = https://filezilla-project.org/
PACKAGE_VERSION = 1.7.3
PACKAGE_VERSION_MAJOR = 1
PACKAGE_VERSION_MICRO = 3
PACKAGE_VERSION_MINOR = 7
PACKAGE_VERSION_NANO = 0
PACKAGE_VERSION_SHORT = 1.7.3
PATH_SEPARATOR = :
PKG_CONFIG = /usr/bin/pkg-config
PKG_CONFIG_LIBDIR = 
PKG_CONFIG_PATH = 
PUGIXML_LIBS = 
RANLIB = ranlib
SED = /usr/bin/sed
SET_MAKE = 
SHELL = bash
STRIP = strip
VERSION = 1.7.3
WINDRES = 
WINDRESFLAGS = 
WX_CFLAGS = -I/usr/lib/x86_64-linux-gnu/wx/include/gtk3-unicode-3.2 -I/usr/include/wx-3.2 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread
WX_CFLAGS_ONLY = -pthread
WX_CONFIG_PATH = /usr/bin/wx-config
WX_CONFIG_WITH_ARGS = /usr/bin/wx-config  --unicode=yes --universal=no
WX_CPPFLAGS = -I/usr/lib/x86_64-linux-gnu/wx/include/gtk3-unicode-3.2 -I/usr/include/wx-3.2 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__
WX_CXXFLAGS = -I/usr/lib/x86_64-linux-gnu/wx/include/gtk3-unicode-3.2 -I/usr/include/wx-3.2 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread
WX_CXXFLAGS_ONLY = 
WX_LIBS = -L/usr/lib/x86_64-linux-gnu -pthread   -lwx_gtk3u_core-3.2 -lwx_baseu-3.2 
WX_LIBS_STATIC = 
WX_RESCOMP = 
WX_VERSION = 3.2.2
WX_VERSION_MAJOR = 3
WX_VERSION_MICRO = 2
WX_VERSION_MINOR = 2
abs_builddir = /home/kali/demo/filezilla-server-1.7.3/pkg/mac
abs_srcdir = /home/kali/demo/filezilla-server-1.7.3/pkg/mac
abs_top_builddir = /home/kali/demo/filezilla-server-1.7.3
abs_top_srcdir = /home/kali/demo/filezilla-server-1.7.3
ac_ct_AR = ar
ac_ct_CC = gcc
ac_ct_CXX = g++
ac_ct_DUMPBIN = 
ac_ct_OBJCXX = g++
am__include = include
am__leading_dot = .
am__quote = 
am__tar = tar --format=ustar -chf - "$$tardir"
am__untar = tar -xf -
bindir = ${exec_prefix}/bin
build = x86_64-pc-linux-gnu
build_alias = 
build_cpu = x86_64
build_os = linux-gnu
build_vendor = pc
builddir = .
datadir = ${datarootdir}
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}
dvidir = ${docdir}
exec_prefix = ${prefix}
host = x86_64-pc-linux-gnu
host_alias = 
host_cpu = x86_64
host_os = linux-gnu
host_vendor = pc
htmldir = ${docdir}
includedir = ${prefix}/include
infodir = ${datarootdir}/info
install_sh = ${SHELL} /home/kali/demo/filezilla-server-1.7.3/config/install-sh
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
localedir = ${datarootdir}/locale
localstatedir = ${prefix}/var
mandir = ${datarootdir}/man
mkdir_p = $(MKDIR_P)
oldincludedir = /usr/include
pdfdir = ${docdir}
prefix = /usr/local
program_transform_name = s,x,x,
psdir = ${docdir}
runstatedir = ${localstatedir}/run
sbindir = ${exec_prefix}/sbin
sharedstatedir = ${prefix}/com
srcdir = .
sysconfdir = ${prefix}/etc
target_alias = 
top_build_prefix = ../../
top_builddir = ../..
top_srcdir = ../..
PKG_NAME = $(PACKAGE_NAME_WITHOUT_SPACES)_$(PACKAGE_VERSION)
APP_NAME = $(PACKAGE_NAME_WITH_ESCAPED_SPACES).app
PKG_DESTDIR = $(abs_builddir)/files/$(APP_NAME)
PKG_PREFIX = $(PKG_DESTDIR)/Contents
PKG_BINDIR = $(PKG_PREFIX)/MacOS
PKG_LIBDIR = $(PKG_PREFIX)/Frameworks
PKG_DOCDIR = $(PKG_PREFIX)/SharedSupport
PKG_RESDIR = $(PKG_PREFIX)/Resources
PKG_STRIPOPTS = -S -x
PKG_CLEAN = files $(PKG_NAME).app.tar.bz2 $(PKG_NAME).pkg main.pkg service.pkg
SERVER_EXES = \
	$(PKG_BINDIR)/filezilla-server$(EXEEXT) \
	$(PKG_BINDIR)/filezilla-server-config-converter$(EXEEXT) \
	$(PKG_BINDIR)/filezilla-server-crypt$(EXEEXT) \
	$(PKG_BINDIR)/filezilla-server-impersonator$(EXEEXT)

GUI_EXES = \
	$(PKG_BINDIR)/filezilla-server-gui$(EXEEXT)

ALL_EXES = \
	$(SERVER_EXES) \
	$(GUI_EXES)

ALL_BLDS = \
	$(top_builddir)/src/server/$(EXEC_OBJDIR)/filezilla-server$(EXEEXT) \
	$(top_builddir)/src/tools/configconverter/$(EXEC_OBJDIR)/filezilla-server-config-converter$(EXEEXT) \
	$(top_builddir)/src/tools/crypt/$(EXEC_OBJDIR)/filezilla-server-crypt$(EXEEXT) \
	$(top_builddir)/src/gui/$(EXEC_OBJDIR)/filezilla-server-gui$(EXEEXT) \
	$(top_builddir)/src/tools/impersonator/$(EXEC_OBJDIR)/filezilla-server-impersonator$(EXEEXT)


#######
PKG_EULA = COPYING

#######
RED = [ -t 1 ] && echo -en '\033[1;31m'
GREEN = [ -t 1 ] && echo -en '\033[1;32m'
YELLOW = [ -t 1 ] && echo -en '\033[1;33m'
NORMAL = [ -t 1 ] && echo -en '\033[0m'
dist_noinst_DATA = \
	scripts/main/preinstall \
	scripts/main/postinstall \
	scripts/service/postinstall.in

all: all-am

.SUFFIXES:
$(srcdir)/Makefile.in:  $(srcdir)/Makefile.am $(srcdir)/../files.mkinc $(am__configure_deps)
	@for dep in $?; do \
	  case '$(am__configure_deps)' in \
	    *$$dep*) \
	      ( cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh ) \
	        && { if test -f $@; then exit 0; else break; fi; }; \
	      exit 1;; \
	  esac; \
	done; \
	echo ' cd $(top_srcdir) && $(AUTOMAKE) --foreign pkg/mac/Makefile'; \
	$(am__cd) $(top_srcdir) && \
	  $(AUTOMAKE) --foreign pkg/mac/Makefile
Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status
	@case '$?' in \
	  *config.status*) \
	    cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh;; \
	  *) \
	    echo ' cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@ $(am__maybe_remake_depfiles)'; \
	    cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@ $(am__maybe_remake_depfiles);; \
	esac;
$(srcdir)/../files.mkinc $(am__empty):

$(top_builddir)/config.status: $(top_srcdir)/configure $(CONFIG_STATUS_DEPENDENCIES)
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh

$(top_srcdir)/configure:  $(am__configure_deps)
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh
$(ACLOCAL_M4):  $(am__aclocal_m4_deps)
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh
$(am__aclocal_m4_deps):
Info.plist: $(top_builddir)/config.status $(srcdir)/Info.plist.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@
server.dist: $(top_builddir)/config.status $(srcdir)/server.dist.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@
component.plist: $(top_builddir)/config.status $(srcdir)/component.plist.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs
tags TAGS:

ctags CTAGS:

cscope cscopelist:

distdir: $(BUILT_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) distdir-am

distdir-am: $(DISTFILES)
	@srcdirstrip=`echo "$(srcdir)" | sed 's/[].[^$$\\*]/\\\\&/g'`; \
	topsrcdirstrip=`echo "$(top_srcdir)" | sed 's/[].[^$$\\*]/\\\\&/g'`; \
	list='$(DISTFILES)'; \
	  dist_files=`for file in $$list; do echo $$file; done | \
	  sed -e "s|^$$srcdirstrip/||;t" \
	      -e "s|^$$topsrcdirstrip/|$(top_builddir)/|;t"`; \
	case $$dist_files in \
	  */*) $(MKDIR_P) `echo "$$dist_files" | \
			   sed '/\//!d;s|^|$(distdir)/|;s,/[^/]*$$,,' | \
			   sort -u` ;; \
	esac; \
	for file in $$dist_files; do \
	  if test -f $$file || test -d $$file; then d=.; else d=$(srcdir); fi; \
	  if test -d $$d/$$file; then \
	    dir=`echo "/$$file" | sed -e 's,/[^/]*$$,,'`; \
	    if test -d "$(distdir)/$$file"; then \
	      find "$(distdir)/$$file" -type d ! -perm -700 -exec chmod u+rwx {} \;; \
	    fi; \
	    if test -d $(srcdir)/$$file && test $$d != $(srcdir); then \
	      cp -fpR $(srcdir)/$$file "$(distdir)$$dir" || exit 1; \
	      find "$(distdir)/$$file" -type d ! -perm -700 -exec chmod u+rwx {} \;; \
	    fi; \
	    cp -fpR $$d/$$file "$(distdir)$$dir" || exit 1; \
	  else \
	    test -f "$(distdir)/$$file" \
	    || cp -p $$d/$$file "$(distdir)/$$file" \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile $(DATA)
installdirs:
install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	if test -z '$(STRIP)'; then \
	  $(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	    install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
	      install; \
	else \
	  $(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	    install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
	    "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'" install; \
	fi
mostlyclean-generic:

clean-generic:

distclean-generic:
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)
	-test . = "$(srcdir)" || test -z "$(CONFIG_CLEAN_VPATH_FILES)" || rm -f $(CONFIG_CLEAN_VPATH_FILES)

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-generic clean-libtool clean-local mostlyclean-am

distclean: distclean-am
	-rm -f Makefile
distclean-am: clean-am distclean-generic

dvi: dvi-am

dvi-am:

html: html-am

html-am:

info: info-am

info-am:

install-data-am:

install-dvi: install-dvi-am

install-dvi-am:

install-exec-am:

install-html: install-html-am

install-html-am:

install-info: install-info-am

install-info-am:

install-man:

install-pdf: install-pdf-am

install-pdf-am:

install-ps: install-ps-am

install-ps-am:

installcheck-am:

maintainer-clean: maintainer-clean-am
	-rm -f Makefile
maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-generic mostlyclean-libtool

pdf: pdf-am

pdf-am:

ps: ps-am

ps-am:

uninstall-am:

.MAKE: install-am install-strip

.PHONY: all all-am check check-am clean clean-generic clean-libtool \
	clean-local cscopelist-am ctags-am distclean distclean-generic \
	distclean-libtool distdir dvi dvi-am html html-am info info-am \
	install install-am install-data install-data-am install-dvi \
	install-dvi-am install-exec install-exec-am install-html \
	install-html-am install-info install-info-am install-man \
	install-pdf install-pdf-am install-ps install-ps-am \
	install-strip installcheck installcheck-am installdirs \
	maintainer-clean maintainer-clean-generic mostlyclean \
	mostlyclean-generic mostlyclean-libtool pdf pdf-am ps ps-am \
	tags-am uninstall uninstall-am

.PRECIOUS: Makefile


#######

$(ALL_EXES): $(ALL_BLDS)
	@for b in $?; do \
		if [ "$$(basename "$@")" = "$$(basename "$$b")" ]; then \
			$(MKDIR_P) "$$(dirname "$@")"; \
			cp -v "$$b" "$@"; \
			chmod +w "$@"; \
		fi; \
	done

$(ALL_BLDS): .maybe_rebuild
	@true #An empty recipe doesn't work here.

.maybe_rebuild:
	@cd ${top_builddir} && $(MAKE) $(AM_MAKEFLAGS)

.PHONY: .maybe_rebuild

#######

files/.stripped: files/.prepared
	@for f in $(PKG_BINDIR)/* $(PKG_LIBDIR)/*; do \
		if [ -f "$$f" ]; then \
			{ chmod +w "$$f" && touch -r "$$f" "$$f.timestamp" && $(STRIP) $(PKG_STRIPOPTS) "$$f" && touch -r "$$f.timestamp" "$$f"; } || exit 1; \
			rm -f "$$f.timestamp"; \
		fi; \
	done
	@touch $@

$(PKG_DOCDIR)/$(PKG_EULA): ${top_srcdir}/$(PKG_EULA)
	@$(MKDIR_P) $(PKG_DOCDIR)
	@cp -v "$<" "$@"

$(PKG_DOCDIR)/NEWS: ${top_srcdir}/NEWS
	@$(MKDIR_P) $(PKG_DOCDIR)
	@cp -v "$<" "$@"

clean-local:
	-rm -rf $(PKG_DESTDIR) $(PKG_CLEAN)

pkg-app-prepare: files/.prepared
pkg-app-strip: files/.stripped
pkg-app: ${abs_builddir}/$(PKG_NAME).app.tar.bz2
pkg-app-pkg: ${abs_builddir}/$(PKG_NAME).pkg

###########################################################

files/.prepared: $(ALL_EXES) Makefile $(PKG_DOCDIR)/$(PKG_EULA) $(PKG_DOCDIR)/NEWS $(PKG_RESDIR)/filezilla-server-gui.icns $(PKG_RESDIR)/org.filezilla-project.filezilla-server.service.plist $(PKG_PREFIX)/Info.plist
	${srcdir}/dylibcopy.sh $(PKG_DESTDIR) ""
	@$(GREEN); echo -n "Installation files have been prepared. You can find them here: "; $(YELLOW); echo $(PKG_DESTDIR); $(NORMAL); echo
	@touch $@

${abs_builddir}/$(PKG_NAME).app.tar.bz2: files/.stripped
	cd files && tar cvjSf $@ $(APP_NAME)
	@$(GREEN); echo -n "App bundle has been created. You can find it here: "; $(YELLOW); echo $@; $(NORMAL); echo

$(PKG_PREFIX)/Info.plist: Info.plist
	@$(MKDIR_P) "$$(dirname "$@")"
	@cp -v "$<" "$@"

$(PKG_RESDIR)/filezilla-server-gui.icns: ${top_srcdir}/res/filezilla-server-gui.icns
	@$(MKDIR_P) "$$(dirname "$@")"
	@cp -v "$<" "$@"

$(PKG_RESDIR)/org.filezilla-project.filezilla-server.service.plist: ${top_builddir}/res/org.filezilla-project.filezilla-server.service.plist
	@$(MKDIR_P) "$$(dirname "$@")"
	@cp -v "$<" "$@"

.PHONY: pkg-app pkg-app-prepare pkg-app-strip pkg-app-pkg

main.pkg: files/.stripped scripts/main/preinstall scripts/main/postinstall
	pkgbuild --compression latest --min-os-version 10.13.2 --scripts "${srcdir}/scripts/main" --root files --component-plist "component.plist" "$@"

service.pkg: scripts/service/postinstall
	pkgbuild --compression latest --min-os-version 10.13.2 --nopayload --scripts "${builddir}/scripts/service" --identifier org.filezilla-project.filezilla-server.service $@

${abs_builddir}/${PKG_NAME}.pkg: main.pkg service.pkg server.dist
	productbuild --distribution server.dist --package-path . $@
	@$(GREEN); echo -n "App package has been created. You can find it here: "; $(YELLOW); echo $@; $(NORMAL); echo

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
