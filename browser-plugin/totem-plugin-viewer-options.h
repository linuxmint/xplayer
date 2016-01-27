/* Xplayer Plugin Viewer options
 *
 * Copyright © 2005 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#ifndef __XPLAYER_PLUGIN_VIEWER_OPTIONS_H__
#define __XPLAYER_PLUGIN_VIEWER_OPTIONS_H__

#define XPLAYER_PLUGIN_VIEWER_INTERFACE_NAME	"org.gnome.xplayer.PluginViewer"
#define XPLAYER_PLUGIN_VIEWER_NAME_TEMPLATE	"org.gnome.xplayer.PluginViewer_%d"
#define XPLAYER_PLUGIN_VIEWER_DBUS_PATH		"/org/gnome/xplayer/PluginViewer"

#define XPLAYER_OPTION_BASE_URI		"base-uri"
#define XPLAYER_OPTION_CONTROLS_HIDDEN	"no-controls"
#define XPLAYER_OPTION_HIDDEN		"hidden"
#define XPLAYER_OPTION_MIMETYPE		"mimetype"
#define XPLAYER_OPTION_NOAUTOSTART	"no-autostart"
#define XPLAYER_OPTION_PLAYLIST		"playlist"
#define XPLAYER_OPTION_PLUGIN_TYPE	"plugin-type"
#define XPLAYER_OPTION_REPEAT		"repeat"
#define XPLAYER_OPTION_USER_AGENT		"user-agent"
#define XPLAYER_OPTION_STATUSBAR		"statusbar"
#define XPLAYER_OPTION_AUDIOONLY		"audio-only"
#define XPLAYER_OPTION_REFERRER           "referrer"

#endif /* !__XPLAYER_PLUGIN_VIEWER_OPTIONS_H__ */
