


#include "bacon-video-widget.h"
#include "bacon-video-widget-enums.h"

/* enumerations from "./bacon-video-widget.h" */
GType
bvw_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_ERROR_NO_PLUGIN_FOR_FILE, "BVW_ERROR_NO_PLUGIN_FOR_FILE", "no-plugin-for-file" },
      { BVW_ERROR_BROKEN_FILE, "BVW_ERROR_BROKEN_FILE", "broken-file" },
      { BVW_ERROR_FILE_GENERIC, "BVW_ERROR_FILE_GENERIC", "file-generic" },
      { BVW_ERROR_FILE_PERMISSION, "BVW_ERROR_FILE_PERMISSION", "file-permission" },
      { BVW_ERROR_FILE_ENCRYPTED, "BVW_ERROR_FILE_ENCRYPTED", "file-encrypted" },
      { BVW_ERROR_FILE_NOT_FOUND, "BVW_ERROR_FILE_NOT_FOUND", "file-not-found" },
      { BVW_ERROR_DVD_ENCRYPTED, "BVW_ERROR_DVD_ENCRYPTED", "dvd-encrypted" },
      { BVW_ERROR_INVALID_DEVICE, "BVW_ERROR_INVALID_DEVICE", "invalid-device" },
      { BVW_ERROR_UNKNOWN_HOST, "BVW_ERROR_UNKNOWN_HOST", "unknown-host" },
      { BVW_ERROR_NETWORK_UNREACHABLE, "BVW_ERROR_NETWORK_UNREACHABLE", "network-unreachable" },
      { BVW_ERROR_CONNECTION_REFUSED, "BVW_ERROR_CONNECTION_REFUSED", "connection-refused" },
      { BVW_ERROR_INVALID_LOCATION, "BVW_ERROR_INVALID_LOCATION", "invalid-location" },
      { BVW_ERROR_GENERIC, "BVW_ERROR_GENERIC", "generic" },
      { BVW_ERROR_CODEC_NOT_HANDLED, "BVW_ERROR_CODEC_NOT_HANDLED", "codec-not-handled" },
      { BVW_ERROR_CANNOT_CAPTURE, "BVW_ERROR_CANNOT_CAPTURE", "cannot-capture" },
      { BVW_ERROR_READ_ERROR, "BVW_ERROR_READ_ERROR", "read-error" },
      { BVW_ERROR_PLUGIN_LOAD, "BVW_ERROR_PLUGIN_LOAD", "plugin-load" },
      { BVW_ERROR_EMPTY_FILE, "BVW_ERROR_EMPTY_FILE", "empty-file" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwError", values);
  }
  return etype;
}
GType
bvw_metadata_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_INFO_TITLE, "BVW_INFO_TITLE", "title" },
      { BVW_INFO_ARTIST, "BVW_INFO_ARTIST", "artist" },
      { BVW_INFO_YEAR, "BVW_INFO_YEAR", "year" },
      { BVW_INFO_COMMENT, "BVW_INFO_COMMENT", "comment" },
      { BVW_INFO_ALBUM, "BVW_INFO_ALBUM", "album" },
      { BVW_INFO_DURATION, "BVW_INFO_DURATION", "duration" },
      { BVW_INFO_TRACK_NUMBER, "BVW_INFO_TRACK_NUMBER", "track-number" },
      { BVW_INFO_COVER, "BVW_INFO_COVER", "cover" },
      { BVW_INFO_CONTAINER, "BVW_INFO_CONTAINER", "container" },
      { BVW_INFO_HAS_VIDEO, "BVW_INFO_HAS_VIDEO", "has-video" },
      { BVW_INFO_DIMENSION_X, "BVW_INFO_DIMENSION_X", "dimension-x" },
      { BVW_INFO_DIMENSION_Y, "BVW_INFO_DIMENSION_Y", "dimension-y" },
      { BVW_INFO_VIDEO_BITRATE, "BVW_INFO_VIDEO_BITRATE", "video-bitrate" },
      { BVW_INFO_VIDEO_CODEC, "BVW_INFO_VIDEO_CODEC", "video-codec" },
      { BVW_INFO_FPS, "BVW_INFO_FPS", "fps" },
      { BVW_INFO_HAS_AUDIO, "BVW_INFO_HAS_AUDIO", "has-audio" },
      { BVW_INFO_AUDIO_BITRATE, "BVW_INFO_AUDIO_BITRATE", "audio-bitrate" },
      { BVW_INFO_AUDIO_CODEC, "BVW_INFO_AUDIO_CODEC", "audio-codec" },
      { BVW_INFO_AUDIO_SAMPLE_RATE, "BVW_INFO_AUDIO_SAMPLE_RATE", "audio-sample-rate" },
      { BVW_INFO_AUDIO_CHANNELS, "BVW_INFO_AUDIO_CHANNELS", "audio-channels" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwMetadataType", values);
  }
  return etype;
}
GType
bvw_video_property_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_VIDEO_BRIGHTNESS, "BVW_VIDEO_BRIGHTNESS", "brightness" },
      { BVW_VIDEO_CONTRAST, "BVW_VIDEO_CONTRAST", "contrast" },
      { BVW_VIDEO_SATURATION, "BVW_VIDEO_SATURATION", "saturation" },
      { BVW_VIDEO_HUE, "BVW_VIDEO_HUE", "hue" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwVideoProperty", values);
  }
  return etype;
}
GType
bvw_aspect_ratio_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_RATIO_AUTO, "BVW_RATIO_AUTO", "auto" },
      { BVW_RATIO_SQUARE, "BVW_RATIO_SQUARE", "square" },
      { BVW_RATIO_FOURBYTHREE, "BVW_RATIO_FOURBYTHREE", "fourbythree" },
      { BVW_RATIO_ANAMORPHIC, "BVW_RATIO_ANAMORPHIC", "anamorphic" },
      { BVW_RATIO_DVB, "BVW_RATIO_DVB", "dvb" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwAspectRatio", values);
  }
  return etype;
}
GType
bvw_zoom_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_ZOOM_NONE, "BVW_ZOOM_NONE", "none" },
      { BVW_ZOOM_EXPAND, "BVW_ZOOM_EXPAND", "expand" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwZoomMode", values);
  }
  return etype;
}
GType
bvw_rotation_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_ROTATION_R_ZERO, "BVW_ROTATION_R_ZERO", "zero" },
      { BVW_ROTATION_R_90R, "BVW_ROTATION_R_90R", "90r" },
      { BVW_ROTATION_R_180, "BVW_ROTATION_R_180", "180" },
      { BVW_ROTATION_R_90L, "BVW_ROTATION_R_90L", "90l" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwRotation", values);
  }
  return etype;
}
GType
bvw_dvd_event_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_DVD_ROOT_MENU, "BVW_DVD_ROOT_MENU", "root-menu" },
      { BVW_DVD_TITLE_MENU, "BVW_DVD_TITLE_MENU", "title-menu" },
      { BVW_DVD_SUBPICTURE_MENU, "BVW_DVD_SUBPICTURE_MENU", "subpicture-menu" },
      { BVW_DVD_AUDIO_MENU, "BVW_DVD_AUDIO_MENU", "audio-menu" },
      { BVW_DVD_ANGLE_MENU, "BVW_DVD_ANGLE_MENU", "angle-menu" },
      { BVW_DVD_CHAPTER_MENU, "BVW_DVD_CHAPTER_MENU", "chapter-menu" },
      { BVW_DVD_NEXT_CHAPTER, "BVW_DVD_NEXT_CHAPTER", "next-chapter" },
      { BVW_DVD_PREV_CHAPTER, "BVW_DVD_PREV_CHAPTER", "prev-chapter" },
      { BVW_DVD_NEXT_TITLE, "BVW_DVD_NEXT_TITLE", "next-title" },
      { BVW_DVD_PREV_TITLE, "BVW_DVD_PREV_TITLE", "prev-title" },
      { BVW_DVD_ROOT_MENU_UP, "BVW_DVD_ROOT_MENU_UP", "root-menu-up" },
      { BVW_DVD_ROOT_MENU_DOWN, "BVW_DVD_ROOT_MENU_DOWN", "root-menu-down" },
      { BVW_DVD_ROOT_MENU_LEFT, "BVW_DVD_ROOT_MENU_LEFT", "root-menu-left" },
      { BVW_DVD_ROOT_MENU_RIGHT, "BVW_DVD_ROOT_MENU_RIGHT", "root-menu-right" },
      { BVW_DVD_ROOT_MENU_SELECT, "BVW_DVD_ROOT_MENU_SELECT", "root-menu-select" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwDVDEvent", values);
  }
  return etype;
}
GType
bvw_audio_output_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { BVW_AUDIO_SOUND_STEREO, "BVW_AUDIO_SOUND_STEREO", "stereo" },
      { BVW_AUDIO_SOUND_4CHANNEL, "BVW_AUDIO_SOUND_4CHANNEL", "4channel" },
      { BVW_AUDIO_SOUND_41CHANNEL, "BVW_AUDIO_SOUND_41CHANNEL", "41channel" },
      { BVW_AUDIO_SOUND_5CHANNEL, "BVW_AUDIO_SOUND_5CHANNEL", "5channel" },
      { BVW_AUDIO_SOUND_51CHANNEL, "BVW_AUDIO_SOUND_51CHANNEL", "51channel" },
      { BVW_AUDIO_SOUND_AC3PASSTHRU, "BVW_AUDIO_SOUND_AC3PASSTHRU", "ac3passthru" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("BvwAudioOutputType", values);
  }
  return etype;
}



