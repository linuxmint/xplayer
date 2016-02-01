plugindir = $(BROWSER_PLUGIN_DIR)
plugin_LTLIBRARIES =

EXTRA_DIST = \
	plugin.symbols	\
	TODO		\
	README.browser-plugin

AM_CPPFLAGS = \
	-I$(top_srcdir)					\
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src				\
	-I$(top_builddir)/src/backend			\
	-DSYSCONFDIR="\"$(sysconfdir)\""		\
	-DLIBEXECDIR="\"$(libexecdir)\""

plugin_sources = \
	npapi.h				\
	npruntime.h			\
	npupp.h				\
	xplayerNPNGlue.cpp		\
	xplayerNPClass.cpp		\
	xplayerNPClass.h			\
	xplayerNPObject.cpp		\
	xplayerNPObject.h			\
	xplayerNPObjectWrapper.h		\
	xplayerNPVariantWrapper.h		\
	xplayerPluginGlue.cpp		\
	xplayerPlugin.cpp			\
	xplayerPlugin.h			\
	xplayer-plugin-viewer-constants.h

plugin_cxxflags = \
	$(BROWSER_PLUGIN_CFLAGS)	\
	$(WARN_CXXFLAGS)		\
	$(PLUGIN_EXTRA_WARN_CXXFLAGS)	\
	$(AM_CXXFLAGS)

plugin_libadd = $(BROWSER_PLUGIN_LIBS)

plugin_ldflags = \
	-avoid-version					\
	-export-symbols $(srcdir)/plugin.symbols	\
	-module						\
	-no-undefined					\
	$(AM_LDFLAGS)

# The Glow button test program
noinst_PROGRAMS = test-glow-button

test_glow_button_SOURCES = \
	xplayer-glow-button.c	\
	xplayer-glow-button.h	\
	test-glow-button.c

test_glow_button_LDADD =	\
	$(GLOW_BUTTON_LIBS)	\
	-lm

