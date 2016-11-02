
/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * mx-aspect-frame.h: A container that respect the aspect ratio of its child
 *
 * Copyright 2010, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef __XPLAYER_ASPECT_FRAME_H__
#define __XPLAYER_ASPECT_FRAME_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XPLAYER_TYPE_ASPECT_FRAME xplayer_aspect_frame_get_type()

#define XPLAYER_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  XPLAYER_TYPE_ASPECT_FRAME, XplayerAspectFrame))

#define XPLAYER_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  XPLAYER_TYPE_ASPECT_FRAME, XplayerAspectFrameClass))

#define XPLAYER_IS_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  XPLAYER_TYPE_ASPECT_FRAME))

#define XPLAYER_IS_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  XPLAYER_TYPE_ASPECT_FRAME))

#define XPLAYER_ASPECT_FRAME_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  XPLAYER_TYPE_ASPECT_FRAME, XplayerAspectFrameClass))

typedef struct _XplayerAspectFrame XplayerAspectFrame;
typedef struct _XplayerAspectFrameClass XplayerAspectFrameClass;
typedef struct _XplayerAspectFramePrivate XplayerAspectFramePrivate;

struct _XplayerAspectFrame
{
  ClutterActor parent;

  XplayerAspectFramePrivate *priv;
};

struct _XplayerAspectFrameClass
{
  ClutterActorClass parent_class;
};

GType           xplayer_aspect_frame_get_type     (void) G_GNUC_CONST;

ClutterActor *  xplayer_aspect_frame_new          (void);

void            xplayer_aspect_frame_set_child    (XplayerAspectFrame *frame,
						 ClutterActor     *child);

void            xplayer_aspect_frame_set_expand   (XplayerAspectFrame *frame,
                                                 gboolean          expand);
gboolean        xplayer_aspect_frame_get_expand   (XplayerAspectFrame *frame);

void            xplayer_aspect_frame_set_rotation (XplayerAspectFrame *frame,
						 gdouble           rotation);
void            xplayer_aspect_frame_set_internal_rotation
						(XplayerAspectFrame *frame,
						 gdouble           rotation);
gdouble         xplayer_aspect_frame_get_rotation (XplayerAspectFrame *frame);

G_END_DECLS

#endif /* __XPLAYER_ASPECT_FRAME_H__ */
