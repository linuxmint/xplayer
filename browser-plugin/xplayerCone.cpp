/* Xplayer Cone plugin
 *
 * Copyright © 2004 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2008 Christian Persch
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

#include <string.h>

#include <glib.h>

#include "xplayerPlugin.h"
#include "xplayerCone.h"

static const char *propertyNames[] = {
  "audio",
  "input",
  "iterator",
  "log",
  "messages",
  "playlist",
  "VersionInfo",
  "video",
};

static const char *methodNames[] = {
  "versionInfo"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerCone,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerCone::xplayerCone (NPP aNPP)
  : xplayerNPObject (aNPP)
{
  XPLAYER_LOG_CTOR ();
}

xplayerCone::~xplayerCone ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerCone::InvokeByIndex (int aIndex,
                          const NPVariant *argv,
                          uint32_t argc,
                          NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerCone);

  switch (Methods (aIndex)) {
    case eversionInfo:
      return GetPropertyByIndex (eVersionInfo, _result);
  }

  return false;
}

bool
xplayerCone::GetPropertyByIndex (int aIndex,
                               NPVariant *_result)
{
  XPLAYER_LOG_GETTER (aIndex, xplayerCone);

  switch (Properties (aIndex)) {
    case eAudio:
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eConeAudio));

    case eInput:
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eConeInput));

    case ePlaylist:
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eConePlaylist));

    case eVideo:
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eConeVideo));

    case eVersionInfo:
      return StringVariant (_result, XPLAYER_CONE_VERSION);

    case eIterator:
    case eLog:
    case eMessages:
      XPLAYER_WARN_GETTER_UNIMPLEMENTED (aIndex, _result);
      return NullVariant (_result);
  }

  return false;
}

bool
xplayerCone::SetPropertyByIndex (int aIndex,
                               const NPVariant *aValue)
{
  XPLAYER_LOG_SETTER (aIndex, xplayerCone);

  return ThrowPropertyNotWritable ();
}
