include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/recent
plugin_LTLIBRARIES = librecent.la

plugin_in_files = recent.plugin.in

librecent_la_SOURCES = totem-recent.c
librecent_la_LDFLAGS = $(plugin_ldflags)
librecent_la_LIBADD = $(plugin_libadd)
librecent_la_CFLAGS = $(plugin_cflags)

-include $(top_srcdir)/git.mk
