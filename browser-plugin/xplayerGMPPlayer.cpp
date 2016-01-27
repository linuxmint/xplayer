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

#include "npupp.h"

#include "xplayerPlugin.h"
#include "xplayerGMPPlayer.h"

static const char *propertyNames[] = {
  "cdromCollection",
  "closedCaption",
  "controls",
  "currentMedia",
  "currentPlaylist",
  "dvd",
  "enableContextMenu",
  "enabled",
  "error",
  "fullScreen",
  "isOnline",
  "isRemote",
  "mediaCollection",
  "network",
  "openState",
  "playerApplication",
  "playlistCollection",
  "playState",
  "settings",
  "status",
  "stretchToFit",
  "uiMode",
  "URL",
  "versionInfo",
  "windowlessVideo",
};

static const char *methodNames[] = {
  "close",
  "launchURL",
  "newMedia",
  "newPlaylist",
  "openPlayer"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerGMPPlayer,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerGMPPlayer::xplayerGMPPlayer (NPP aNPP)
  : xplayerNPObject (aNPP)
{
  XPLAYER_LOG_CTOR ();
}

xplayerGMPPlayer::~xplayerGMPPlayer ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerGMPPlayer::InvokeByIndex (int aIndex,
                               const NPVariant *argv,
                               uint32_t argc,
                               NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerGMPPlayer);

  switch (Methods (aIndex)) {
    case eNewPlaylist:
      /* xplayerIGMPPlaylist newPlaylist (in AUTF8String name, in AUTF8String URL); */
      XPLAYER_WARN_INVOKE_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return NullVariant (_result);

    case eClose:
      /* void close (); */
    case eNewMedia:
      /* xplayerIGMPMedia newMedia (in AUTF8String URL); */
    case eOpenPlayer:
      /* void openPlayer (in AUTF8String URL); */
    case eLaunchURL:
      /* void launchURL (in AUTF8String URL); */
      return ThrowSecurityError ();
  }

  return false;
}

bool
xplayerGMPPlayer::GetPropertyByIndex (int aIndex,
                                    NPVariant *_result)
{
  XPLAYER_LOG_GETTER (aIndex, xplayerGMPPlayer);

  switch (Properties (aIndex)) {
    case eControls:
      /* readonly attribute xplayerIGMPControls controls; */
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eGMPControls));

    case eNetwork:
      /* readonly attribute xplayerIGMPNetwork network; */
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eGMPNetwork));

    case eSettings:
      /* readonly attribute xplayerIGMPSettings settings; */
      return ObjectVariant (_result, Plugin()->GetNPObject (xplayerPlugin::eGMPSettings));

    case eVersionInfo:
      /* readonly attribute ACString versionInfo; */
      return StringVariant (_result, XPLAYER_GMP_VERSION_BUILD);

    case eFullScreen:
      /* attribute boolean fullScreen; */
      return BoolVariant (_result, Plugin()->IsFullscreen());

    case eWindowlessVideo:
      /* attribute boolean windowlessVideo; */
      return BoolVariant (_result, Plugin()->IsWindowless());

    case eIsOnline:
      /* readonly attribute boolean isOnline; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return BoolVariant (_result, true);

    case eEnableContextMenu:
      /* attribute boolean enableContextMenu; */
      return BoolVariant (_result, Plugin()->AllowContextMenu());

    case eClosedCaption:
      /* readonly attribute xplayerIGMPClosedCaption closedCaption; */
    case eCurrentMedia:
      /* attribute xplayerIGMPMedia currentMedia; */
    case eCurrentPlaylist:
      /* attribute xplayerIGMPPlaylist currentPlaylist; */
    case eError:
      /* readonly attribute xplayerIGMPError error; */
      XPLAYER_WARN_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return NullVariant (_result);

    case eStatus:
      /* readonly attribute AUTF8String status; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return StringVariant (_result, "OK");

    case eURL:
      /* attribute AUTF8String URL; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return StringVariant (_result, Plugin()->Src()); /* FIXMEchpe use URL()? */

    case eEnabled:
      /* attribute boolean enabled; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return BoolVariant (_result, true);

    case eOpenState:
      /* readonly attribute long openState; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return Int32Variant (_result, 0);

    case ePlayState:
      /* readonly attribute long playState; */
      return Int32Variant (_result, mPluginState);

    case eStretchToFit:
      /* attribute boolean stretchToFit; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return BoolVariant (_result, false);

    case eUiMode:
      /* attribute ACString uiMode; */
      XPLAYER_WARN_1_GETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return VoidVariant (_result);

    case eCdromCollection:
      /* readonly attribute xplayerIGMPCdromCollection cdromCollection; */
    case eDvd:
      /* readonly attribute xplayerIGMPDVD dvd; */
    case eMediaCollection:
      /* readonly attribute xplayerIGMPMediaCollection mediaCollection; */
    case ePlayerApplication:
      /* readonly attribute xplayerIGMPPlayerApplication playerApplication; */
    case ePlaylistCollection:
      /* readonly attribute xplayerIGMPPlaylistCollection playlistCollection; */
    case eIsRemote:
      /* readonly attribute boolean isRemote; */
      return ThrowSecurityError ();
  }

  return false;
}

