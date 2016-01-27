include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/sidebar-test
plugin_LTLIBRARIES = libsidebar-test.la

plugin_in_files = sidebar-test.plugin.in

libsidebar_test_la_SOURCES = totem-sidebar-test.c
libsidebar_test_la_LDFLAGS = $(plugin_ldflags)
libsidebar_test_la_CFLAGS = $(plugin_cflags)
libsidebar_test_la_LIBADD = $(plugin_libadd)

# Override to _not_ install the test plugins
install-pluginLTLIBRARIES:
install-pluginDATA:
uninstall-pluginLTLIBRARIES:
uninstall-pluginDATA:

-include $(top_srcdir)/git.mk
