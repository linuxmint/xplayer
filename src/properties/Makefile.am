noinst_LTLIBRARIES = libbaconvideowidgetproperties.la

libbaconvideowidgetproperties_la_SOURCES = \
	bacon-video-widget-properties.c \
	bacon-video-widget-properties.h \
	$(NULL)

libbaconvideowidgetproperties_la_CPPFLAGS =	\
	-I$(top_srcdir)/src/			\
	-DDATADIR=\""$(datadir)/totem/"\"	\
	$(AM_CPPFLAGS)

libbaconvideowidgetproperties_la_CFLAGS = \
	$(PROPERTIES_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS)

-include $(top_srcdir)/git.mk
