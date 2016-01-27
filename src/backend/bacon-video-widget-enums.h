


#ifndef BVW_ENUMS_H
#define BVW_ENUMS_H

#include <glib-object.h>

G_BEGIN_DECLS
/* enumerations from "./bacon-video-widget.h" */
GType bvw_error_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_ERROR (bvw_error_get_type())
GType bvw_metadata_type_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_METADATA_TYPE (bvw_metadata_type_get_type())
GType bvw_visualization_quality_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_VISUALIZATION_QUALITY (bvw_visualization_quality_get_type())
GType bvw_video_property_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_VIDEO_PROPERTY (bvw_video_property_get_type())
GType bvw_aspect_ratio_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_ASPECT_RATIO (bvw_aspect_ratio_get_type())
GType bvw_zoom_mode_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_ZOOM_MODE (bvw_zoom_mode_get_type())
GType bvw_rotation_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_ROTATION (bvw_rotation_get_type())
GType bvw_dvd_event_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_DVD_EVENT (bvw_dvd_event_get_type())
GType bvw_audio_output_type_get_type (void) G_GNUC_CONST;
#define BVW_TYPE_AUDIO_OUTPUT_TYPE (bvw_audio_output_type_get_type())
G_END_DECLS

#endif /* !BVW_ENUMS_H */



