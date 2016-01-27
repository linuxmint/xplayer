noinst_LTLIBRARIES =			\
	libtotemgsthelpers.la		\
	libtotemgstpixbufhelpers.la	\
	libtotemtimehelpers.la		\
	libtotemrtlhelpers.la

libtotemgsthelpers_la_SOURCES =	\
	totem-gst-helpers.c	\
	totem-gst-helpers.h

libtotemgsthelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libtotemgsthelpers_la_CFLAGS =	\
	$(HELPER_CFLAGS)	\
	$(AM_CFLAGS)

libtotemgsthelpers_la_LIBADD = $(HELPER_LIBS)
libtotemgsthelpers_la_LDFLAGS= -no-undefined

libtotemgstpixbufhelpers_la_SOURCES =	\
	totem-gst-pixbuf-helpers.c	\
	totem-gst-pixbuf-helpers.h

libtotemgstpixbufhelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libtotemgstpixbufhelpers_la_CFLAGS =	\
	$(PIXBUF_HELPER_CFLAGS)		\
	$(AM_CFLAGS)

libtotemgstpixbufhelpers_la_LIBADD = $(PIXBUF_HELPER_LIBS)
libtotemgstpixbufhelpers_la_LDFLAGS= -no-undefined

libtotemtimehelpers_la_SOURCES =	\
	totem-time-helpers.c		\
	totem-time-helpers.h

libtotemtimehelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libtotemtimehelpers_la_CFLAGS =	\
	$(TIME_HELPER_CFLAGS)	\
	$(AM_CFLAGS)

libtotemtimehelpers_la_LIBADD = $(TIME_HELPER_LIBS)
libtotemtimehelpers_la_LDFLAGS= -no-undefined

libtotemrtlhelpers_la_SOURCES =	\
	totem-rtl-helpers.c		\
	totem-rtl-helpers.h

libtotemrtlhelpers_la_CPPFLAGS =	\
	-D_REENTRANT			\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libtotemrtlhelpers_la_CFLAGS =	\
	$(RTL_HELPER_CFLAGS)	\
	$(AM_CFLAGS)

libtotemrtlhelpers_la_LIBADD = $(RTL_HELPER_LIBS)
libtotemrtlhelpers_la_LDFLAGS= -no-undefined

EXTRA_DIST = totem-time-helpers.h

-include $(top_srcdir)/git.mk
