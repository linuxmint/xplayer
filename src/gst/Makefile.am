noinst_LTLIBRARIES =			\
	libxplayergsthelpers.la		\
	libxplayergstpixbufhelpers.la	\
	libxplayertimehelpers.la		\
	libxplayerrtlhelpers.la

libxplayergsthelpers_la_SOURCES =	\
	xplayer-gst-helpers.c	\
	xplayer-gst-helpers.h

libxplayergsthelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libxplayergsthelpers_la_CFLAGS =	\
	$(HELPER_CFLAGS)	\
	$(AM_CFLAGS)

libxplayergsthelpers_la_LIBADD = $(HELPER_LIBS)
libxplayergsthelpers_la_LDFLAGS= -no-undefined

libxplayergstpixbufhelpers_la_SOURCES =	\
	xplayer-gst-pixbuf-helpers.c	\
	xplayer-gst-pixbuf-helpers.h

libxplayergstpixbufhelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libxplayergstpixbufhelpers_la_CFLAGS =	\
	$(PIXBUF_HELPER_CFLAGS)		\
	$(AM_CFLAGS)

libxplayergstpixbufhelpers_la_LIBADD = $(PIXBUF_HELPER_LIBS)
libxplayergstpixbufhelpers_la_LDFLAGS= -no-undefined

libxplayertimehelpers_la_SOURCES =	\
	xplayer-time-helpers.c		\
	xplayer-time-helpers.h

libxplayertimehelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libxplayertimehelpers_la_CFLAGS =	\
	$(TIME_HELPER_CFLAGS)	\
	$(AM_CFLAGS)

libxplayertimehelpers_la_LIBADD = $(TIME_HELPER_LIBS)
libxplayertimehelpers_la_LDFLAGS= -no-undefined

libxplayerrtlhelpers_la_SOURCES =	\
	xplayer-rtl-helpers.c		\
	xplayer-rtl-helpers.h

libxplayerrtlhelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libxplayerrtlhelpers_la_CFLAGS =	\
	$(RTL_HELPER_CFLAGS)	\
	$(AM_CFLAGS)

libxplayerrtlhelpers_la_LIBADD = $(RTL_HELPER_LIBS)
libxplayerrtlhelpers_la_LDFLAGS= -no-undefined

EXTRA_DIST = xplayer-time-helpers.h

-include $(top_srcdir)/git.mk
