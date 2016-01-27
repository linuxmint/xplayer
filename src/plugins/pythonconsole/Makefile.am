include $(top_srcdir)/src/plugins/Makefile.plugins

plugindir = $(PLUGINDIR)/pythonconsole
plugin_PYTHON = pythonconsole.py console.py

plugin_in_files = pythonconsole.plugin.in

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

gsettings_SCHEMAS = org.gnome.totem.plugins.pythonconsole.gschema.xml
CLEANFILES += $(gsettings_SCHEMAS)

convertdir = $(datadir)/GConf/gsettings
convert_DATA = pythonconsole.convert
EXTRA_DIST += pythonconsole.convert

-include $(top_srcdir)/git.mk
