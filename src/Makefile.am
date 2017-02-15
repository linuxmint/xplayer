SUBDIRS = gst backend properties . plugins

bin_PROGRAMS = xplayer xplayer-video-thumbnailer xplayer-audio-preview
lib_LTLIBRARIES = libxplayer.la
noinst_LTLIBRARIES = libxplayer_player.la

AM_CPPFLAGS = \
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DXPLAYER_PLUGIN_DIR=\""$(libdir)/xplayer/plugins"\"\
	-DBINDIR=\""$(bindir)"\"			\
	-DGTKBUILDERDIR=\"""\"				\
	$(DISABLE_DEPRECATED)				\
	-I$(top_srcdir)/				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src/backend			\
	-I$(srcdir)/plugins

AM_CFLAGS = $(WARN_CFLAGS)

# Header files installed for use by plugins (and consequently introspected using gobject-introspection; see the bottom of the Makefile)
INST_H_FILES = \
	xplayer.h				\
	xplayer-interface.h		\
	plugins/xplayer-plugin.h		\
	plugins/xplayer-dirs.h

headerdir = $(includedir)/xplayer/$(XPLAYER_API_VERSION)
header_DATA = $(INST_H_FILES)

# Xplayer UI ltlibrary (used by browser plugins)
libxplayer_player_la_SOURCES = \
	$(INST_H_FILES)		\
	xplayer-interface.c	\
	xplayer-fullscreen.c	\
	xplayer-fullscreen.h	\
	gd-fullscreen-filter.c	\
	gd-fullscreen-filter.h	\
	xplayer-time-label.c	\
	xplayer-time-label.h

libxplayer_player_la_CPPFLAGS =		\
	-DG_LOG_DOMAIN=\""Xplayer"\"	\
	$(AM_CPPFLAGS)

libxplayer_player_la_LIBADD =		\
	$(PLAYER_LIBS)			\
	gst/libxplayertimehelpers.la	\
	gst/libxplayerrtlhelpers.la
libxplayer_player_la_CFLAGS =	\
	$(LIBPLAYER_CFLAGS)	\
	-I$(srcdir)/gst/	\
	$(AM_CFLAGS)

# Xplayer main library (used for main player; separate to allow gtk-doc to be used)
libxplayer_la_SOURCES = \
	$(XPLAYER_MARSHAL_FILES)		\
	$(INST_H_FILES)			\
	xplayer-object.c			\
	xplayer-private.h			\
	xplayer-preferences.c		\
	xplayer-preferences.h		\
	xplayer-dnd-menu.c		\
	xplayer-dnd-menu.h		\
	xplayer-options.c			\
	xplayer-options.h			\
	xplayer-playlist.c		\
	xplayer-playlist.h		\
	eggfileformatchooser.c		\
	eggfileformatchooser.h		\
	egg-macros.h			\
	xplayer-session.c			\
	xplayer-session.h			\
	xplayer-sidebar.c			\
	xplayer-sidebar.h			\
	xplayer-open-location.c		\
	xplayer-open-location.h		\
	xplayer-menu.c			\
	xplayer-menu.h			\
	xplayer-uri.c			\
	xplayer-uri.h			\
	xplayer-subtitle-encoding.c	\
	xplayer-subtitle-encoding.h	\
	xplayer-profile.h			\
	plugins/xplayer-plugins-engine.c	\
	plugins/xplayer-plugins-engine.h	\
	plugins/xplayer-dirs.c

libxplayer_la_CPPFLAGS = \
	-I$(top_builddir)/data		\
	-DG_LOG_DOMAIN=\""Xplayer"\"	\
	$(AM_CPPFLAGS)

libxplayer_la_CFLAGS = \
	$(PLAYER_CFLAGS)		\
	$(AM_CFLAGS)			\
	-I$(srcdir)/gst/

libxplayer_la_LDFLAGS = \
	-export-dynamic	\
	-no-undefined \
	$(AM_LDFLAGS)

libxplayer_la_LIBADD = \
	libxplayer_player.la		\
	backend/libbaconvideowidget.la	\
	$(PLAYER_LIBS)

