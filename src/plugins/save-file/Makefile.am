include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/save-file
plugin_LTLIBRARIES = libsave-file.la

plugin_in_files = save-file.plugin.in

libsave_file_la_SOURCES = totem-save-file.c
libsave_file_la_LDFLAGS = $(plugin_ldflags)
libsave_file_la_LIBADD = \
	$(plugin_libadd)	\
	$(SAVE_FILE_LIBS)
libsave_file_la_CFLAGS = \
	$(plugin_cflags)		\
	$(SAVE_FILE_CFLAGS)

-include $(top_srcdir)/git.mk
