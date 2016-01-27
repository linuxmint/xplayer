/* Xplayer GMP plugin
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

#include "xplayerGMPError.h"

static const char *propertyNames[] = {
  "errorCount",
};

static const char *methodNames[] = {
  "clearErrorQueue",
  "item",
  "webHelp"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerGMPError,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerGMPError::xplayerGMPError (NPP aNPP)
  : xplayerNPObject (aNPP)
{
  XPLAYER_LOG_CTOR ();
}

xplayerGMPError::~xplayerGMPError ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerGMPError::InvokeByIndex (int aIndex,
                              const NPVariant *argv,
                              uint32_t argc,
                              NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerGMPError);

  switch (Methods (aIndex)) {
    case eClearErrorQueue:
      /* void clearErrorQueue (); */
    case eWebHelp:
      /* void webHelp (); */
      XPLAYER_WARN_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPError);
      return VoidVariant (_result);

    case eItem:
      /* xplayerIGMPErrorItem item (in long index); */
      XPLAYER_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPError);
      return NullVariant (_result);
  }

  return false;
}

bool
xplayerGMPError::GetPropertyByIndex (int aIndex,
                                   NPVariant *_result)
{
  XPLAYER_LOG_GETTER (aIndex, xplayerGMPError);

  switch (Properties (aIndex)) {
    case eErrorCount:
      /* readonly attribute long errorCount; */
      return Int32Variant (_result, 0);
  }

  return false;
}

bool
xplayerGMPError::SetPropertyByIndex (int aIndex,
                                   const NPVariant *aValue)
{
  XPLAYER_LOG_SETTER (aIndex, xplayerGMPError);

  return ThrowPropertyNotWritable ();
}
