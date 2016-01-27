include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/screensaver
plugin_LTLIBRARIES = libscreensaver.la

plugin_in_files = screensaver.plugin.in

libscreensaver_la_SOURCES = totem-screensaver.c
libscreensaver_la_LDFLAGS = $(plugin_ldflags)
libscreensaver_la_LIBADD = $(plugin_libadd)
libscreensaver_la_CPPFLAGS = $(AM_CPPFLAGS)
libscreensaver_la_CFLAGS = $(plugin_cflags)

-include $(top_srcdir)/git.mk
