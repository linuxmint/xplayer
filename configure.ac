AC_PREREQ[(2.64)]

m4_define(xplayer_version_major, 1)
m4_define(xplayer_version_minor, 0)
m4_define(xplayer_version_micro, 1)

# The version of the API exposed to plugins
m4_define(xplayer_api_version_major, 1)
m4_define(xplayer_api_version_minor, 0)

AC_INIT([xplayer],
        [xplayer_version_major.xplayer_version_minor.xplayer_version_micro],
        [http://github.com/linuxmint/xplayer],
        [xplayer],
        [http://github.com/linuxmint/xplayer/])

AM_MAINTAINER_MODE([enable])
AC_CONFIG_SRCDIR([src/xplayer.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 dist-xz tar-ustar no-dist-gzip check-news])

# Enable silent build when available (Automake 1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl Add the languages which your application supports to po/LINGUAS
GETTEXT_PACKAGE=xplayer
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define to the Gettext package name])
AC_SUBST(GETTEXT_PACKAGE)
IT_PROG_INTLTOOL([0.40.0])

# User Documentation
YELP_HELP_INIT

GOBJECT_INTROSPECTION_CHECK([0.6.7])

AC_PROG_CXX
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT
m4_ifdef([LT_OUTPUT], [LT_OUTPUT])

PKG_PROG_PKG_CONFIG

AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

# Requirements
GLIB_REQS=2.33.0
GIO_REQS=2.27.92
GTK_REQS=3.5.2
XPLAYER_PLPARSER_REQS=1.0.0
DBUS_REQS=0.82
VALA_REQS=0.14.1
PEAS_REQS=1.1.0
PYTHON_REQS=2.3
PYGOBJECT_REQS=2.90.3
GRILO_REQS=0.2.0
LIBXML_REQS=2.6.0

# Used in the pkg-config file for plugins
AC_SUBST(GLIB_REQS)
AC_SUBST(GTK_REQS)
AC_SUBST(XPLAYER_PLPARSER_REQS)

XPLAYER_VERSION_MAJOR=xplayer_version_major
XPLAYER_VERSION_MINOR=xplayer_version_minor
XPLAYER_VERSION_MICRO=xplayer_version_micro
AC_SUBST(XPLAYER_VERSION_MAJOR)
AC_SUBST(XPLAYER_VERSION_MINOR)
AC_SUBST(XPLAYER_VERSION_MICRO)

XPLAYER_API_VERSION=xplayer_api_version_major.xplayer_api_version_minor
AC_SUBST(XPLAYER_API_VERSION)
AC_DEFINE_UNQUOTED(XPLAYER_API_VERSION, ["$XPLAYER_API_VERSION"], [Define to the Xplayer plugin API version])

# The full list of plugins
allowed_plugins="apple-trailers autoload-subtitles brasero-disc-recorder chapters dbusservice im-status gromit lirc media-player-keys ontop opensubtitles properties pythonconsole recent rotation samplepython sample-vala screensaver screenshot sidebar-test skipto zeitgeist-dp grilo vimeo"

PLUGINDIR='${libdir}/xplayer/plugins'
AC_SUBST(PLUGINDIR)

dnl the two versions here implicate the gstreamer core and gst-plugins
dnl release versions.
GST_MAJORMINOR=1.0
GST_REQS=0.11.93
GST_PLUG_BASE_REQS=0.11.93
GSTPLUG_REQS=0.11.93
CLUTTER_REQS=1.10.0
CLUTTER_GTK_REQS=1.0.2

dnl Check for the required GStreamer versions for missing plugins
dnl installation, unless this has been disabled.
MISSING_PLUGINS_MODULES=""
AC_MSG_CHECKING([whether to enable easy codec installation support])
AC_ARG_ENABLE([easy-codec-installation],
	[
		AS_HELP_STRING([--enable-easy-codec-installation],
			[Whether to enable easy codec installation support for GStreamer]
		)
	], [
		case "${enableval}" in
			yes) enable_easy_codec_installation=yes ;;
			no)  enable_easy_codec_installation=no ;;
			*) enable_easy_codec_installation=auto ;;
		esac
	], [
		dnl default value
		enable_easy_codec_installation=auto
	]
)
AC_MSG_RESULT([$enable_easy_codec_installation])

