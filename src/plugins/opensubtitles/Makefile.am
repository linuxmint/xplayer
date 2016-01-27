include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/opensubtitles
plugin_PYTHON = opensubtitles.py hash.py

plugin_in_files = opensubtitles.plugin.in

uidir = $(plugindir)
ui_DATA = opensubtitles.ui
EXTRA_DIST += $(ui_DATA)

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

gsettings_SCHEMAS = org.gnome.totem.plugins.opensubtitles.gschema.xml
CLEANFILES += $(gsettings_SCHEMAS)

convertdir = $(datadir)/GConf/gsettings
convert_DATA = opensubtitles.convert
EXTRA_DIST += opensubtitles.convert

-include $(top_srcdir)/git.mk
