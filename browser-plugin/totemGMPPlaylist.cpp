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

#include "xplayerGMPPlaylist.h"

static const char *propertyNames[] = {
  "attributeCount",
  "count",
  "name"
};

static const char *methodNames[] = {
  "appendItem",
  "attributeName",
  "getAttributeName",
  "getItemInfo",
  "insertItem",
  "isIdentical",
  "item",
  "moveItem",
  "removeItem",
  "setItemInfo"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerGMPPlaylist,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerGMPPlaylist::xplayerGMPPlaylist (NPP aNPP)
  : xplayerNPObject (aNPP),
    mName (NPN_StrDup ("Playlist"))
{
  XPLAYER_LOG_CTOR ();
}

xplayerGMPPlaylist::~xplayerGMPPlaylist ()
{
  XPLAYER_LOG_DTOR ();

  g_free (mName);
}

bool
xplayerGMPPlaylist::InvokeByIndex (int aIndex,
                                 const NPVariant *argv,
                                 uint32_t argc,
                                 NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerGMPPlaylist);

  switch (Methods (aIndex)) {
    case eAttributeName:
      /* AUTF8String attributeName (in long index); */
    case eGetAttributeName:
      /* AUTF8String getAttributeName (in long index); */
    case eGetItemInfo:
      XPLAYER_WARN_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPPlaylist);
      /* AUTF8String getItemInfo (in AUTF8String name); */
      return StringVariant (_result, "");

    case eIsIdentical: {
      /* boolean isIdentical (in xplayerIGMPPlaylist playlist); */
      NPObject *other;
      if (!GetObjectFromArguments (argv, argc, 0, other))
        return false;

      return BoolVariant (_result, other == static_cast<NPObject*>(this));
    }

    case eItem:
      /* xplayerIGMPMedia item (in long index); */
      XPLAYER_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPPlaylist);
      return NullVariant (_result);

    case eAppendItem:
      /* void appendItem (in xplayerIGMPMedia item); */
    case eInsertItem:
      /* void insertItem (in long index, in xplayerIGMPMedia item); */
    case eMoveItem:
      /* void moveItem (in long oldIndex, in long newIndex); */
    case eRemoveItem:
      /* void removeItem (in xplayerIGMPMedia item); */
    case eSetItemInfo:
      /* void setItemInfo (in AUTF8String name, in AUTF8String value); */
      XPLAYER_WARN_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPPlaylist);
      return VoidVariant (_result);
  }

  return false;
}

bool
xplayerGMPPlaylist::GetPropertyByIndex (int aIndex,
                                      NPVariant *_result)
{
  XPLAYER_LOG_GETTER (aIndex, xplayerGMPPlaylist);

  switch (Properties (aIndex)) {
    case eAttributeCount:
      /* readonly attribute long attributeCount; */
    case eCount:
      /* readonly attribute long count; */
      return Int32Variant (_result, 0);

    case eName:
      /* attribute AUTF8String name; */
      return StringVariant (_result, mName);
  }

  return false;
}

bool
xplayerGMPPlaylist::SetPropertyByIndex (int aIndex,
                                      const NPVariant *aValue)
{
  XPLAYER_LOG_SETTER (aIndex, xplayerGMPPlaylist);

  switch (Properties (aIndex)) {
    case eName:
      /* attribute AUTF8String name; */
      return DupStringFromArguments (aValue, 1, 0, mName);

    case eAttributeCount:
      /* readonly attribute long attributeCount; */
    case eCount:
      /* readonly attribute long count; */
      return ThrowPropertyNotWritable ();
  }

  return false;
}
