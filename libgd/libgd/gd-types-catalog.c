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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gd-types-catalog.h"

#ifdef LIBGD__VIEW_COMMON
# include "gd-main-view-generic.h"
# include "gd-styled-text-renderer.h"
# include "gd-toggle-pixbuf-renderer.h"
# include "gd-two-lines-renderer.h"
#endif

#ifdef LIBGD_MAIN_ICON_VIEW
# include "gd-main-icon-view.h"
#endif

#ifdef LIBGD_MAIN_LIST_VIEW
# include "gd-main-list-view.h"
#endif

#ifdef LIBGD_MAIN_VIEW
# include "gd-main-view.h"
#endif

#ifdef LIBGD_MAIN_TOOLBAR
# include "gd-main-toolbar.h"
#endif

#ifdef LIBGD_HEADER_BAR
# include "gd-header-bar.h"
#endif

#ifdef LIBGD__HEADER_BUTTON
# include "gd-header-button.h"
#endif

#ifdef LIBGD_MARGIN_CONTAINER
# include "gd-margin-container.h"
#endif

#ifdef LIBGD_TAGGED_ENTRY
# include "gd-tagged-entry.h"
#endif

#ifdef LIBGD_NOTIFICATION
# include "gd-notification.h"
#endif

#ifdef LIBGD_REVEALER
# include "gd-revealer.h"
#endif

#ifdef LIBGD_STACK
# include "gd-stack.h"
# include "gd-stack-switcher.h"
#endif

/**
 * gd_ensure_types:
 *
 * This functions must be called during initialization
 * to make sure the widget types are available to GtkBuilder.
 */
void
gd_ensure_types (void)
{
#ifdef LIBGD__VIEW_COMMON
  g_type_ensure (GD_TYPE_MAIN_VIEW_GENERIC);
  g_type_ensure (GD_TYPE_STYLED_TEXT_RENDERER);
  g_type_ensure (GD_TYPE_TOGGLE_PIXBUF_RENDERER);
  g_type_ensure (GD_TYPE_TWO_LINES_RENDERER);
#endif

#ifdef LIBGD_MAIN_ICON_VIEW
  g_type_ensure (GD_TYPE_MAIN_ICON_VIEW);
#endif

#ifdef LIBGD_MAIN_LIST_VIEW
  g_type_ensure (GD_TYPE_MAIN_LIST_VIEW);
#endif

#ifdef LIBGD_MAIN_VIEW
  g_type_ensure (GD_TYPE_MAIN_VIEW);
#endif

#ifdef LIBGD_MAIN_TOOLBAR
  g_type_ensure (GD_TYPE_MAIN_TOOLBAR);
#endif

#ifdef LIBGD_HEADER_BAR
  g_type_ensure (GD_TYPE_HEADER_BAR);
#endif

#ifdef LIBGD__HEADER_BUTTON
  g_type_ensure (GD_TYPE_HEADER_SIMPLE_BUTTON);
  g_type_ensure (GD_TYPE_HEADER_MENU_BUTTON);
  g_type_ensure (GD_TYPE_HEADER_TOGGLE_BUTTON);
  g_type_ensure (GD_TYPE_HEADER_RADIO_BUTTON);
#endif

#ifdef LIBGD_MARGIN_CONTAINER
  g_type_ensure (GD_TYPE_MARGIN_CONTAINER);
#endif

#ifdef LIBGD_TAGGED_ENTRY
  g_type_ensure (GD_TYPE_TAGGED_ENTRY);
#endif

#ifdef LIBGD_NOTIFICATION
  g_type_ensure (GD_TYPE_NOTIFICATION);
#endif

#ifdef LIBGD_REVEALER
  g_type_ensure (GD_TYPE_REVEALER);
#endif

#ifdef LIBGD_STACK
  g_type_ensure (GD_TYPE_STACK);
  g_type_ensure (GD_TYPE_STACK_SWITCHER);
#endif
}