if test "x$enable_easy_codec_installation" != "xno"; then
	MISSING_PLUGINS_MODULES="gstreamer-pbutils-1.0"
	PKG_CHECK_MODULES(MISSING_PLUGINS, $MISSING_PLUGINS_MODULES,
	[
		AC_DEFINE([ENABLE_MISSING_PLUGIN_INSTALLATION], 1,
		          [Whether we can and want to do installation of missing plugins])
	])
fi

# ================================================================
# Backend requirements
# ================================================================

PKG_CHECK_MODULES(CLUTTER_GST_3, clutter-gst-3.0,
	[
		AC_MSG_NOTICE([** Using clutter-gst-3.0])
		CLUTTER_GST="clutter-gst-3.0 >= 2.99.2"
		AC_DEFINE([HAVE_CLUTTER_GST_3], 1, [Whether we're using clutter-gst-3.0])
	],
	[
		AC_MSG_NOTICE([** Using clutter-gst-2.0])
		CLUTTER_GST="clutter-gst-2.0 >= 1.5.5"
	])

BACKEND_MODULES="gstreamer-1.0 >= $GST_REQS gstreamer-base-1.0 >= $GST_REQS gstreamer-plugins-base-1.0 >= $GST_PLUG_BASE_REQS $MISSING_PLUGINS_MODULES gstreamer-tag-1.0 >= $GSTPLUG_REQS gstreamer-audio-1.0 gstreamer-video-1.0 clutter-1.0 >= $CLUTTER_REQS $CLUTTER_GST clutter-gtk-1.0 cairo gsettings-desktop-schemas"

PKG_CHECK_MODULES(BACKEND, $BACKEND_MODULES)
PKG_CHECK_MODULES(BACKEND_TEST, $BACKEND_MODULES gtk+-3.0 >= $GTK_REQS)

dnl Check for required plugins
gst10_toolsdir=`$PKG_CONFIG --variable=toolsdir gstreamer-1.0`
gst10_inspect="$gst10_toolsdir/gst-inspect-1.0"

dnl Give error and exit if we don't have the gst_inspect tool
AC_MSG_CHECKING([GStreamer 1.0 inspection tool])
if test -r "$gst10_inspect"; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([
		Cannot find required GStreamer-1.0 tool 'gst-inspect-1.0'.
		It should be part of gstreamer-1_0-utils. Please install it.
	])
fi

dnl Check for elements from gst-plugins-base
dnl Set plugins which contain below elements
set -- playback videoscale
for base_element in playbin videoscale
do
	AC_MSG_CHECKING([GStreamer 1.0 $base_element element])
	if $gst10_inspect $base_element >/dev/null 2>/dev/null; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([
			Cannot find required GStreamer-1.0 plugin '$1'.
			It should be part of gst-plugins-base. Please install it.
		])

	fi
	shift;
done

dnl Check for elements from gst-plugins-good
dnl Set plugins which contain below elements
set -- autodetect goom scaletempo
for good_element in autoaudiosink goom scaletempo
do
	AC_MSG_CHECKING([GStreamer 1.0 $good_element element])
	if $gst10_inspect $good_element >/dev/null 2>/dev/null; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([
			Cannot find required GStreamer-1.0 plugin '$1'.
			It should be part of gst-plugins-good. Please install it.
		])
	fi
	shift;
done

dnl Check the smclient backend
AC_MSG_CHECKING([which smclient backend to use])
AC_ARG_WITH([smclient],
  [AS_HELP_STRING([--with-smclient],[which smclient backend to use (auto/no/xsmp/win32/quartz)])],
  [],
  [with_smclient=auto])

if test "$with_smclient" = "auto"; then
	# There could be several targets, given in any order (see bgo#652999)
	gdk_targets=`$PKG_CONFIG --variable targets gdk-3.0`
	for target in $gdk_targets; do
		case "$target" in
			x11)
				with_smclient=xsmp
				PKG_CHECK_MODULES([SMCLIENT],[sm ice])
				break
				;;
			win32)
				with_smclient=win32
				break
				;;
			quartz)
				with_smclient=quartz
				break
				;;
			*)
				with_smclient=no
				# Loop around and see if we find a better target next time
				;;
		esac
	done
