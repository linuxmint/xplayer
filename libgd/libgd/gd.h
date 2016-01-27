/*
 * Copyright (c) 2012 Red Hat, Inc.
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
 */

#ifndef __GD_H__
#define __GD_H__

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#include <libgd/gd-types-catalog.h>

#ifdef LIBGD_GTK_HACKS
# include <libgd/gd-icon-utils.h>
# include <libgd/gd-entry-focus-hack.h>
#endif

#ifdef LIBGD__VIEW_COMMON
# include <libgd/gd-main-view-generic.h>
# include <libgd/gd-styled-text-renderer.h>
# include <libgd/gd-toggle-pixbuf-renderer.h>
# include <libgd/gd-two-lines-renderer.h>
#endif

#ifdef LIBGD_MAIN_ICON_VIEW
# include <libgd/gd-main-icon-view.h>
#endif

#ifdef LIBGD_MAIN_LIST_VIEW
# include <libgd/gd-main-list-view.h>
#endif

#ifdef LIBGD_MAIN_VIEW
# include <libgd/gd-main-view.h>
#endif

#ifdef LIBGD_MAIN_TOOLBAR
# include <libgd/gd-main-toolbar.h>
#endif

#ifdef LIBGD_HEADER_BAR
# include <libgd/gd-header-bar.h>
#endif

#ifdef LIBGD__HEADER_BUTTON
# include <libgd/gd-header-button.h>
#endif

#ifdef LIBGD_MARGIN_CONTAINER
# include <libgd/gd-margin-container.h>
#endif

#ifdef LIBGD_TAGGED_ENTRY
# include <libgd/gd-tagged-entry.h>
#endif

#ifdef LIBGD_NOTIFICATION
# include <libgd/gd-notification.h>
#endif

#ifdef LIBGD_REVEALER
# include <libgd/gd-revealer.h>
#endif

#ifdef LIBGD_STACK
# include <libgd/gd-stack.h>
# include <libgd/gd-stack-switcher.h>
#endif

G_END_DECLS

#endif /* __GD_H__ */
