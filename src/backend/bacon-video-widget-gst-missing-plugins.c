/* xplayer-missing-plugins.c

   Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Tim-Philipp Müller <tim centricular net>
 */

#include "config.h"

#include "bacon-video-widget-gst-missing-plugins.h"

#define GST_USE_UNSTABLE_API 1
#include <gst/gst.h> /* for gst_registry_update and functions in bacon_video_widget_gst_missing_plugins_blacklist */

#ifdef ENABLE_MISSING_PLUGIN_INSTALLATION

#include "bacon-video-widget.h"

#include <gst/pbutils/pbutils.h>
#include <gst/pbutils/install-plugins.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gtk/gtkx.h>
#endif

#include <string.h>

GST_DEBUG_CATEGORY_EXTERN (_xplayer_gst_debug_cat);
#define GST_CAT_DEFAULT _xplayer_gst_debug_cat

/* list of blacklisted detail strings */
static GList *blacklisted_plugins = NULL;

typedef struct
{
	gboolean   playing;
	gchar    **descriptions;
	gchar    **details;
	BaconVideoWidget *bvw;
}
XplayerCodecInstallContext;

#ifdef GDK_WINDOWING_X11
/* Adapted from xplayer-interface.c */
static Window
bacon_video_widget_gtk_plug_get_toplevel (GtkPlug *plug)
{
	Window root, parent, *children;
	guint nchildren;
	Window xid;

	g_return_val_if_fail (GTK_IS_PLUG (plug), 0);

	xid = gtk_plug_get_id (plug);

	do
	{
		/* FIXME: multi-head */
		if (XQueryTree (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xid, &root,
					&parent, &children, &nchildren) == 0)
		{
			g_warning ("Couldn't find window manager window");
			return 0;
		}

		if (root == parent)
			return xid;

		xid = parent;
	}
	while (TRUE);
}

static Window
bacon_video_widget_gst_get_toplevel (GtkWidget *widget)
{
	GtkWidget *parent;

	parent = gtk_widget_get_toplevel (GTK_WIDGET (widget));
	if (parent == NULL)
		return 0;

	if (GTK_IS_PLUG (parent))
		return bacon_video_widget_gtk_plug_get_toplevel (GTK_PLUG (parent));
	else
		return GDK_WINDOW_XID(gtk_widget_get_window (parent));
}
#endif

static gboolean
bacon_video_widget_gst_codec_install_plugin_is_blacklisted (const gchar * detail)
{
	GList *res;

	res = g_list_find_custom (blacklisted_plugins,
	                          detail,
	                          (GCompareFunc) strcmp);

	return (res != NULL);	
}

static void
bacon_video_widget_gst_codec_install_blacklist_plugin (const gchar * detail)
{
	if (!bacon_video_widget_gst_codec_install_plugin_is_blacklisted (detail))
	{
		blacklisted_plugins = g_list_prepend (blacklisted_plugins,
		                                      g_strdup (detail));
	}
}

static void
bacon_video_widget_gst_codec_install_context_free (XplayerCodecInstallContext *ctx)
{
	g_strfreev (ctx->descriptions);
	g_strfreev (ctx->details);
	g_free (ctx);
}

