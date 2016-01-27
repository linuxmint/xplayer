/* Xplayer MullY Plugin
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

#include <glib.h>

#include "xplayerPlugin.h"
#include "xplayerMullYPlugin.h"

static const char *methodNames[] = {
  "GetVersion",
  "SetMinVersion",
  "SetMode",
  "SetAllowContextMenu",
  "SetAutoPlay",
  "SetLoop",
  "SetBufferingMode",
  "SetBannerEnabled",
  "SetVolume",
  "SetMovieTitle",
  "SetPreviewImage",
  "SetPreviewMessage",
  "SetPreviewMessageFontSize",
  "Open",
  "Play",
  "Pause",
  "StepForward",
  "StepBackward",
  "FF",
  "RW",
  "Stop",
  "Mute",
  "UnMute",
  "Seek",
  "About",
  "ShowPreferences",
  "ShowContextMenu",
  "GoEmbedded",
  "GoWindowed",
  "GoFullscreen",
  "Resize",
  "GetTotalTime",
  "GetVideoWidth",
  "GetVideoHeight",
  "GetTotalVideoFrames",
  "GetVideoFramerate",
  "GetNumberOfAudioTracks",
  "GetNumberOfSubtitleTracks",
  "GetAudioTrackLanguage",
  "GetSubtitleTrackLanguage",
  "GetAudioTrackName",
  "GetSubtitleTrackName",
  "GetCurrentAudioTrack",
  "GetCurrentSubtitleTrack",
  "SetCurrentAudioTrack",
  "SetCurrentSubtitleTrack"
};

XPLAYER_IMPLEMENT_NPCLASS (xplayerMullYPlayer,
                         NULL, 0,
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

xplayerMullYPlayer::xplayerMullYPlayer (NPP aNPP)
  : xplayerNPObject (aNPP)
{
  XPLAYER_LOG_CTOR ();
}

xplayerMullYPlayer::~xplayerMullYPlayer ()
{
  XPLAYER_LOG_DTOR ();
}

bool
xplayerMullYPlayer::InvokeByIndex (int aIndex,
                                 const NPVariant *argv,
                                 uint32_t argc,
                                 NPVariant *_result)
{
  XPLAYER_LOG_INVOKE (aIndex, xplayerMullYPlayer);

  switch (Methods (aIndex)) {
    case eGetVersion:
      return StringVariant (_result, XPLAYER_MULLY_VERSION);

    case ePlay:
      Plugin()->Command (XPLAYER_COMMAND_PLAY);
      return VoidVariant (_result);

    case ePause:
      Plugin()->Command (XPLAYER_COMMAND_PAUSE);
      return VoidVariant (_result);

    case eStop:
      Plugin()->Command (XPLAYER_COMMAND_STOP);
      return VoidVariant (_result);

    case eSetVolume: {
      // FIXMEchpe where's getVolume?
          break;
    }

    case eMute:
      Plugin()->SetMute (true);
      return VoidVariant (_result);

    case eUnMute:
      Plugin()->SetMute (false);
      return VoidVariant (_result);

    case eSetMinVersion:
    case eSetMode:
    case eSetAllowContextMenu:
    case eSetAutoPlay:
    case eSetLoop:
    case eSetBufferingMode:
    case eSetBannerEnabled:
    case eSetMovieTitle:
    case eSetPreviewImage:
    case eSetPreviewMessage:
    case eSetPreviewMessageFontSize:
    case eOpen:
    case eStepForward:
    case eStepBackward:
    case eFF:
    case eRW:
    case eSeek:
    case eResize:
    case eGetTotalTime:
    case eGetVideoWidth:
    case eGetVideoHeight:
    case eGetTotalVideoFrames:
    case eGetVideoFramerate:
    case eGetNumberOfAudioTracks:
    case eGetNumberOfSubtitleTracks:
    case eGetAudioTrackLanguage:
    case eGetSubtitleTrackLanguage:
    case eGetAudioTrackName:
    case eGetSubtitleTrackName:
    case eGetCurrentAudioTrack:
    case eGetCurrentSubtitleTrack:
    case eSetCurrentAudioTrack:
    case eSetCurrentSubtitleTrack:
      XPLAYER_WARN_INVOKE_UNIMPLEMENTED (aIndex, xplayerMullYPlayer);
      return VoidVariant (_result);

    case eGoEmbedded:
    case eGoWindowed:
    case eGoFullscreen:
    case eAbout:
    case eShowPreferences:
    case eShowContextMenu:
      /* We don't allow the page's JS to do this. Don't throw though, just silently do nothing. */
      return VoidVariant (_result);
  }

  return false;
}
