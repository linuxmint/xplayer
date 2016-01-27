include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/im-status
plugin_LTLIBRARIES = libxplayer-im-status.la

plugin_in_files = xplayer-im-status.plugin.in

libxplayer_im_status_la_SOURCES = xplayer-im-status.c
libxplayer_im_status_la_LDFLAGS = $(plugin_ldflags)
libxplayer_im_status_la_LIBADD = \
	$(plugin_libadd)
libxplayer_im_status_la_CFLAGS = \
	$(plugin_cflags)

-include $(top_srcdir)/git.mk
