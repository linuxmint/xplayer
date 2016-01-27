include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/apple-trailers
plugin_LTLIBRARIES = libapple-trailers.la

plugin_in_files = apple-trailers.plugin.in

libapple_trailers_la_SOURCES = totem-apple-trailers.c
libapple_trailers_la_LDFLAGS = $(plugin_ldflags)
libapple_trailers_la_LIBADD = $(plugin_libadd)
libapple_trailers_la_CFLAGS = $(plugin_cflags)

-include $(top_srcdir)/git.mk
