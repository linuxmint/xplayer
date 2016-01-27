/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Plugin engine for Totem, heavily based on the code from Rhythmbox,
 * which is based heavily on the code from totem.
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 *               2006 James Livingston  <jrl@ids.org.au>
 *               2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __TOTEM_PLUGINS_ENGINE_H__
#define __TOTEM_PLUGINS_ENGINE_H__

#include <glib.h>
#include <libpeas/peas-engine.h>
#include <totem.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_PLUGINS_ENGINE              (totem_plugins_engine_get_type ())
#define TOTEM_PLUGINS_ENGINE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), TOTEM_TYPE_PLUGINS_ENGINE, TotemPluginsEngine))
#define TOTEM_PLUGINS_ENGINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), TOTEM_TYPE_PLUGINS_ENGINE, TotemPluginsEngineClass))
#define TOTEM_IS_PLUGINS_ENGINE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), TOTEM_TYPE_PLUGINS_ENGINE))
#define TOTEM_IS_PLUGINS_ENGINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_PLUGINS_ENGINE))
#define TOTEM_PLUGINS_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), TOTEM_TYPE_PLUGINS_ENGINE, TotemPluginsEngineClass))

typedef struct _TotemPluginsEngine		TotemPluginsEngine;
typedef struct _TotemPluginsEnginePrivate	TotemPluginsEnginePrivate;
typedef struct _TotemPluginsEngineClass		TotemPluginsEngineClass;

struct _TotemPluginsEngine
{
	PeasEngine parent;
	TotemPluginsEnginePrivate *priv;
};

struct _TotemPluginsEngineClass
{
	PeasEngineClass parent_class;
};

GType			totem_plugins_engine_get_type			(void) G_GNUC_CONST;
TotemPluginsEngine	*totem_plugins_engine_get_default		(TotemObject *totem);
void			totem_plugins_engine_shut_down			(TotemPluginsEngine *self);

G_END_DECLS

#endif  /* __TOTEM_PLUGINS_ENGINE_H__ */

