/* Xplayer Plugin Viewer constants
 *
 * Copyright © 2006 Christian Persch
 * Copyright © 2007 Bastien Nocera <hadess@hadess.net>
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

#ifndef __XPLAYER_PLUGIN_VIEWER_CONSTANTS__
#define __XPLAYER_PLUGIN_VIEWER_CONSTANTS__

#define XPLAYER_COMMAND_PLAY		"Play"
#define XPLAYER_COMMAND_PAUSE		"Pause"
#define XPLAYER_COMMAND_STOP		"Stop"

typedef enum {
	XPLAYER_STATE_PLAYING,
	XPLAYER_STATE_PAUSED,
	XPLAYER_STATE_STOPPED,
	XPLAYER_STATE_INVALID
} XplayerStates;

static const char *xplayer_states[] = {
	"PLAYING",
	"PAUSED",
	"STOPPED",
	"INVALID"
};

#define XPLAYER_PROPERTY_VOLUME		"volume"
#define XPLAYER_PROPERTY_ISFULLSCREEN	"is-fullscreen"

#endif /* !__XPLAYER_PLUGIN_VIEWER_CONSTANTS__ */
