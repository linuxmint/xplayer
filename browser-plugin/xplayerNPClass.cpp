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

#include <config.h>
#include <string.h>

#include "xplayerNPClass.h"
#include "xplayerNPObject.h"

xplayerNPClass_base::xplayerNPClass_base (const char *aPropertNames[],
                                      uint32_t aPropertyCount,
                                      const char *aMethodNames[],
                                      uint32_t aMethodCount,
                                      const char *aDefaultMethodName) :
  mPropertyNameIdentifiers (GetIdentifiersForNames (aPropertNames, aPropertyCount)),
  mPropertyNamesCount (aPropertyCount),
  mMethodNameIdentifiers (GetIdentifiersForNames (aMethodNames, aMethodCount)),
  mMethodNamesCount (aMethodCount),
  mDefaultMethodIndex (aDefaultMethodName ? GetMethodIndex (NPN_GetStringIdentifier (aDefaultMethodName)) : -1)
{
  structVersion  = NP_CLASS_STRUCT_VERSION_ENUM;
  allocate       = Allocate;
  deallocate     = Deallocate;
  invalidate     = Invalidate;
  hasMethod      = HasMethod;
  invoke         = Invoke;
  invokeDefault  = InvokeDefault;
  hasProperty    = HasProperty;
  getProperty    = GetProperty;
  setProperty    = SetProperty;
  removeProperty = RemoveProperty;
#if defined(NP_CLASS_STRUCT_VERSION_ENUM) && (NP_CLASS_STRUCT_VERSION >= NP_CLASS_STRUCT_VERSION_ENUM)
  enumerate      = Enumerate;
#endif
#if defined(NP_CLASS_STRUCT_VERSION_CTOR) && (NP_CLASS_STRUCT_VERSION >= NP_CLASS_STRUCT_VERSION_CTOR)
  /* FIXMEchpe find out what's this supposed to do */
  /* construct      = Construct; */
  construct = NULL;
#endif
}

xplayerNPClass_base::~xplayerNPClass_base ()
{
  NPN_MemFree (mPropertyNameIdentifiers);
  NPN_MemFree (mMethodNameIdentifiers);
}

NPIdentifier*
xplayerNPClass_base::GetIdentifiersForNames (const char *aNames[],
                                           uint32_t aCount)
{
  if (aCount == 0)
    return NULL;

  NPIdentifier *identifiers = reinterpret_cast<NPIdentifier*>(NPN_MemAlloc (aCount * sizeof (NPIdentifier)));
  if (!identifiers)
    return NULL;

  NPN_GetStringIdentifiers (aNames, aCount, identifiers);

  return identifiers;
}

int
xplayerNPClass_base::GetPropertyIndex (NPIdentifier aName)
{
  if (!mPropertyNameIdentifiers)
    return -1;

  for (int i = 0; i < mPropertyNamesCount; ++i) {
    if (aName == mPropertyNameIdentifiers[i])
      return i;
  }

  return -1;
}

int
xplayerNPClass_base::GetMethodIndex (NPIdentifier aName)
{
  if (!mMethodNameIdentifiers)
    return -1;

  for (int i = 0; i < mMethodNamesCount; ++i) {
    if (aName == mMethodNameIdentifiers[i])
      return i;
  }

  return -1;
}

bool
xplayerNPClass_base::EnumerateProperties (NPIdentifier **_result, uint32_t *_count)
{
  if (!mPropertyNameIdentifiers)
    return false;

  uint32_t bytes = mPropertyNamesCount * sizeof (NPIdentifier);
  NPIdentifier *identifiers = reinterpret_cast<NPIdentifier*>(NPN_MemAlloc (bytes));
  if (!identifiers)
    return false;

  memcpy (identifiers, mPropertyNameIdentifiers, bytes);

  *_result = identifiers;
  *_count = mPropertyNamesCount;

  return true;
}

NPObject*
xplayerNPClass_base::Allocate (NPP aNPP, NPClass *aClass)
{
  xplayerNPClass_base* _class = static_cast<xplayerNPClass_base*>(aClass);
  return _class->InternalCreate (aNPP);
}

void
xplayerNPClass_base::Deallocate (NPObject *aObject)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  delete object;
}

void
xplayerNPClass_base::Invalidate (NPObject *aObject)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  object->Invalidate ();
}

bool
xplayerNPClass_base::HasMethod (NPObject *aObject, NPIdentifier aName)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->HasMethod (aName);
}

bool
xplayerNPClass_base::Invoke (NPObject *aObject, NPIdentifier aName, const NPVariant *argv, uint32_t argc, NPVariant *_result)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->Invoke (aName, argv, argc, _result);
}

bool
xplayerNPClass_base::InvokeDefault (NPObject *aObject, const NPVariant *argv, uint32_t argc, NPVariant *_result)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->InvokeDefault (argv, argc, _result);
}

bool
xplayerNPClass_base::HasProperty (NPObject *aObject, NPIdentifier aName)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->HasProperty (aName);
}

bool
xplayerNPClass_base::GetProperty (NPObject *aObject, NPIdentifier aName, NPVariant *_result)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->GetProperty (aName, _result);
}

bool
xplayerNPClass_base::SetProperty (NPObject *aObject, NPIdentifier aName, const NPVariant *aValue)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->SetProperty (aName, aValue);
}

bool
xplayerNPClass_base::RemoveProperty (NPObject *aObject, NPIdentifier aName)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->RemoveProperty (aName);
}

bool
xplayerNPClass_base::Enumerate (NPObject *aObject, NPIdentifier **_result, uint32_t *_count)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->Enumerate (_result, _count);
}

bool
xplayerNPClass_base::Construct (NPObject *aObject, const NPVariant *argv, uint32_t argc, NPVariant *_result)
{
  xplayerNPObject* object = static_cast<xplayerNPObject*> (aObject);
  return object->Construct (argv, argc, _result);
}
