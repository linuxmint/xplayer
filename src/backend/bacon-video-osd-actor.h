/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 8; tab-width: 8 -*-
 *
 * On-screen-display (OSD) window for gnome-settings-daemon's plugins
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu> 
 * Copyright (C) 2009 Novell, Inc
 *
 * Authors:
 *   William Jon McCann <mccann@jhu.edu>
 *   Federico Mena-Quintero <federico@novell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef BACON_VIDEO_OSD_ACTOR_H
#define BACON_VIDEO_OSD_ACTOR_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define BACON_TYPE_VIDEO_OSD_ACTOR            (bacon_video_osd_actor_get_type ())
#define BACON_VIDEO_OSD_ACTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  BACON_TYPE_VIDEO_OSD_ACTOR, BaconVideoOsdActor))
#define BACON_VIDEO_OSD_ACTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),   BACON_TYPE_VIDEO_OSD_ACTOR, BaconVideoOsdActorClass))
#define BACON_IS_VIDEO_OSD_ACTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  BACON_TYPE_VIDEO_OSD_ACTOR))
#define BACON_IS_VIDEO_OSD_ACTOR_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), BACON_TYPE_VIDEO_OSD_ACTOR))
#define BACON_VIDEO_OSD_ACTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BACON_TYPE_VIDEO_OSD_ACTOR, BaconVideoOsdActorClass))

typedef struct BaconVideoOsdActor                   BaconVideoOsdActor;
typedef struct BaconVideoOsdActorClass              BaconVideoOsdActorClass;
typedef struct BaconVideoOsdActorPrivate            BaconVideoOsdActorPrivate;

struct BaconVideoOsdActor {
        ClutterActor                parent;

        BaconVideoOsdActorPrivate  *priv;
};

struct BaconVideoOsdActorClass {
        ClutterActorClass parent_class;
};

GType                 bacon_video_osd_actor_get_type          (void);

ClutterActor *        bacon_video_osd_actor_new               (void);
void                  bacon_video_osd_actor_set_icon_name     (BaconVideoOsdActor *osd,
							       const char         *icon_name);
void                  bacon_video_osd_actor_hide              (BaconVideoOsdActor *osd);
void                  bacon_video_osd_actor_show              (BaconVideoOsdActor *osd);
void                  bacon_video_osd_actor_show_and_fade     (BaconVideoOsdActor *osd);

G_END_DECLS

#endif
