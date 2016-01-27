include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/lirc
plugin_LTLIBRARIES = liblirc.la

plugin_in_files = lirc.plugin.in

configdir = $(PLUGINDIR)/lirc
config_DATA = totem_lirc_default
EXTRA_DIST += $(config_DATA)

liblirc_la_SOURCES = totem-lirc.c
liblirc_la_LDFLAGS = $(plugin_ldflags)
liblirc_la_LIBADD = \
	$(plugin_libadd)	\
	$(LIRC_LIBS)
liblirc_la_CFLAGS = \
	$(plugin_cflags)	\
	$(LIRC_CFLAGS)

-include $(top_srcdir)/git.mk
