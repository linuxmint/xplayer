noinst_PROGRAMS = bvw-test

noinst_LTLIBRARIES = libbaconvideowidget.la

bvw_test_SOURCES = bvw-test.c

bvw_test_CPPFLAGS = \
	-DDATADIR=\"$(pkgdatadir)\"	\
	-DLOGO_PATH=DATADIR\"\"G_DIR_SEPARATOR_S\"totem\"G_DIR_SEPARATOR_S\"totem_logo.png\"	\
	-DG_LOG_DOMAIN="\"bvw-test\"" \
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

bvw_test_CFLAGS =		\
	$(BACKEND_CFLAGS)	\
	$(AM_CFLAGS)

bvw_test_LDADD =		\
	libbaconvideowidget.la	\
	$(BACKEND_LIBS)

# Enums
BVW_ENUM_FILES = bacon-video-widget-enums.c bacon-video-widget-enums.h

bacon-video-widget-enums.h: bacon-video-widget.h Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef BVW_ENUMS_H\n#define BVW_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define BVW_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !BVW_ENUMS_H */" $(srcdir)/bacon-video-widget.h > $@)

bacon-video-widget-enums.c: bacon-video-widget.h Makefile bacon-video-widget-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"bacon-video-widget.h\"\n#include \"bacon-video-widget-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(srcdir)/bacon-video-widget.h > $@)

libbaconvideowidget_la_SOURCES = \
	$(BVW_MARSHAL_FILES)				\
	$(BVW_ENUM_FILES)				\
	bacon-video-widget.h				\
	video-utils.c video-utils.h			\
	bacon-video-widget.c				\
	bacon-video-widget-gst-missing-plugins.c	\
	bacon-video-widget-gst-missing-plugins.h	\
	totem-aspect-frame.h				\
	totem-aspect-frame.c				\
	gsd-osd-window.c				\
	gsd-osd-window.h				\
	gsd-osd-window-private.h			\
	bacon-video-osd-actor.c				\
	bacon-video-osd-actor.h

libbaconvideowidget_la_CPPFLAGS = \
	-D_REENTRANT				\
	-I$(top_srcdir)/src/gst/		\
	-I$(top_builddir)/src/backend		\
	$(DISABLE_DEPRECATED)			\
	$(AM_CPPFLAGS)

libbaconvideowidget_la_CFLAGS =	\
	$(BACKEND_CFLAGS)	\
	$(AM_CFLAGS)

libbaconvideowidget_la_LIBADD =					\
	$(top_builddir)/src/gst/libtotemgsthelpers.la		\
	$(top_builddir)/src/gst/libtotemgstpixbufhelpers.la	\
	$(BACKEND_LIBS)						\
	-lm

libbaconvideowidget_la_LDFLAGS= \
	-no-undefined

CLEANFILES = \
	$(BVW_MARSHAL_FILES)	\
	$(BVW_ENUM_FILES)

-include $(top_srcdir)/git.mk