fi

AC_MSG_RESULT([$with_smclient])

if test "$with_smclient" != "no"; then
	AC_DEFINE([WITH_SMCLIENT],[1],[Define if smclient is enabled])
fi

AM_CONDITIONAL([WITH_SMCLIENT],[test "$with_smclient" != "no"])
AM_CONDITIONAL([WITH_SMCLIENT_XSMP],[test "$with_smclient" = "xsmp"])
AM_CONDITIONAL([WITH_SMCLIENT_WIN32],[test "$with_smclient" = "win32"])
AM_CONDITIONAL([WITH_SMCLIENT_QUARTZ],[test "$with_smclient" = "quartz"])


# ================================================================
# Python plugins
# ================================================================
# We don't Python or PyGObject at compile time, but they are needed for the Python plugins at runtime, so we check anyway.
# We need pylint for `make check`.

AC_MSG_CHECKING([whether Python plugin support is requested])
AC_ARG_ENABLE([python],
	AS_HELP_STRING([--enable-python],[Enable python support]),
	[enable_python=$enableval],
	[enable_python=autodetect])
AC_MSG_RESULT([$enable_python])

have_python=no
have_pygobject=no
PYTHON_MODULES=""

# Test for the Python run time and PyGObject. Test for pylint as well, for `make check`.
if test "x$enable_python" != "xno"; then
	AM_PATH_PYTHON([$PYTHON_REQS],[have_python=yes],[have_python=no])
	if test "x$PYTHON" = "x:"; then
		have_python=no
	fi

	PYTHON_MODULES="pygobject-3.0 >= $PYGOBJECT_REQS"
	PKG_CHECK_MODULES([PYGOBJECT],[$PYTHON_MODULES],[have_pygobject=yes],[have_pygobject=no])
	AC_PATH_PROG([PYLINT], [pylint], [no])
fi

# Output the results of the Python checks
if test "x$have_python" != "xyes" -o "x$have_pygobject" != "xyes" -o "x$PYLINT" = "xno"; then
	if test "x$enable_python" = "xyes"; then
		AC_MSG_ERROR([python >= $PYTHON_REQS, $PYTHON_MODULES or pylint not found])
	elif test "x$enable_python" = "xautodetect"; then
		enable_python=no
		AC_MSG_WARN([python >= $PYTHON_REQS, $PYTHON_MODULES or pylint not found; disabling Python support])
	fi
elif test "x$enable_python" != "xno"; then
	enable_python=yes
fi

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

# ================================================================
# Player requirements
#================================================================

PKG_CHECK_MODULES(PLAYER, $BACKEND_MODULES glib-2.0 >= $GLIB_REQS gio-2.0 >= $GIO_REQS gtk+-3.0 >= $GTK_REQS gdk-x11-3.0 >= $GTK_REQS gmodule-2.0 xplayer-plparser >= $XPLAYER_PLPARSER_REQS libpeas-1.0 >= $PEAS_REQS libpeas-gtk-1.0 >= $PEAS_REQS $PYTHON_MODULES)
PKG_CHECK_MODULES(LIBPLAYER, glib-2.0 >= $GLIB_REQS gio-2.0 >= $GIO_REQS gtk+-3.0 >= $GTK_REQS gdk-x11-3.0 >= $GTK_REQS clutter-gtk-1.0)
PKG_CHECK_MODULES(HELPER, gstreamer-1.0 gstreamer-tag-1.0)
PKG_CHECK_MODULES(TIME_HELPER, glib-2.0)
PKG_CHECK_MODULES(RTL_HELPER, glib-2.0 gtk+-3.0 >= $GTK_REQS)
PKG_CHECK_MODULES(PIXBUF_HELPER, gdk-pixbuf-2.0 gstreamer-tag-1.0 >= $GSTPLUG_REQS gstreamer-video-1.0)
PKG_CHECK_MODULES(THUMBNAILER, gtk+-3.0 >= $GTK_REQS xplayer-plparser >= $XPLAYER_PLPARSER_REQS gstreamer-tag-1.0 >= $GSTPLUG_REQS gstreamer-video-1.0)
PKG_CHECK_MODULES(PREVIEW, gstreamer-1.0 >= $GST_REQS gio-2.0 >= $GIO_REQS gdk-pixbuf-2.0)
PKG_CHECK_MODULES(PROPERTIES, gtk+-3.0 >= $GTK_REQS)

