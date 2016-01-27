include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/zeitgeist-dp
plugin_LTLIBRARIES = libxplayer-zeitgeist-dp-plugin.la

plugin_in_files = zeitgeist-dp.plugin.in

# here we are explicitly specifying gtk+-3.0 to use the vapi because vala still
# cannot parse the gir
AM_VALAFLAGS = \
	--girdir=$(top_builddir)/src	\
	--pkg Xplayer-1.0 --pkg Peas-1.0 --pkg gtk+-3.0 \
	--pkg zeitgeist-2.0 \
	bacon-video.vapi

libxplayer_zeitgeist_dp_plugin_la_SOURCES = xplayer-zeitgeist-dp-plugin.vala
libxplayer_zeitgeist_dp_plugin_la_LDFLAGS = $(plugin_ldflags)
libxplayer_zeitgeist_dp_plugin_la_LIBADD = \
	$(plugin_libadd)		\
	$(LIBZEITGEIST_LIBS)
libxplayer_zeitgeist_dp_plugin_la_CFLAGS = \
	$(plugin_cflags) \
	$(LIBZEITGEIST_CFLAGS) \
	-I $(top_srcdir)/src/backend

GITIGNOREFILES = \
	$(libxplayer_zeitgeist_dp_plugin_la_SOURCES:.vala=.c) \
	libxplayer_zeitgeist_dp_plugin_la_vala.stamp \
	$(NULL)

-include $(top_srcdir)/git.mk
