include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/properties
plugin_LTLIBRARIES = libmovie-properties.la

plugin_in_files = movie-properties.plugin.in

libmovie_properties_la_SOURCES = totem-movie-properties.c
libmovie_properties_la_LDFLAGS = $(plugin_ldflags)
libmovie_properties_la_LIBADD = \
	$(plugin_libadd)	\
	$(top_builddir)/src/properties/libbaconvideowidgetproperties.la
libmovie_properties_la_CFLAGS = $(plugin_cflags)
libmovie_properties_la_CPPFLAGS = \
	-I$(top_srcdir)/src/properties/ \
	-I$(top_srcdir)/src/backend/ \
	$(AM_CPPFLAGS)

-include $(top_srcdir)/git.mk
