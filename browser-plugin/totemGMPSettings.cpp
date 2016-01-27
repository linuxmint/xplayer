/* Xplayer GMP plugin
 *
 * Copyright © 2006, 2007 Christian Persch
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
#include "xplayerGMPSettings.h"

static const char *propertyNames[] = {
  "autostart",
  "balance",
  "baseURL",
  "defaultAudioLanguage",
  "defaultFrame",
  "enableErrorDialogs",
  "invokeURLs",
  "mediaAccessRights",
  "mute",
  "playCount",
  "rate",
  "volume"
};

static const char *methodNames[] = {
  "getMode",
  "isAvailable",
  "requestMediaAccessRights",
  "setMode"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerGMPSettings,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerGMPSettings::xplayerGMPSettings (NPP aNPP)
  : xplayerNPObject (aNPP),
    mMute (false)
{
  XPLAYER_LOG_CTOR ();
}

xplayerGMPSettings::~xplayerGMPSettings ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerGMPSettings::InvokeByIndex (int aIndex,
                                 const NPVariant *argv,
                                 uint32_t argc,
                                 NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerGMPSettings);

  switch (Methods (aIndex)) {
    case eIsAvailable:
      /* boolean isAvailable (in ACString name); */
    case eGetMode:
      /* boolean getMode (in ACString modeName); */
    case eSetMode:
      /* void setMode (in ACString modeName, in boolean state); */
    case eRequestMediaAccessRights:
      /* boolean requestMediaAccessRights (in ACString access); */
      XPLAYER_WARN_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return BoolVariant (_result, false);
  }

  return false;
}

bool
xplayerGMPSettings::GetPropertyByIndex (int aIndex,
                                      NPVariant *_result)
{
  XPLAYER_LOG_GETTER (aIndex, xplayerGMPSettings);

  switch (Properties (aIndex)) {
    case eMute:
      /* attribute boolean mute; */
      return BoolVariant (_result, Plugin()->IsMute());

    case eVolume:
      /* attribute long volume; */
      return Int32Variant (_result, Plugin()->Volume () * 100.0);

    case eAutostart:
      /* attribute boolean autoStart; */
      return BoolVariant (_result, Plugin()->AutoPlay());

    case eBalance:
      /* attribute long balance; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return Int32Variant (_result, 0);

    case eBaseURL:
      /* attribute AUTF8String baseURL; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return StringVariant (_result, "");

    case eDefaultAudioLanguage:
      /* readonly attribute long defaultAudioLanguage; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return Int32Variant (_result, 0); /* FIXME */

    case eDefaultFrame:
      /* attribute AUTF8String defaultFrame; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return StringVariant (_result, "");

    case eEnableErrorDialogs:
      /* attribute boolean enableErrorDialogs; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return BoolVariant (_result, true);

    case eInvokeURLs:
      /* attribute boolean invokeURLs; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return BoolVariant (_result, true);

    case eMediaAccessRights:
      /* readonly attribute ACString mediaAccessRights; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return StringVariant (_result, "");

    case ePlayCount:
      /* attribute long playCount; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return Int32Variant (_result, 1);

    case eRate:
      /* attribute double rate; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return DoubleVariant (_result, 1.0);
  }

  return false;
}

bool
xplayerGMPSettings::SetPropertyByIndex (int aIndex,
                                      const NPVariant *aValue)
{
  XPLAYER_LOG_SETTER (aIndex, xplayerGMPSettings);

  switch (Properties (aIndex)) {
    case eMute: {
      /* attribute boolean mute; */
      bool enabled;
      if (!GetBoolFromArguments (aValue, 1, 0, enabled))
        return false;

      Plugin()->SetMute (enabled);
      return true;
    }

    case eVolume: {
      /* attribute long volume; */
      int32_t volume;
      if (!GetInt32FromArguments (aValue, 1, 0, volume))
        return false;

      Plugin()->SetVolume ((double) CLAMP (volume, 0, 100) / 100.0);
      return true;
    }

    case eAutostart: {
      /* attribute boolean autoStart; */
      bool enabled;
      if (!GetBoolFromArguments (aValue, 1, 0, enabled))
        return false;

      Plugin()->SetAutoPlay (enabled);
      return true;
    }

    case eBalance:
      /* attribute long balance; */
    case eBaseURL:
      /* attribute AUTF8String baseURL; */
    case eDefaultFrame:
      /* attribute AUTF8String defaultFrame; */
    case eEnableErrorDialogs:
      /* attribute boolean enableErrorDialogs; */
    case eInvokeURLs:
      /* attribute boolean invokeURLs; */
    case ePlayCount:
      /* attribute long playCount; */
    case eRate:
      /* attribute double rate; */
      XPLAYER_WARN_SETTER_UNIMPLEMENTED (aIndex, xplayerGMPSettings);
      return true;

    case eDefaultAudioLanguage:
      /* readonly attribute long defaultAudioLanguage; */
    case eMediaAccessRights:
      /* readonly attribute ACString mediaAccessRights; */
      return ThrowPropertyNotWritable ();
  }

  return false;
}