static void
on_plugin_installation_done (GstInstallPluginsReturn res, gpointer user_data)
{
	XplayerCodecInstallContext *ctx = (XplayerCodecInstallContext *) user_data;
	gchar **p;

	GST_INFO ("res = %d (%s)", res, gst_install_plugins_return_get_name (res));

	switch (res)
	{
		/* treat partial success the same as success; in the worst case we'll
		 * just do another round and get NOT_FOUND as result that time */
		case GST_INSTALL_PLUGINS_PARTIAL_SUCCESS:
		case GST_INSTALL_PLUGINS_SUCCESS:
			{
				/* blacklist installed plugins too, so that we don't get
				 * into endless installer loops in case of inconsistencies */
				for (p = ctx->details; p != NULL && *p != NULL; ++p)
					bacon_video_widget_gst_codec_install_blacklist_plugin (*p);

				bacon_video_widget_stop (ctx->bvw);
				g_message ("Missing plugins installed. Updating plugin registry ...");

				/* force GStreamer to re-read its plugin registry */
				if (gst_update_registry ())
				{
					g_message ("Plugin registry updated, trying again.");
					bacon_video_widget_play (ctx->bvw, NULL);
				} else {
					g_warning ("GStreamer registry update failed");
					/* FIXME: should we show an error message here? */
				}
			}
			break;
		case GST_INSTALL_PLUGINS_NOT_FOUND:
			{
				g_message ("No installation candidate for missing plugins found.");

				/* NOT_FOUND should only be returned if not a single one of the
				 * requested plugins was found; if we managed to play something
				 * anyway, we should just continue playing what we have and
				 * blacklist the requested plugins for this session; if we
				 * could not play anything we should blacklist them as well,
				 * so the install wizard isn't called again for nothing */
				for (p = ctx->details; p != NULL && *p != NULL; ++p)
					bacon_video_widget_gst_codec_install_blacklist_plugin (*p);

				if (ctx->playing)
				{
					bacon_video_widget_play (ctx->bvw, NULL);
				} else {
					/* wizard has not shown error, do stop/play again,
					 * so that an error message gets shown */
					bacon_video_widget_stop (ctx->bvw);
					bacon_video_widget_play (ctx->bvw, NULL);
				}
			}
			break;
		case GST_INSTALL_PLUGINS_USER_ABORT:
			{
				/* blacklist on user abort, so we show an error next time (or
				 * just play what we can) instead of calling the installer */
				for (p = ctx->details; p != NULL && *p != NULL; ++p)
					bacon_video_widget_gst_codec_install_blacklist_plugin (*p);

				if (ctx->playing) {
					bacon_video_widget_play (ctx->bvw, NULL);
				} else {
					/* if we couldn't play anything, do stop/play again,
					 * so that an error message gets shown */
					bacon_video_widget_stop (ctx->bvw);
					bacon_video_widget_play (ctx->bvw, NULL);
				}
			}
			break;
		case GST_INSTALL_PLUGINS_INVALID:
		case GST_INSTALL_PLUGINS_ERROR:
		case GST_INSTALL_PLUGINS_CRASHED:
		default:
			{
				g_message ("Missing plugin installation failed: %s",
				           gst_install_plugins_return_get_name (res));

				if (ctx->playing)
					bacon_video_widget_play (ctx->bvw, NULL);
				else
					bacon_video_widget_stop (ctx->bvw);
				break;
			}
		case GST_INSTALL_PLUGINS_STARTED_OK:
		case GST_INSTALL_PLUGINS_INTERNAL_FAILURE:
		case GST_INSTALL_PLUGINS_HELPER_MISSING:
		case GST_INSTALL_PLUGINS_INSTALL_IN_PROGRESS:
			{
				g_assert_not_reached ();
				break;
			}
	}

	bacon_video_widget_gst_codec_install_context_free (ctx);
}

#ifdef GDK_WINDOWING_X11
static void
set_startup_notification_id (GstInstallPluginsContext *install_ctx)
{
	gchar *startup_id;
	guint32 timestamp;

	timestamp = gtk_get_current_event_time ();
	startup_id = g_strdup_printf ("_TIME%u", timestamp);
	gst_install_plugins_context_set_startup_notification_id (install_ctx, startup_id);
	g_free (startup_id);
}
#endif