LIBGD_INIT([
	tagged-entry
	main-icon-view
	static
 ])


dnl ================================================================
dnl Vala plugins
dnl ================================================================
AC_MSG_CHECKING([whether Vala plugin support is requested])
AC_ARG_ENABLE([vala],
	AS_HELP_STRING([--enable-vala],[Enable Vala plugin support]),
	[enable_vala=$enableval],
	[enable_vala=auto])
AC_MSG_RESULT([$enable_vala])
if test "x$enable_vala" != "xno"; then
	AM_PROG_VALAC([$VALA_REQS])
	if test "x$VALAC" = "x"; then
		if test "x$enable_vala" = "xyes"; then
			AC_MSG_ERROR([Vala plugin support explicitly requested, but not found])
		fi
		with_vala=no
	else
		with_vala=yes
	fi
fi

AM_CONDITIONAL(ENABLE_VALA, test "x$with_vala" = "xyes")

# ********************
# Movie player plugins
# ********************

plugin_error_or_ignore()
{
	if test "${error_on_bad_plugin}" = "1" ; then
		AC_MSG_ERROR([$1])
	else
		AC_MSG_WARN([$1 (disabling plugin)])
	fi
}

plugin_error()
{
	AC_MSG_ERROR([$1])
}

AC_ARG_WITH([plugins],
	[AS_HELP_STRING([--with-plugins],[Which Xplayer plugins to compile (default: auto; "all", "none" and "autodetect" are valid)])],
	[], [with_plugins=autodetect])

used_plugins=""
used_plugins2=""

if test "x${with_plugins}" = "xautodetect" ; then
	# Default to all plugins and just ignore them if they won't compile
	with_plugins="all"
	error_on_bad_plugin="0"
else
	# Clean up the plugin list and error if a plugin won't compile
	with_plugins=`echo ,"${with_plugins}", | sed -e 's/[[	,]][[	,]]*/ /g' -e 's/,$//'`
	error_on_bad_plugin="1"
fi

# Iterate through the wanted plugins and weed out bad ones (or error)
for wanted_plugin in ${with_plugins}; do
	for allowed_plugin in ${allowed_plugins}; do
		case ${wanted_plugin} in
			all)
				# List all plugins as being used
				used_plugins=${allowed_plugins}
			;;
			autodetect)
				# List all plugins as being used
				used_plugins=${allowed_plugins}
			;;
			none) used_plugins="";; # List no plugins
			${allowed_plugin}) used_plugins="${used_plugins} ${wanted_plugin}";; # List the plugin
			*) ;;
		esac
	done
done

