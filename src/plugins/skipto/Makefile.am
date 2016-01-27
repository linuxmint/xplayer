include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/skipto
plugin_LTLIBRARIES = libskipto.la

plugin_in_files = skipto.plugin.in

uidir = $(plugindir)
ui_DATA = skipto.ui
EXTRA_DIST += $(ui_DATA)

libskipto_la_SOURCES = \
	xplayer-skipto-plugin.c	\
	xplayer-time-entry.c	\
	xplayer-time-entry.h	\
	xplayer-skipto.c		\
	xplayer-skipto.h
libskipto_la_LDFLAGS = $(plugin_ldflags)
libskipto_la_CFLAGS = $(plugin_cflags)
libskipto_la_LIBADD = $(plugin_libadd)

-include $(top_srcdir)/git.mk
