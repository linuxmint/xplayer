include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/brasero-disc-recorder
plugin_LTLIBRARIES = libbrasero-disc-recorder.la

plugin_in_files = brasero-disc-recorder.plugin.in

libbrasero_disc_recorder_la_SOURCES = totem-disc-recorder.c
libbrasero_disc_recorder_la_LDFLAGS = $(plugin_ldflags)
libbrasero_disc_recorder_la_LIBADD = \
	$(plugin_libadd)	\
	$(DISC_RECORDER_LIBS)
libbrasero_disc_recorder_la_CFLAGS = \
	$(plugin_cflags)		\
	$(DISC_RECORDER_CFLAGS)

-include $(top_srcdir)/git.mk