# Check for plugin-specific requirements and error if necessary
for plugin in ${used_plugins}; do
	add_plugin="1"

	case ${plugin} in
		gromit)
			have_x11=no
			gdk_targets=$(pkg-config --variable=targets gtk+-3.0)
			for gdk_target in $gdk_targets; do
				if test x$gdk_target = xx11; then
					have_x11=yes
					break
				fi
			done

			if test "${have_x11}" != "yes" ; then
				plugin_error_or_ignore "the gromit plugin is not supported on non-X11 targets"
				add_plugin="0"
			fi
		;;
		media-player-keys)
			PKG_CHECK_MODULES(MEDIA_PLAYER_KEYS, gio-2.0,
				[HAVE_MEDIA_PLAYER_KEYS=yes], [HAVE_MEDIA_PLAYER_KEYS=no])
			if test "${HAVE_MEDIA_PLAYER_KEYS}" != "yes" ; then
				plugin_error "you need gio installed for the media-player-keys plugin"
				add_plugin="0"
			fi
		;;
		lirc)
			AC_CHECK_LIB(lirc_client, lirc_init,
				[AC_CHECK_HEADER(lirc/lirc_client.h,
				 	[LIRC_LIBS="-llirc_client"
					 HAVE_LIRC=yes],
					[
					 LIRC_LIBS=""
					 HAVE_LIRC=no])])
			AC_SUBST(LIRC_LIBS)
			if test "${HAVE_LIRC}" != "yes" ; then
				plugin_error_or_ignore "you need lirc_client installed for the lirc plugin"
				add_plugin="0"
			fi
		;;
		rotation)
			if test "${with_vala}" != "yes" ; then
				plugin_error_or_ignore "you need vala installed to use the rotation plugin"
				add_plugin="0"
                        elif test "x$found_introspection" != "xyes"; then
				plugin_error_or_ignore "you need introspection support for the rotation plugin"
				add_plugin="0"
			fi
		;;
		sample-vala)
			if test "${with_vala}" != "yes" ; then
				plugin_error_or_ignore "you need vala installed to use the sample-vala plugin"
				add_plugin="0"
                        elif test "x$found_introspection" != "xyes"; then
				plugin_error_or_ignore "you need introspection support for the sample-vala plugin"
				add_plugin="0"
			fi
		;;
		brasero-disc-recorder)
			dnl this is only needed to use brasero icons
			PKG_CHECK_MODULES(DISC_RECORDER, libxml-2.0 >= 2.6.0 gtk+-x11-3.0,
					[BUILD_RECORDER=yes], [BUILD_RECORDER=no])
			if test "${BUILD_RECORDER}" != "yes" ; then
				plugin_error_or_ignore "you need libxml-2.0 >= 2.6.0 and gtk+-x11-3.0 to use the brasero-disc-recorder plugin"
				add_plugin="0"
			fi
		;;
		chapters)
			PKG_CHECK_MODULES(CHAPTERS, libxml-2.0 >= $LIBXML_REQS gtk+-3.0 glib-2.0,
				[BUILD_CHAPTERS=yes], [BUILD_CHAPTERS=no])
			if test "${BUILD_CHAPTERS}" != "yes" ; then
				plugin_error "you need gtk+-3.0, glib-2.0 >= 2.15.0 and libxml-2.0 >= 2.6.0 to use the chapters plugin"
				add_plugin="0"
			fi
		;;
		zeitgeist-dp)
			if test "${with_vala}" != "yes" ; then
				plugin_error_or_ignore "you need vala installed to use the zeitgeist-dp plugin"
				add_plugin="0"
                        elif test "x$found_introspection" != "xyes"; then
				plugin_error_or_ignore "you need introspection support for the zeitgeist-dp plugin"
				add_plugin="0"
			else
				PKG_CHECK_MODULES(LIBZEITGEIST, zeitgeist-2.0 >= 0.9.12,
				[HAS_LIBZEITGEIST=yes], [HAS_LIBZEITGEIST=no])
				if test "${HAS_LIBZEITGEIST}" != "yes" ; then
					plugin_error_or_ignore "you need zeitgeist-2.0 >= 0.9.12 to use the zeitgeist-dp plugin"
					add_plugin="0"
				fi
			fi
		;;
		grilo)
			PKG_CHECK_MODULES(LIBGRILO, grilo-0.2 >= $GRILO_REQS,
				[HAVE_LIBGRILO=yes], [HAVE_LIBGRILO=no])
			if test "${HAVE_LIBGRILO}" != "yes" ; then
				plugin_error_or_ignore "you need libgrilo >= $GRILO_REQS installed for the Grilo plugin"
				add_plugin="0"
			fi
		;;
	esac

	# Add the specified plugin
	if test "${add_plugin}" = "1" ; then
		used_plugins2="${used_plugins2} ${plugin}"
	fi
done

ALL_PLUGINS=$allowed_plugins
PLUGINS=$used_plugins2

AC_MSG_CHECKING([which plugins to compile])
AC_MSG_RESULT([$PLUGINS])

AC_SUBST([ALL_PLUGINS])
AC_SUBST([PLUGINS])

# ***************
# Browser plugins
# ***************