if WITH_SMCLIENT
libxplayer_la_SOURCES += \
	eggsmclient.c \
	eggsmclient.h \
	eggsmclient-private.h \
	$(NULL)
libxplayer_la_CFLAGS += $(SMCLIENT_CFLAGS)
libxplayer_la_LIBADD += $(SMCLIENT_LIBS)

if WITH_SMCLIENT_XSMP
libxplayer_la_SOURCES += \
	eggdesktopfile.c \
	eggdesktopfile.h \
	eggsmclient-xsmp.c
libxplayer_la_CPPFLAGS += -DEGG_SM_CLIENT_BACKEND_XSMP
endif
if WITH_SMCLIENT_WIN32
libxplayer_la_SOURCES += eggsmclient-win32.c
endif
if WITH_SMCLIENT_QUARTZ
libxplayer_la_SOURCES += eggsmclient-osx.c
endif
endif

# Xplayer
xplayer_SOURCES = xplayer.c

xplayer_CPPFLAGS = \
	-DG_LOG_DOMAIN=\""Xplayer"\"	\
	$(AM_CPPFLAGS)

xplayer_CFLAGS =			\
	$(PLAYER_CFLAGS)	\
	-I$(srcdir)/gst/	\
	$(AM_CFLAGS)

xplayer_LDADD =		\
	libxplayer.la	\
	$(PLAYER_LIBS)

# Xplayer video thumbnailer
xplayer_video_thumbnailer_SOURCES = \
	xplayer-video-thumbnailer.c	\
	xplayer-resources.c		\
	xplayer-resources.h

xplayer_video_thumbnailer_CPPFLAGS = \
	-DG_LOG_DOMAIN=\""XplayerVideoThumbnailer"\"	\
	$(AM_CPPFLAGS)

xplayer_video_thumbnailer_CFLAGS =	\
	$(THUMBNAILER_CFLAGS)		\
	$(AM_CFLAGS)

xplayer_video_thumbnailer_LDADD =		\
	$(THUMBNAILER_LIBS)		\
	gst/libxplayergstpixbufhelpers.la	\
	gst/libxplayergsthelpers.la	\
	gst/libxplayertimehelpers.la	\
	-lm

# Xplayer Audio Preview for Nautilus
xplayer_audio_preview_SOURCES = \
	xplayer-audio-preview.c		\
	xplayer-resources.c		\
	xplayer-resources.h

xplayer_audio_preview_CPPFLAGS = \
	-I$(top_builddir)/data			\
	-I$(top_srcdir)/src/gst			\
	-DG_LOG_DOMAIN=\""XplayerAudioPreview"\"	\
	$(AM_CPPFLAGS)

xplayer_audio_preview_CFLAGS =	\
	$(PREVIEW_CFLAGS)	\
	$(AM_CFLAGS)

xplayer_audio_preview_LDADD =			\
	gst/libxplayergsthelpers.la		\
	$(PREVIEW_LIBS)

# Introspection
-include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

introspection_sources = $(INST_H_FILES) $(filter %.c,$(libxplayer_player_la_SOURCES) $(libxplayer_la_SOURCES))

# NOTE: This should change with $(XPLAYER_API_VERSION)
Xplayer-1.0.gir: libxplayer.la
Xplayer_1_0_gir_INCLUDES = GLib-2.0 GObject-2.0 Gtk-3.0 XplayerPlParser-1.0
Xplayer_1_0_gir_SCANNERFLAGS = --c-include=xplayer.h --pkg-export=xplayer
Xplayer_1_0_gir_CFLAGS =		\
	$(PLAYER_CFLAGS)	\
	$(libxplayer_la_CPPFLAGS)
Xplayer_1_0_gir_LIBS = libxplayer.la
Xplayer_1_0_gir_FILES = $(introspection_sources)
Xplayer_1_0_gir_EXPORT_PACKAGES = xplayer
INTROSPECTION_GIRS += Xplayer-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(gir_DATA) $(typelib_DATA)

endif

-include $(top_srcdir)/git.mk
