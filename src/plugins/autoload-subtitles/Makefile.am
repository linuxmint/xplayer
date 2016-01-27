include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/autoload-subtitles
plugin_LTLIBRARIES = libautoload-subtitles.la

plugin_in_files = autoload-subtitles.plugin.in

libautoload_subtitles_la_SOURCES = totem-autoload-subtitles.c
libautoload_subtitles_la_LDFLAGS = $(plugin_ldflags)
libautoload_subtitles_la_LIBADD = $(plugin_libadd)
libautoload_subtitles_la_CFLAGS = $(plugin_cflags)

-include $(top_srcdir)/git.mk