AC_MSG_CHECKING([whether to compile the browser plugins])

AC_ARG_ENABLE([browser-plugins],
	[AS_HELP_STRING([--enable-browser-plugins],[compile the xplayer browser plugins])],
	[],[enable_browser_plugins=yes])
AC_MSG_RESULT([$enable_browser_plugins])

if test "$enable_browser_plugins" = "yes" ; then
	AC_DEFINE([ENABLE_BROWSER_PLUGINS],[1],[Define to build the browser plugin])

	BROWSER_PLUGIN_DIR="${BROWSER_PLUGIN_DIR:-"\${libdir}/mozilla/plugins"}"
	AC_ARG_VAR([BROWSER_PLUGIN_DIR],[Where to install the plugin to])

	PKG_CHECK_MODULES([BROWSER_PLUGIN],
		[glib-2.0
		 gio-2.0 >= $GIO_REQS
		 gmodule-export-2.0
		 x11
		 xplayer-plparser-mini >= $XPLAYER_PLPARSER_REQS],
		[],[enable_browser_plugins=no])

	AC_SUBST([BROWSER_PLUGIN_CFLAGS])
	AC_SUBST([BROWSER_PLUGIN_LIBS])

	# Earlier versions misdetect playlists
	PKG_CHECK_EXISTS([shared-mime-info >= 0.22],
			[],
			[AC_MSG_ERROR([
				The Browser plugin requires shared-mime-info
				>= 0.22 to function.])])

	PKG_CHECK_MODULES([DBUS], [dbus-glib-1 >= $DBUS_REQS],,
			[enable_browser_plugins=no])
	DBUSLIBDIR="`$PKG_CONFIG dbus-glib-1 --variable=libdir`"
	DBUSBINDIR="`echo $DBUSLIBDIR | sed -e s/lib/bin/`"
	AC_PATH_PROG([DBUS_BIND], [dbus-binding-tool], [no], [$DBUSBINDIR:$PATH])
	if test "x$DBUS_BIND" = "xno"; then
		AC_MSG_WARN([dbus-binding-tool not found])
		enable_browser_plugins=no
	fi

        # check for -fno-rtti flag

	AC_LANG_PUSH([C++])
	__SAVE_CXXFLAGS=$CXXFLAGS
	CXXFLAGS="-fno-rtti $CXXFLAGS"

	AC_MSG_CHECKING([for -fno-rtti compiler flag])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],[have_no_rtti_flag=yes],[have_no_rtti_flag=no])
	AC_MSG_RESULT([$have_no_rtti_flag])

	CXXFLAGS=$__SAVE_CXXFLAGS
	AC_LANG_POP([C++])

	if test "$have_no_rtti_flag" = "yes"; then
		AM_CXXFLAGS="-fno-rtti $AM_CXXFLAGS"
	fi

	PKG_CHECK_MODULES(PLUGIN_VIEWER, $BACKEND_MODULES gtk+-3.0 >= $GTK_REQS xplayer-plparser >= $XPLAYER_PLPARSER_REQS gdk-x11-3.0 >= $GTK_REQS dbus-glib-1 >= $DBUS_REQS)
	PKG_CHECK_MODULES(GLOW_BUTTON, gtk+-3.0)
fi

AM_CONDITIONAL([ENABLE_BROWSER_PLUGINS], [test "$enable_browser_plugins" = "yes"])

# check which plugins to enable

if test "$enable_browser_plugins" = "yes"; then

AC_MSG_CHECKING([whether to enable the GMP browser plugin])
AC_ARG_ENABLE([gmp-plugin],
	AS_HELP_STRING([--enable-gmp-plugin],[Whether to enable the GMP browser plugin]),
	[],[enable_gmp_plugin=yes])
AC_MSG_RESULT([$enable_gmp_plugin])

AC_MSG_CHECKING([whether to enable the NarrowSpace browser plugin])
AC_ARG_ENABLE([narrowspace-plugin],
	AS_HELP_STRING([--enable-narrowspace-plugin],[Whether to enable the NarrowSpace browser plugin]),
	[],[enable_narrowspace_plugin=yes])
