include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/sample-vala
plugin_LTLIBRARIES = libsample-vala.la

plugin_in_files = sample-vala.plugin.in

# here we are explicitly specifying gtk+-3.0 to use the vapi because vala still
# cannot parse the gir
AM_VALAFLAGS = \
	--girdir=$(top_srcdir)/src	\
	--pkg Totem-1.0 --pkg libpeas-1.0 --pkg gtk+-3.0

libsample_vala_la_SOURCES = totem-sample-vala-plugin.vala
libsample_vala_la_LDFLAGS = $(plugin_ldflags)
libsample_vala_la_CFLAGS = $(plugin_cflags)
libsample_vala_la_LIBADD = $(plugin_libadd)

GITIGNOREFILES = \
	$(libsample_vala_la_SOURCES:.vala=.c) \
	libsample_vala_la_vala.stamp \
	$(NULL)

# Override to _not_ install the test plugins
install-pluginLTLIBRARIES:
install-pluginDATA:
uninstall-pluginLTLIBRARIES:
uninstall-pluginDATA:

-include $(top_srcdir)/git.mk