test_glow_button_CFLAGS =	\
	$(GLOW_BUTTON_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

# Xplayer Plugin Viewer
libexec_PROGRAMS = xplayer-plugin-viewer

# Are we a bit thick, Mister?
xplayer-plugin-viewer.c: xplayer-plugin-viewer-interface.h $(MARSHAL_FILES)

xplayer_plugin_viewer_SOURCES = \
	xplayer-glow-button.c		\
	xplayer-glow-button.h		\
	xplayer-plugin-viewer-interface.h	\
	xplayer-plugin-viewer.c		\
	xplayer-plugin-viewer-constants.h	\
	xplayer-plugin-viewer-options.h	\
	$(MARSHAL_FILES)

xplayer_plugin_viewer_CPPFLAGS = \
	-DGNOMELOCALEDIR="\"$(datadir)/locale\""	\
	-DG_LOG_DOMAIN=\""XplayerEmbedded\""		\
	-I$(top_srcdir)/src/gst				\
	$(AM_CPPFLAGS)

xplayer_plugin_viewer_CFLAGS = \
	$(PLUGIN_VIEWER_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

xplayer_plugin_viewer_LDADD = \
	$(top_builddir)/src/backend/libbaconvideowidget.la		\
	$(top_builddir)/src/libxplayer_player.la				\
	$(PLUGIN_VIEWER_LIBS)

xplayer_plugin_viewer_LDFLAGS = -export-dynamic

xplayer-plugin-viewer-interface.h: org_x_player_PluginViewer.xml
	$(DBUS_BIND) --prefix=xplayer_embedded --mode=glib-server $< > $@

CLEANFILES = xplayer-plugin-viewer-interface.h
EXTRA_DIST += org_x_player_PluginViewer.xml

# Xplayer GMP Plugin
if ENABLE_GMP_PLUGIN
plugin_LTLIBRARIES += libxplayer-gmp-plugin.la

libxplayer_gmp_plugin_la_SOURCES = \
	$(plugin_sources)		\
	xplayerGMPControls.cpp		\
	xplayerGMPControls.h		\
	xplayerGMPError.cpp		\
	xplayerGMPError.h			\
	xplayerGMPNetwork.cpp		\
	xplayerGMPNetwork.h		\
	xplayerGMPPlayer.cpp		\
	xplayerGMPPlayer.h		\
	xplayerGMPPlaylist.cpp		\
	xplayerGMPPlaylist.h		\
	xplayerGMPSettings.cpp		\
	xplayerGMPSettings.h

libxplayer_gmp_plugin_la_CPPFLAGS = \
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DXPLAYER_GMP_PLUGIN				\
	-DXPLAYER_PL_PARSER_MINI				\
	$(AM_CPPFLAGS)

libxplayer_gmp_plugin_la_CXXFLAGS = $(plugin_cxxflags)
libxplayer_gmp_plugin_la_LIBADD = $(plugin_libadd)
libxplayer_gmp_plugin_la_LDFLAGS = $(plugin_ldflags)
endif

# Xplayer NarrowSpace Plugin
if ENABLE_NARROWSPACE_PLUGIN
plugin_LTLIBRARIES += libxplayer-narrowspace-plugin.la

libxplayer_narrowspace_plugin_la_SOURCES = \
	$(plugin_sources)		\
	xplayerNarrowSpacePlugin.cpp	\
	xplayerNarrowSpacePlugin.h

libxplayer_narrowspace_plugin_la_CPPFLAGS = \
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DXPLAYER_NARROWSPACE_PLUGIN			\
	-DXPLAYER_PL_PARSER_MINI				\
	$(AM_CPPFLAGS)

libxplayer_narrowspace_plugin_la_CXXFLAGS = $(plugin_cxxflags)
libxplayer_narrowspace_plugin_la_LIBADD = $(plugin_libadd)
libxplayer_narrowspace_plugin_la_LDFLAGS = $(plugin_ldflags)
endif

# Xplayer Vegas Plugin
if ENABLE_VEGAS_PLUGIN
plugin_LTLIBRARIES += libxplayer-vegas-plugin.la

libxplayer_vegas_plugin_la_SOURCES = \
	$(plugin_sources)		\
	xplayerVegasPlugin.cpp	\
	xplayerVegasPlugin.h

libxplayer_vegas_plugin_la_CPPFLAGS = \
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DXPLAYER_VEGAS_PLUGIN			\
	-DXPLAYER_PL_PARSER_MINI				\
	$(AM_CPPFLAGS)

libxplayer_vegas_plugin_la_CXXFLAGS = $(plugin_cxxflags)
libxplayer_vegas_plugin_la_LIBADD = $(plugin_libadd)
libxplayer_vegas_plugin_la_LDFLAGS = $(plugin_ldflags)
endif

# Xplayer MULLY Plugin
if ENABLE_MULLY_PLUGIN
plugin_LTLIBRARIES += libxplayer-mully-plugin.la

libxplayer_mully_plugin_la_SOURCES = \
	$(plugin_sources)		\
	xplayerMullYPlugin.cpp		\
	xplayerMullYPlugin.h

libxplayer_mully_plugin_la_CPPFLAGS = \
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DXPLAYER_MULLY_PLUGIN				\
	-DXPLAYER_PL_PARSER_MINI				\
	$(AM_CPPFLAGS)

libxplayer_mully_plugin_la_CXXFLAGS = $(plugin_cxxflags)
libxplayer_mully_plugin_la_LIBADD = $(plugin_libadd)
libxplayer_mully_plugin_la_LDFLAGS = $(plugin_ldflags)
endif

# Xplayer Cone Plugin
if ENABLE_CONE_PLUGIN
plugin_LTLIBRARIES += libxplayer-cone-plugin.la

libxplayer_cone_plugin_la_SOURCES = \
	$(plugin_sources)		\
	xplayerConeAudio.cpp		\
	xplayerConeAudio.h		\
	xplayerCone.cpp			\
	xplayerCone.h			\
	xplayerConeInput.cpp		\
	xplayerConeInput.h		\
	xplayerConePlaylist.cpp		\
	xplayerConePlaylist.h		\
	xplayerConePlaylistItems.cpp	\
	xplayerConePlaylistItems.h	\
	xplayerConeVideo.cpp		\
	xplayerConeVideo.h

libxplayer_cone_plugin_la_CPPFLAGS = \
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DXPLAYER_CONE_PLUGIN				\
	-DXPLAYER_PL_PARSER_MINI				\
	$(AM_CPPFLAGS)

libxplayer_cone_plugin_la_CXXFLAGS = $(plugin_cxxflags)
libxplayer_cone_plugin_la_LIBADD = $(plugin_libadd)
libxplayer_cone_plugin_la_LDFLAGS = $(plugin_ldflags)
endif

-include $(top_srcdir)/git.mk