AC_MSG_RESULT([$enable_narrowspace_plugin])

AC_MSG_CHECKING([whether to enable the MullY browser plugin])
AC_ARG_ENABLE([mully-plugin],
	AS_HELP_STRING([--enable-mully-plugin],[Whether to enable the MullY browser plugin]),
	[],[enable_mully_plugin=yes])
AC_MSG_RESULT([$enable_mully_plugin])

AC_MSG_CHECKING([whether to enable the Cone browser plugin])
AC_ARG_ENABLE([cone-plugin],
	AS_HELP_STRING([--enable-cone-plugin],[Whether to enable the Cone browser plugin]),
	[],[enable_cone_plugin=yes])
AC_MSG_RESULT([$enable_cone_plugin])

AC_MSG_CHECKING([whether to enable the Vegas browser plugin])
AC_ARG_ENABLE([vegas-plugin],
	AS_HELP_STRING([--enable-vegas-plugin],[Whether to enable the Vegas browser plugin]),
	[],[enable_vegas_plugin=yes])
AC_MSG_RESULT([$enable_vegas_plugin])

fi

AM_CONDITIONAL([ENABLE_GMP_PLUGIN],[test "$enable_gmp_plugin" = "yes"])
AM_CONDITIONAL([ENABLE_NARROWSPACE_PLUGIN],[test "$enable_narrowspace_plugin" = "yes"])
AM_CONDITIONAL([ENABLE_MULLY_PLUGIN],[test "$enable_mully_plugin" = "yes"])
AM_CONDITIONAL([ENABLE_CONE_PLUGIN],[test "$enable_cone_plugin" = "yes"])
AM_CONDITIONAL([ENABLE_VEGAS_PLUGIN],[test "$enable_vegas_plugin" = "yes"])

GLIB_GSETTINGS

GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS
GNOME_MAINTAINER_MODE_DEFINES
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

# This macro expands DIR and assigns it to RET.
# If DIR is NONE, then it's replaced by DEFAULT.
# Based on AC_DEFINE_DIR
AC_DEFUN([XPLAYER_FULLPATH], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_expand=[$]$1
  test "x$ac_expand" = xNONE && ac_expand="[$]$2"
  ac_expand=`eval echo [$]ac_expand`
  $3=`eval echo [$]ac_expand`
])

dnl FULL_LIBEXECDIR is used for X-GNOME-Bugzilla-ExtraInfoScript expansion
dnl in data/xplayer.desktop.in.in.in
XPLAYER_FULLPATH(libexecdir, NONE, FULL_LIBEXECDIR)
AC_SUBST(FULL_LIBEXECDIR)

dnl run in source tree
AC_ARG_ENABLE(run-in-source-tree,
	      AS_HELP_STRING([--disable-run-in-source-tree],[disable search for UI files and plugins in local directory ]),
	[case "${enableval}" in
	yes) ENABLE_RUN_IN_SOURCE_TREE=yes ;;
	no)  ENABLE_RUN_IN_SOURCE_TREE=no ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --disable-run-in-source-tree) ;;
	esac],
[ENABLE_RUN_IN_SOURCE_TREE=yes]) dnl Default value
if test x$ENABLE_RUN_IN_SOURCE_TREE = xyes; then
	AC_DEFINE(XPLAYER_RUN_IN_SOURCE_TREE, 1, [enable search for UI files and plugins in local directory])
fi

if test "$GCC" = "yes" ; then
	CFLAGS="\
	-Wall \
	-Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	-Wnested-externs -Wpointer-arith \
	-Wcast-align -Wsign-compare \
	-fno-strict-aliasing \
	$CFLAGS"