bool
xplayerGMPPlayer::SetPropertyByIndex (int aIndex,
                                    const NPVariant *aValue)
{
  XPLAYER_LOG_SETTER (aIndex, xplayerGMPPlayer);

  switch (Properties (aIndex)) {
    case eFullScreen: {
      /* attribute boolean fullScreen; */
      bool enabled;
      if (!GetBoolFromArguments (aValue, 1, 0, enabled))
        return false;

      Plugin()->SetFullscreen (enabled);
      return true;
    }

    case eWindowlessVideo: {
      /* attribute boolean windowlessVideo; */
      bool enabled;
      if (!GetBoolFromArguments (aValue, 1, 0, enabled))
        return false;

      Plugin()->SetIsWindowless(enabled);
      return true;
    }

    case eURL: {
      /* attribute AUTF8String URL; */
      NPString url;
      if (!GetNPStringFromArguments (aValue, 1, 0, url))
        return false;

      Plugin()->SetSrc (url); /* FIXMEchpe: use SetURL instead?? */
      return true;
    }

    case eEnableContextMenu: {
      /* attribute boolean enableContextMenu; */
      bool enabled;
      if (!GetBoolFromArguments (aValue, 1, 0, enabled))
        return false;

      Plugin()->SetAllowContextMenu (enabled);
      return true;
    }

    case eCurrentMedia:
      /* attribute xplayerIGMPMedia currentMedia; */
    case eCurrentPlaylist:
      /* attribute xplayerIGMPPlaylist currentPlaylist; */
    case eEnabled:
      /* attribute boolean enabled; */
    case eStretchToFit:
      /* attribute boolean stretchToFit; */
    case eUiMode:
      /* attribute ACString uiMode; */
      XPLAYER_WARN_SETTER_UNIMPLEMENTED (aIndex, xplayerGMPPlayer);
      return true;

    case eCdromCollection:
      /* readonly attribute xplayerIGMPCdromCollection cdromCollection; */
    case eClosedCaption:
      /* readonly attribute xplayerIGMPClosedCaption closedCaption; */
    case eControls:
      /* readonly attribute xplayerIGMPControls controls; */
    case eDvd:
      /* readonly attribute xplayerIGMPDVD dvd; */
    case eError:
      /* readonly attribute xplayerIGMPError error; */
    case eIsOnline:
      /* readonly attribute boolean isOnline; */
    case eIsRemote:
      /* readonly attribute boolean isRemote; */
    case eMediaCollection:
      /* readonly attribute xplayerIGMPMediaCollection mediaCollection; */
    case eNetwork:
      /* readonly attribute xplayerIGMPNetwork network; */
    case eOpenState:
      /* readonly attribute long openState; */
    case ePlayerApplication:
      /* readonly attribute xplayerIGMPPlayerApplication playerApplication; */
    case ePlaylistCollection:
      /* readonly attribute xplayerIGMPPlaylistCollection playlistCollection; */
    case ePlayState:
      /* readonly attribute long playState; */
    case eSettings:
      /* readonly attribute xplayerIGMPSettings settings; */
    case eStatus:
      /* readonly attribute AUTF8String status; */
    case eVersionInfo:
      /* readonly attribute ACString versionInfo; */
      return ThrowPropertyNotWritable ();
  }

  return false;
}