static gboolean
bacon_video_widget_start_plugin_installation (XplayerCodecInstallContext *ctx,
                                              gboolean                  confirm_search)
{
	GstInstallPluginsContext *install_ctx;
	GstInstallPluginsReturn status;
#ifdef GDK_WINDOWING_X11
	GdkDisplay *display;
#endif

	install_ctx = gst_install_plugins_context_new ();
	gst_install_plugins_context_set_desktop_id (install_ctx, "xplayer.desktop");
	gst_install_plugins_context_set_confirm_search (install_ctx, confirm_search);

#ifdef GDK_WINDOWING_X11
	display = gdk_display_get_default ();

	if (GDK_IS_X11_DISPLAY (display) &&
	    gtk_widget_get_window (GTK_WIDGET (ctx->bvw)) != NULL &&
	    gtk_widget_get_realized (GTK_WIDGET (ctx->bvw)))
	{
		gulong xid = 0;

		set_startup_notification_id (install_ctx);

		xid = bacon_video_widget_gst_get_toplevel (GTK_WIDGET (ctx->bvw));
		gst_install_plugins_context_set_xid (install_ctx, xid);
	}
#endif /* GDK_WINDOWING_X11 */

	status = gst_install_plugins_async ((const gchar * const*)ctx->details, install_ctx,
	                                    on_plugin_installation_done,
	                                    ctx);

	gst_install_plugins_context_free (install_ctx);

	GST_INFO ("gst_install_plugins_async() result = %d", status);

	if (status != GST_INSTALL_PLUGINS_STARTED_OK)
	{
		if (status == GST_INSTALL_PLUGINS_HELPER_MISSING)
		{
			g_message ("Automatic missing codec installation not supported "
			           "(helper script missing)");
		} else {
			g_warning ("Failed to start codec installation: %s",
			           gst_install_plugins_return_get_name (status));
		}
		bacon_video_widget_gst_codec_install_context_free (ctx);
		return FALSE;
	}

	return TRUE;
}

static void
codec_confirmation_dialog_response_cb (GtkDialog       *dialog,
                                       GtkResponseType  response_type,
                                       gpointer         user_data)
{
	XplayerCodecInstallContext *ctx = user_data;

	switch (response_type) {
	case GTK_RESPONSE_ACCEPT:
		bacon_video_widget_start_plugin_installation (ctx, FALSE);
		break;
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
		break;
	default:
		g_assert_not_reached ();
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
show_codec_confirmation_dialog (XplayerCodecInstallContext *ctx,
                                const gchar              *install_helper_display_name)
{
	GtkWidget *button;
	GtkWidget *dialog;
	GtkWidget *toplevel;
	gchar *button_text;
	gchar *descriptions_text;
	gchar *message_text;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (ctx->bvw));

	dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
	                                 GTK_DIALOG_MODAL |
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_CANCEL,
	                                 _("Unable to play the file"));

	descriptions_text = g_strjoinv (", ", ctx->descriptions);
	message_text = g_strdup_printf (ngettext ("%s is required to play the file, but is not installed.",
	                                          "%s are required to play the file, but are not installed.",
	                                          g_strv_length (ctx->descriptions)),
	                                descriptions_text);

	/* TRANSLATORS: this is a button to launch a codec installer.
	 * %s will be replaced with the software installer's name, e.g.
	 * 'Software' in case of gnome-software. */
	button_text = g_strdup_printf ("_Find in %s", install_helper_display_name);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", message_text);
	button = gtk_dialog_add_button (GTK_DIALOG (dialog),
	                                button_text,
	                                GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "suggested-action");
	g_signal_connect (dialog, "response",
	                  G_CALLBACK (codec_confirmation_dialog_response_cb),
	                  ctx);

	gtk_window_present (GTK_WINDOW (dialog));

	g_free (button_text);
	g_free (descriptions_text);
	g_free (message_text);
}

static void
on_packagekit_proxy_ready (GObject      *source_object,
                           GAsyncResult *res,
                           gpointer      user_data)
{
	XplayerCodecInstallContext *ctx = (XplayerCodecInstallContext *) user_data;
	GDBusProxy *packagekit_proxy = NULL;
	GVariant *property = NULL;
	GError *error = NULL;

	packagekit_proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
	if (packagekit_proxy == NULL &&
	    g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		goto out;
	}

	if (packagekit_proxy != NULL) {
		property = g_dbus_proxy_get_cached_property (packagekit_proxy, "DisplayName");
		if (property != NULL) {
			const gchar *display_name;

			display_name = g_variant_get_string (property, NULL);
			if (display_name != NULL && display_name[0] != '\0') {
				show_codec_confirmation_dialog (ctx, display_name);
				goto out;
			}
		}
	}

	/* If the above failed, fall back to immediately starting the codec installation */
	bacon_video_widget_start_plugin_installation (ctx, TRUE);

out:
	g_clear_error (&error);
	g_clear_pointer (&property, g_variant_unref);
	g_clear_object (&packagekit_proxy);
}

