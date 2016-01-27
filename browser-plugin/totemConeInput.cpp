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
#include "xplayerConeInput.h"

static const char *propertyNames[] = {
  "fps",
  "hasVout",
  "length",
  "position",
  "rate",
  "state",
  "time"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerConeInput,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         NULL, 0,
                         NULL);

xplayerConeInput::xplayerConeInput (NPP aNPP)
  : xplayerNPObject (aNPP)
{
  XPLAYER_LOG_CTOR ();
}

xplayerConeInput::~xplayerConeInput ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerConeInput::GetPropertyByIndex (int aIndex,
                                    NPVariant *_result)
{
  XPLAYER_LOG_GETTER (aIndex, xplayerConeInput);

  switch (Properties (aIndex)) {
    case eState: {
      int32_t state;

      /* IDLE/CLOSE=0,
       * OPENING=1,
       * BUFFERING=2,
       * PLAYING=3,
       * PAUSED=4,
       * STOPPING=5,
       * ERROR=6
       */
      if (Plugin()->State() == XPLAYER_STATE_PLAYING) {
        state = 3;
      } else if (Plugin()->State() == XPLAYER_STATE_PAUSED) {
        state = 4;
      } else {
        state = 0;
      }

      return Int32Variant (_result, state);
    }

    case eTime:
      return DoubleVariant (_result, double (Plugin()->GetTime()));

    case eLength:
      return DoubleVariant (_result, Plugin()->Duration());

    case eFps:
    case eHasVout:
    case ePosition:
    case eRate:
      XPLAYER_WARN_GETTER_UNIMPLEMENTED (aIndex, _result);
      return VoidVariant (_result);
  }

  return false;
}

bool
xplayerConeInput::SetPropertyByIndex (int aIndex,
                                    const NPVariant *aValue)
{
  XPLAYER_LOG_SETTER (aIndex, xplayerConeInput);

  switch (Properties (aIndex)) {
    case eTime:
      int32_t time;
      if (!GetInt32FromArguments (aValue, 1, 0, time))
	return false;

      Plugin()->SetTime(time);
      return true;

    case ePosition:
    case eRate:
    case eState:
      XPLAYER_WARN_SETTER_UNIMPLEMENTED (aIndex, _result);
      return true;

    case eFps:
    case eHasVout:
    case eLength:
      return ThrowPropertyNotWritable ();
  }

  return false;
}
