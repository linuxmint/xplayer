/* Xplayer Vegas Plugin
 *
 * Copyright © 2011 Bastien Nocera <hadess@hadess.net>
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

#include <config.h>

#include "xplayerPlugin.h"
#include "xplayerVegasPlugin.h"

static const char *methodNames[] = {
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerVegasPlayer,
                         NULL, 0,
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerVegasPlayer::xplayerVegasPlayer (NPP aNPP)
  : xplayerNPObject (aNPP)
{
  XPLAYER_LOG_CTOR ();
}

xplayerVegasPlayer::~xplayerVegasPlayer ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerVegasPlayer::InvokeByIndex (int aIndex,
                                       const NPVariant *argv,
                                       uint32_t argc,
                                       NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerVegasPlayer);

  return false;
}