fi

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([
Makefile
xplayer.spec
libgd/Makefile
src/Makefile
src/gst/Makefile
src/properties/Makefile
src/plugins/Makefile
src/plugins/apple-trailers/Makefile
src/plugins/autoload-subtitles/Makefile
src/plugins/dbusservice/Makefile
src/plugins/screensaver/Makefile
src/plugins/screenshot/Makefile
src/plugins/ontop/Makefile
src/plugins/im-status/Makefile
src/plugins/gromit/Makefile
src/plugins/lirc/Makefile
src/plugins/media-player-keys/Makefile
src/plugins/opensubtitles/Makefile
src/plugins/opensubtitles/org.x.player.plugins.opensubtitles.gschema.xml.in
src/plugins/properties/Makefile
src/plugins/recent/Makefile
src/plugins/rotation/Makefile
src/plugins/sidebar-test/Makefile
src/plugins/skipto/Makefile
src/plugins/samplepython/Makefile
src/plugins/sample-vala/Makefile
src/plugins/pythonconsole/Makefile
src/plugins/pythonconsole/org.x.player.plugins.pythonconsole.gschema.xml.in
src/plugins/brasero-disc-recorder/Makefile
src/plugins/chapters/Makefile
src/plugins/zeitgeist-dp/Makefile
src/plugins/grilo/Makefile
src/plugins/vimeo/Makefile
src/backend/Makefile
browser-plugin/Makefile
data/Makefile
data/org.x.player.gschema.xml.in
data/xplayer.desktop.in.in
data/xplayer.pc
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/64x64/Makefile
data/icons/96x96/Makefile
data/icons/scalable/Makefile
po/Makefile.in
help/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
])
AC_OUTPUT

AC_MSG_NOTICE([Xplayer was configured with the following options:])
AC_MSG_NOTICE([** Using the GStreamer-$GST_MAJORMINOR backend])
if test "x$MISSING_PLUGINS_LIBS" != "x"; then
	AC_MSG_NOTICE([** Easy codec installation support enabled])
else
	AC_MSG_NOTICE([   Easy codec installation support disabled])
fi
if test x$enable_python = xyes ; then
	AC_MSG_NOTICE([** Python plugin support enabled])
else
	AC_MSG_NOTICE([   Python plugin support disabled])
fi
if test x$with_vala = xyes ; then
	AC_MSG_NOTICE([** Vala plugin support enabled])
else
	AC_MSG_NOTICE([   Vala plugin support disabled])
fi
if test "x${PLUGINS}" != "x" ; then
	for allowed_plugin in ${ALL_PLUGINS}; do
		for plugin in ${PLUGINS}; do
			case ${allowed_plugin} in
				${plugin})
					AC_MSG_NOTICE([** ${allowed_plugin} plugin enabled])
					continue 2
				;;
				*);;
			esac
		done
		AC_MSG_NOTICE([   ${allowed_plugin} plugin disabled])
	done
else
	AC_MSG_NOTICE([   No Xplayer plugins enabled])
fi
if test x$enable_browser_plugins = xyes ; then
	AC_MSG_NOTICE([** Browser plugin enabled])
	if test x$enable_gmp_plugin = xyes ; then
		AC_MSG_NOTICE([** GMP (Windows Media) plugin enabled])
	else
		AC_MSG_NOTICE([   GMP (Windows Media) plugin disabled])
	fi
	if test x$enable_narrowspace_plugin = xyes ; then
		AC_MSG_NOTICE([** NarrowSpace (QuickTime) plugin enabled])
	else
		AC_MSG_NOTICE([   NarrowSpace (QuickTime) plugin disabled])
	fi
	if test x$enable_mully_plugin = xyes ; then
		AC_MSG_NOTICE([** MullY (DivX) plugin enabled])
	else
		AC_MSG_NOTICE([   MullY (DivX) plugin disabled])
	fi
	if test x$enable_cone_plugin = xyes ; then
		AC_MSG_NOTICE([** Cone (VLC) plugin enabled])
	else
		AC_MSG_NOTICE([   Cone (VLC) plugin disabled])
	fi
	if test x$enable_vegas_plugin = xyes ; then
		AC_MSG_NOTICE([** Vegas (Flash) plugin enabled])
	else
		AC_MSG_NOTICE([   Vegas (Flash) plugin disabled])
	fi
else
	AC_MSG_NOTICE([   Browser plugin disabled])
fi
if test "$with_smclient" != "no" ; then
	AC_MSG_NOTICE([** SM client support enabled])
else
	AC_MSG_NOTICE([   SM client support disabled])
fi

AC_MSG_NOTICE([End options])

