/*
 * Copyright © 2008 Christian Persch
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __XPLAYER_NPOBJECT_H__
#define __XPLAYER_NPOBJECT_H__

#include <assert.h>

#include "npapi.h"
#include "npruntime.h"

class xplayerPlugin;
class xplayerNPObject;
class xplayerNPClass_base;
template<class T> class xplayerNPClass;

class xplayerNPObject : public NPObject {
  public:
    xplayerNPObject (NPP);

    virtual ~xplayerNPObject ();

    void* operator new (size_t aSize) throw ();

  protected:
    friend class xplayerNPClass_base;

    /* NPObject methods */
    virtual void Invalidate     ();
    virtual bool HasMethod      (NPIdentifier aName);
    virtual bool Invoke         (NPIdentifier aName, const NPVariant *argv, uint32_t argc, NPVariant *_result);
    virtual bool InvokeDefault  (const NPVariant *argv, uint32_t argc, NPVariant *_result);
    virtual bool HasProperty    (NPIdentifier aName);
    virtual bool GetProperty    (NPIdentifier aName, NPVariant *_result);
    virtual bool SetProperty    (NPIdentifier aName, const NPVariant *aValue);
    virtual bool RemoveProperty (NPIdentifier aName);
    virtual bool Enumerate      (NPIdentifier **_result, uint32_t *_count);
    virtual bool Construct      (const NPVariant *argv, uint32_t argc, NPVariant *_result);

    /* By Index methods */
    virtual bool InvokeByIndex         (int aIndex, const NPVariant *argv, uint32_t argc, NPVariant *_result);
    virtual bool GetPropertyByIndex    (int aIndex, NPVariant *_result);
    virtual bool SetPropertyByIndex    (int aIndex, const NPVariant *aValue);
    virtual bool RemovePropertyByIndex (int aIndex);

  private:

    NPP mNPP;
    xplayerPlugin *mPlugin;

  protected:

    bool IsValid () const { return mPlugin != 0; }
    xplayerPlugin* Plugin () const { assert (IsValid ()); return mPlugin; }

    bool Throw (const char*);
    bool ThrowPropertyNotWritable ();
    bool ThrowSecurityError ();

    bool CheckArgc (uint32_t, uint32_t, uint32_t = uint32_t(-1), bool = true);
    bool CheckArgType (NPVariantType, NPVariantType, uint32_t = 0);
    bool CheckArg (const NPVariant*, uint32_t, uint32_t, NPVariantType);
    bool CheckArgv (const NPVariant*, uint32_t, uint32_t, ...);

    bool GetBoolFromArguments (const NPVariant*, uint32_t, uint32_t, bool&);
    bool GetInt32FromArguments (const NPVariant*, uint32_t, uint32_t, int32_t&);
    bool GetDoubleFromArguments (const NPVariant*, uint32_t, uint32_t, double&);
    bool GetNPStringFromArguments (const NPVariant*, uint32_t, uint32_t, NPString&);
    bool DupStringFromArguments (const NPVariant*, uint32_t, uint32_t, char*&);
    bool GetObjectFromArguments (const NPVariant*, uint32_t, uint32_t, NPObject*&);

    bool VoidVariant (NPVariant*);
    bool NullVariant (NPVariant*);
    bool BoolVariant (NPVariant*, bool);
    bool Int32Variant (NPVariant*, int32_t);
    bool DoubleVariant (NPVariant*, double);
    bool StringVariant (NPVariant*, const char*, int32_t = -1);
    bool ObjectVariant (NPVariant*, NPObject*);

  private:

    xplayerNPClass_base* GetClass() const { return static_cast<xplayerNPClass_base*>(_class); }
};

/* Helper macros */
#define XPLAYER_LOG_CTOR() g_debug ("%s [%p]", __func__, (void*) this)
#define XPLAYER_LOG_DTOR() g_debug ("%s [%p]", __func__, (void*) this)

#define XPLAYER_LOG_INVOKE(i, T) \
{\
  static bool logAccess[G_N_ELEMENTS (methodNames)];\
  if (!logAccess[i]) {\
    g_debug ("NOTE: site calls function %s::%s", #T, methodNames[i]);\
    logAccess[i] = true;\
  }\
}

#define XPLAYER_LOG_GETTER(i, T) \
{\
  static bool logAccess[G_N_ELEMENTS (propertyNames)];\
  if (!logAccess[i]) {\
    g_debug ("NOTE: site gets property %s::%s", #T, propertyNames[i]);\
    logAccess[i] = true;\
  }\
}

#define XPLAYER_LOG_SETTER(i, T) \
{\
  static bool logAccess[G_N_ELEMENTS (propertyNames)];\
  if (!logAccess[i]) {\
    g_debug ("NOTE: site sets property %s::%s", #T, propertyNames[i]);\
    logAccess[i] = true;\
  }\
}

#define XPLAYER_WARN_INVOKE_UNIMPLEMENTED(i, T) \
{\
  static bool logWarning[G_N_ELEMENTS (methodNames)];\
  if (!logWarning[i]) {\
    g_warning ("WARNING: function %s::%s is unimplemented", #T, methodNames[i]);\
    logWarning[i] = true;\
  }\
}

#define XPLAYER_WARN_1_INVOKE_UNIMPLEMENTED(i, T) \
{\
  static bool logWarning;\
  if (!logWarning) {\
    g_warning ("WARNING: function %s::%s is unimplemented", #T, methodNames[i]);\
    logWarning = true;\
  }\
}

#define XPLAYER_WARN_GETTER_UNIMPLEMENTED(i, T) \
{\
  static bool logWarning[G_N_ELEMENTS (propertyNames)];\
  if (!logWarning[i]) {\
    g_warning ("WARNING: getter for property %s::%s is unimplemented", #T, propertyNames[i]);\
    logWarning[i] = true;\
  }\
}

#define XPLAYER_WARN_1_GETTER_UNIMPLEMENTED(i, T) \
{\
  static bool logWarning;\
  if (!logWarning) {\
    g_warning ("WARNING: getter for property %s::%s is unimplemented", #T, propertyNames[i]);\
    logWarning = true;\
  }\
}

#define XPLAYER_WARN_SETTER_UNIMPLEMENTED(i, T) \
{\
  static bool logWarning[G_N_ELEMENTS (propertyNames)];\
  if (!logWarning[i]) {\
    g_warning ("WARNING: setter for property %s::%s is unimplemented", #T, propertyNames[i]);\
    logWarning[i] = true;\
  }\
}

#define XPLAYER_WARN_1_SETTER_UNIMPLEMENTED(i, T) \
{\
  static bool logWarning;\
  if (!logWarning) {\
    g_warning ("WARNING: setter for property %s::%s is unimplemented", #T, propertyNames[i]);\
    logWarning = true;\
  }\
}

#endif /* __XPLAYER_NPOBJECT_H__ */
