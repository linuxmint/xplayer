/* Totem Vegas plugin scriptable
 *
 * Copyright © 2011 Bastien Nocera <hadess@hadess.net>
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

#ifndef __VEGAS_PLUGIN_H__
#define __VEGAS_PLUGIN_H__

#include "totemNPClass.h"
#include "totemNPObject.h"

class totemVegasPlayer : public totemNPObject
{
  public:
    totemVegasPlayer (NPP);
    virtual ~totemVegasPlayer ();

    enum PluginState {
    };

    PluginState mPluginState;

  private:

    enum Methods {
    };

    virtual bool InvokeByIndex (int aIndex, const NPVariant *argv, uint32_t argc, NPVariant *_result);
};

TOTEM_DEFINE_NPCLASS (totemVegasPlayer);

#endif /* __VEGAS_PLUGIN_H__ */
