include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/media-player-keys
plugin_LTLIBRARIES = libmedia_player_keys.la

plugin_in_files = media-player-keys.plugin.in

libmedia_player_keys_la_SOURCES = \
	totem-media-player-keys.c
libmedia_player_keys_la_LDFLAGS = $(plugin_ldflags)
libmedia_player_keys_la_LIBADD = \
	$(plugin_libadd)		\
	$(MEDIA_PLAYER_KEYS_LIBS)
libmedia_player_keys_la_CFLAGS = \
	$(plugin_cflags)		\
	$(MEDIA_PLAYER_KEYS_CFLAGS)

-include $(top_srcdir)/git.mk
