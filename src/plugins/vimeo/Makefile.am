include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/vimeo
plugin_LTLIBRARIES = libvimeo.la

plugin_in_files = vimeo.plugin.in

libvimeo_la_SOURCES = totem-vimeo.c
libvimeo_la_LDFLAGS = $(plugin_ldflags)
libvimeo_la_LIBADD = $(plugin_libadd)
libvimeo_la_CFLAGS = $(plugin_cflags)

-include $(top_srcdir)/git.mk