static gboolean
bacon_video_widget_gst_on_missing_plugins_event (BaconVideoWidget  *bvw,
                                                 char             **details,
                                                 char             **descriptions,
                                                 gboolean           playing,
                                                 gpointer           user_data)
{
	XplayerCodecInstallContext *ctx;
	guint i, num;

	num = g_strv_length (details);
	g_return_val_if_fail (num > 0 && g_strv_length (descriptions) == num, FALSE);

	ctx = g_new0 (XplayerCodecInstallContext, 1);
	ctx->descriptions = g_strdupv (descriptions);
	ctx->details = g_strdupv (details);
	ctx->playing = playing;
	ctx->bvw = bvw;

	for (i = 0; i < num; ++i)
	{
		if (bacon_video_widget_gst_codec_install_plugin_is_blacklisted (ctx->details[i]))
		{
			g_message ("Missing plugin: %s (ignoring)", ctx->details[i]);
			g_free (ctx->details[i]);
			g_free (ctx->descriptions[i]);
			ctx->details[i] = ctx->details[num-1];
			ctx->descriptions[i] = ctx->descriptions[num-1];
			ctx->details[num-1] = NULL;
			ctx->descriptions[num-1] = NULL;
			--num;
			--i;
		} else {
			g_message ("Missing plugin: %s (%s)", ctx->details[i], ctx->descriptions[i]);
		}
	}

	if (num == 0)
	{
		g_message ("All missing plugins are blacklisted, doing nothing");
		bacon_video_widget_gst_codec_install_context_free (ctx);
		return FALSE;
	}

	/* Get the PackageKit session interface proxy and continue with the
	 * codec installation in the callback */
	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
	                          G_DBUS_PROXY_FLAGS_NONE,
	                          NULL, /* g-interface-info */
	                          "org.freedesktop.PackageKit",
	                          "/org/freedesktop/PackageKit",
	                          "org.freedesktop.PackageKit.Modify2",
	                          g_object_get_data (G_OBJECT (bvw), "missing-plugins-cancellable"),
	                          on_packagekit_proxy_ready,
	                          ctx);

	/* if we managed to start playing, pause playback, since some install
	 * wizard should now take over in a second anyway and the user might not
	 * be able to use xplayer's controls while the wizard is running */
	if (playing)
		bacon_video_widget_pause (bvw);

	return TRUE;
}

#endif /* ENABLE_MISSING_PLUGIN_INSTALLATION */

void
bacon_video_widget_gst_missing_plugins_setup (BaconVideoWidget *bvw)
{
#ifdef ENABLE_MISSING_PLUGIN_INSTALLATION
	g_signal_connect (G_OBJECT (bvw),
			"missing-plugins",
			G_CALLBACK (bacon_video_widget_gst_on_missing_plugins_event),
			bvw);

	gst_pb_utils_init ();

	GST_INFO ("Set up support for automatic missing plugin installation");
#endif
}

void
bacon_video_widget_gst_missing_plugins_blacklist (void)
{
	struct {
		const char *name;
		gboolean remove;
	} blacklisted_elements[] = {
		{ "ffdemux_flv", 0 },
		{ "avdemux_flv", 0 },
		{ "dvdreadsrc" , 1 }
	};
	GstRegistry *registry;
	guint i;

	registry = gst_registry_get ();

	for (i = 0; i < G_N_ELEMENTS (blacklisted_elements); ++i) {
		GstPluginFeature *feature;

		feature = gst_registry_find_feature (registry,
						     blacklisted_elements[i].name,
						     GST_TYPE_ELEMENT_FACTORY);

		if (!feature)
			continue;

		if (blacklisted_elements[i].remove)
			gst_registry_remove_feature (registry, feature);
		else
			gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
	}
}

