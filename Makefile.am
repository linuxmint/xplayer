ACLOCAL_AMFLAGS = -I m4 -I libgd ${ACLOCAL_FLAGS}

NORMAL_SUBDIRS = libgd po data help src docs

SUBDIRS = $(NORMAL_SUBDIRS)

if ENABLE_BROWSER_PLUGINS
SUBDIRS += browser-plugin
endif

EXTRA_DIST =			\
	license_change		\
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	AUTHORS NEWS		\
	totem.spec.in		\
	totem.spec		\
	ChangeLog.pre-gitlog	\
	MAINTAINERS		\
	totem.doap

CLEANFILES = totem.spec

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/configure.scan \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/py-compile \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/ChangeLog \
	$(srcdir)/omf.make \
	$(srcdir)/xmldocs.make \
	`find "$(srcdir)" -type f -name Makefile.in -print`

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

DIST_SUBDIRS = $(NORMAL_SUBDIRS) browser-plugin
DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-gtk-doc --with-nautilusdir='$${libdir}/nautilus/extensions-2.0-distcheck' --enable-vala=no --enable-introspection

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --after="Mon Jul 20 23:47:57 2009" > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
