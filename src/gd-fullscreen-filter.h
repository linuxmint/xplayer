/*
 * Copyright (c) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#ifndef __GD_FULLSCREEN_FILTER_H__
#define __GD_FULLSCREEN_FILTER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GD_TYPE_FULLSCREEN_FILTER gd_fullscreen_filter_get_type()

#define GD_FULLSCREEN_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
   GD_TYPE_FULLSCREEN_FILTER, GdFullscreenFilter))

#define GD_FULLSCREEN_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
   GD_TYPE_FULLSCREEN_FILTER, GdFullscreenFilterClass))

#define GD_IS_FULLSCREEN_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   GD_TYPE_FULLSCREEN_FILTER))

#define GD_IS_FULLSCREEN_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
   GD_TYPE_FULLSCREEN_FILTER))

#define GD_FULLSCREEN_FILTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
   GD_TYPE_FULLSCREEN_FILTER, GdFullscreenFilterClass))

typedef struct _GdFullscreenFilter GdFullscreenFilter;
typedef struct _GdFullscreenFilterClass GdFullscreenFilterClass;
typedef struct _GdFullscreenFilterPrivate GdFullscreenFilterPrivate;

struct _GdFullscreenFilter
{
  GObject parent;

  GdFullscreenFilterPrivate *priv;
};

struct _GdFullscreenFilterClass
{
  GObjectClass parent_class;
};

GType gd_fullscreen_filter_get_type (void) G_GNUC_CONST;

GdFullscreenFilter *gd_fullscreen_filter_new (void);
void gd_fullscreen_filter_start (GdFullscreenFilter *self);
void gd_fullscreen_filter_stop (GdFullscreenFilter *self);

G_END_DECLS

#endif /* __GD_FULLSCREEN_FILTER_H__ */
