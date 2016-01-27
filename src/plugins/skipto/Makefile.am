include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/skipto
plugin_LTLIBRARIES = libskipto.la

plugin_in_files = skipto.plugin.in

uidir = $(plugindir)
ui_DATA = skipto.ui
EXTRA_DIST += $(ui_DATA)

libskipto_la_SOURCES = \
	totem-skipto-plugin.c	\
	totem-time-entry.c	\
	totem-time-entry.h	\
	totem-skipto.c		\
	totem-skipto.h
libskipto_la_LDFLAGS = $(plugin_ldflags)
libskipto_la_CFLAGS = $(plugin_cflags)
libskipto_la_LIBADD = $(plugin_libadd)

-include $(top_srcdir)/git.mk
