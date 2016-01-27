include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/im-status
plugin_LTLIBRARIES = libtotem-im-status.la

plugin_in_files = totem-im-status.plugin.in

libtotem_im_status_la_SOURCES = totem-im-status.c
libtotem_im_status_la_LDFLAGS = $(plugin_ldflags)
libtotem_im_status_la_LIBADD = \
	$(plugin_libadd)
libtotem_im_status_la_CFLAGS = \
	$(plugin_cflags)

-include $(top_srcdir)/git.mk
