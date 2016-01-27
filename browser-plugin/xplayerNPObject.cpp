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
#include <stdio.h>
#include <stdarg.h>

#include <glib.h>

#include "xplayerNPClass.h"
#include "xplayerNPObject.h"

#ifdef DEBUG_PLUGIN
#define NOTE(x) x
#else
#define NOTE(x)
#endif

static const char *variantTypes[] = {
  "void",
  "null",
  "bool",
  "int32",
  "double",
  "string",
  "object",
  "unknown"
};

#define VARIANT_TYPE(type) (variantTypes[MIN (type, NPVariantType_Object + 1)])

void*
xplayerNPObject::operator new (size_t aSize) throw ()
{
  void *instance = ::operator new (aSize);
  if (instance) {
    memset (instance, 0, aSize);
  }

  return instance;
}

xplayerNPObject::xplayerNPObject (NPP aNPP)
  : mNPP (aNPP),
    mPlugin (reinterpret_cast<xplayerPlugin*>(aNPP->pdata))
{
  NOTE (g_print ("xplayerNPObject ctor [%p]\n", (void*) this));
}

xplayerNPObject::~xplayerNPObject ()
{
  NOTE (g_print ("xplayerNPObject dtor [%p]\n", (void*) this));
}

bool
xplayerNPObject::Throw (const char *aMessage)
{
  NOTE (g_print ("xplayerNPObject::Throw [%p] : %s\n", (void*) this, aMessage));

  NPN_SetException (this, aMessage);
  return false;
}

bool
xplayerNPObject::ThrowPropertyNotWritable ()
{
  return Throw ("Property not writable");
}

bool
xplayerNPObject::ThrowSecurityError ()
{
  return Throw ("Access denied");
}

bool
xplayerNPObject::CheckArgc (uint32_t argc,
                          uint32_t minArgc,
                          uint32_t maxArgc,
                          bool doThrow)
{
  if (argc >= minArgc && argc <= maxArgc)
    return true;

  if (argc < minArgc) {
    if (doThrow)
      return Throw ("Not enough arguments");

    return false;
  }

  if (doThrow)
    return Throw ("Too many arguments");

  return false;
}

bool
xplayerNPObject::CheckArgType (NPVariantType argType,
                             NPVariantType expectedType,
                             uint32_t argNum)
{
  bool conforms;

  switch (argType) {
    case NPVariantType_Void:
    case NPVariantType_Null:
      conforms = (argType == expectedType);
      break;

    case NPVariantType_Bool:
      conforms = (argType == NPVariantType_Bool ||
                  argType == NPVariantType_Int32 ||
                  argType == NPVariantType_Double);
      break;

    case NPVariantType_Int32:
    case NPVariantType_Double:
      /* FIXMEchpe: also accept NULL or VOID ? */
      conforms = (argType == NPVariantType_Int32 ||
                  argType == NPVariantType_Double);
      break;

    case NPVariantType_String:
    case NPVariantType_Object:
      conforms = (argType == expectedType ||
                  argType == NPVariantType_Null ||
                  argType == NPVariantType_Void);
      break;
    default:
      conforms = false;
  }

  if (!conforms) {
      char msg[128];
      g_snprintf (msg, sizeof (msg),
                  "Wrong type of argument %d: expected %s but got %s\n",
                  argNum, VARIANT_TYPE (expectedType), VARIANT_TYPE (argType));

      return Throw (msg);
  }

  return true;
}

bool
xplayerNPObject::CheckArg (const NPVariant *argv,
                         uint32_t argc,
                         uint32_t argNum,
                         NPVariantType type)
{
  if (!CheckArgc (argc, argNum + 1))
    return false;

  return CheckArgType (argv[argNum].type, type, argNum);
}

bool
xplayerNPObject::CheckArgv (const NPVariant* argv,
                          uint32_t argc,
                          uint32_t expectedArgc,
                          ...)
{
  if (!CheckArgc (argc, expectedArgc, expectedArgc))
    return false;

  va_list type_args;
  va_start (type_args, expectedArgc);

  for (uint32_t i = 0; i < argc; ++i) {
    NPVariantType type = NPVariantType (va_arg (type_args, int /* promotion */));

    if (!CheckArgType (argv[i].type, type)) {
      va_end (type_args);
      return false;
    }
  }

  va_end (type_args);

  return true;
}

bool
xplayerNPObject::GetBoolFromArguments (const NPVariant* argv,
                                     uint32_t argc,
                                     uint32_t argNum,
                                     bool& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Bool))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_BOOLEAN (arg)) {
    _result = NPVARIANT_TO_BOOLEAN (arg);
  } else if (NPVARIANT_IS_INT32 (arg)) {
    _result = NPVARIANT_TO_INT32 (arg) != 0;
  } else if (NPVARIANT_IS_DOUBLE (arg)) {
    _result = NPVARIANT_TO_DOUBLE (arg) != 0.0;
  } else {
    /* void/null */
    _result = false;
  }

  return true;
}

bool
xplayerNPObject::GetInt32FromArguments (const NPVariant* argv,
                                      uint32_t argc,
                                      uint32_t argNum,
                                      int32_t& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Int32))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_INT32 (arg)) {
    _result = NPVARIANT_TO_INT32 (arg);
  } else if (NPVARIANT_IS_DOUBLE (arg)) {
    _result = int32_t (NPVARIANT_TO_DOUBLE (arg));
    /* FIXMEchpe: overflow? */
  }

  return true;
}

bool
xplayerNPObject::GetDoubleFromArguments (const NPVariant* argv,
                                       uint32_t argc,
                                       uint32_t argNum,
                                       double& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Double))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_DOUBLE (arg)) {
    _result = NPVARIANT_TO_DOUBLE (arg);
  } else if (NPVARIANT_IS_INT32 (arg)) {
    _result = double (NPVARIANT_TO_INT32 (arg));
  }

  return true;
}

bool
xplayerNPObject::GetNPStringFromArguments (const NPVariant* argv,
                                         uint32_t argc,
                                         uint32_t argNum,
                                         NPString& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_String))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_STRING (arg)) {
    _result = NPVARIANT_TO_STRING (arg);
  } else if (NPVARIANT_IS_NULL (arg) ||
             NPVARIANT_IS_VOID (arg)) {
    _result.UTF8Characters = NULL;
    _result.UTF8Length = 0;
  }

  return true;
}

bool
xplayerNPObject::DupStringFromArguments (const NPVariant* argv,
                                       uint32_t argc,
                                       uint32_t argNum,
                                       char*& _result)
{
  NPN_MemFree (_result);
  _result = NULL;

  NPString newValue;
  if (!GetNPStringFromArguments (argv, argc, argNum, newValue))
    return false;

  _result = NPN_StrnDup (newValue.UTF8Characters, newValue.UTF8Length);
  return true;
}

bool
xplayerNPObject::GetObjectFromArguments (const NPVariant* argv,
                                        uint32_t argc,
                                        uint32_t argNum,
                                        NPObject*& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Object))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_STRING (arg)) {
    _result = NPVARIANT_TO_OBJECT (arg);
  } else if (NPVARIANT_IS_NULL (arg) ||
             NPVARIANT_IS_VOID (arg)) {
    _result = NULL;
  }

  return true;
}

bool
xplayerNPObject::VoidVariant (NPVariant* _result)
{
  VOID_TO_NPVARIANT (*_result);
  return true;
}

bool
xplayerNPObject::NullVariant (NPVariant* _result)
{
  NULL_TO_NPVARIANT (*_result);
  return true;
}

bool
xplayerNPObject::BoolVariant (NPVariant* _result,
                            bool value)
{
  BOOLEAN_TO_NPVARIANT (value, *_result);
  return true;
}

bool
xplayerNPObject::Int32Variant (NPVariant* _result,
                             int32_t value)
{
  INT32_TO_NPVARIANT (value, *_result);
  return true;
}

bool
xplayerNPObject::DoubleVariant (NPVariant* _result,
                              double value)
{
  DOUBLE_TO_NPVARIANT (value, *_result);
  return true;
}

bool
xplayerNPObject::StringVariant (NPVariant* _result,
                              const char* value,
                              int32_t len)
{
  if (!value) {
    NULL_TO_NPVARIANT (*_result);
  } else {
    char *dup;

    if (len < 0) {
      len = strlen (value);
      dup = (char*) NPN_MemDup (value, len + 1);
    } else {
      dup = (char*) NPN_MemDup (value, len);
    }

    if (dup) {
      STRINGN_TO_NPVARIANT (dup, len, *_result);
    } else {
      NULL_TO_NPVARIANT (*_result);
    }
  }

  return true;
}

bool
xplayerNPObject::ObjectVariant (NPVariant* _result,
                              NPObject* object)
{
  if (object) {
    NPN_RetainObject (object);
    OBJECT_TO_NPVARIANT (object, *_result);
  } else {
    NULL_TO_NPVARIANT (*_result);
  }

  return true;
}

/* NPObject method default implementations */

void
xplayerNPObject::Invalidate ()
{
  NOTE (g_print ("xplayerNPObject %p invalidated\n", (void*) this));

  mNPP = NULL;
  mPlugin = NULL;
}

bool
xplayerNPObject::HasMethod (NPIdentifier aName)
{
  if (!IsValid ())
    return false;

  int methodIndex = GetClass()->GetMethodIndex (aName);
  NOTE (g_print ("xplayerNPObject::HasMethod [%p] %s => %s\n", (void*) this, NPN_UTF8FromIdentifier (aName), methodIndex >= 0 ? "yes" : "no"));
  if (methodIndex >= 0)
    return true;

  if (aName == NPN_GetStringIdentifier ("__noSuchMethod__"))
    return true;

  return false;
}

bool
xplayerNPObject::Invoke (NPIdentifier aName,
                       const NPVariant *argv,
                       uint32_t argc,
                       NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::Invoke [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int methodIndex = GetClass()->GetMethodIndex (aName);
  if (methodIndex >= 0)
    return InvokeByIndex (methodIndex, argv, argc, _result);

  if (aName == NPN_GetStringIdentifier ("__noSuchMethod__")) {
    /* http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Global_Objects:Object:_noSuchMethod */
    if (!CheckArgv (argv, argc, 2, NPVariantType_String, NPVariantType_Object))
      return false;

    const char *id = NPVARIANT_TO_STRING (argv[0]).UTF8Characters;
    g_message ("NOTE: site calls unknown function \"%s\" on xplayerNPObject %p\n", id ? id : "(null)", (void*) this);

    /* Silently ignore the invocation */
    VOID_TO_NPVARIANT (*_result);
    return true;
  }

  return Throw ("No method with this name exists.");
}

bool
xplayerNPObject::InvokeDefault (const NPVariant *argv,
                              uint32_t argc,
                              NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::InvokeDefault [%p]\n", (void*) this));
  int defaultMethodIndex = GetClass()->GetDefaultMethodIndex ();
  if (defaultMethodIndex >= 0)
    return InvokeByIndex (defaultMethodIndex, argv, argc, _result);

  return false;
}

bool
xplayerNPObject::HasProperty (NPIdentifier aName)
{
  if (!IsValid ())
    return false;

  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  NOTE (g_print ("xplayerNPObject::HasProperty [%p] %s => %s\n", (void*) this, NPN_UTF8FromIdentifier (aName), propertyIndex >= 0 ? "yes" : "no"));
  if (propertyIndex >= 0)
    return true;

  return false;
}

bool
xplayerNPObject::GetProperty (NPIdentifier aName,
                            NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::GetProperty [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  if (propertyIndex >= 0)
    return GetPropertyByIndex (propertyIndex, _result);

  return Throw ("No property with this name exists.");
}

bool
xplayerNPObject::SetProperty (NPIdentifier aName,
                            const NPVariant *aValue)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::SetProperty [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  if (propertyIndex >= 0)
    return SetPropertyByIndex (propertyIndex, aValue);

  return Throw ("No property with this name exists.");
}

bool
xplayerNPObject::RemoveProperty (NPIdentifier aName)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::RemoveProperty [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  if (propertyIndex >= 0)
    return RemovePropertyByIndex (propertyIndex);

  return Throw ("No property with this name exists.");
}

bool
xplayerNPObject::Enumerate (NPIdentifier **_result,
                          uint32_t *_count)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::Enumerate [%p]\n", (void*) this));
  return GetClass()->EnumerateProperties (_result, _count);
}

bool
xplayerNPObject::Construct (const NPVariant *argv,
                          uint32_t argc,
                          NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("xplayerNPObject::Construct [%p]\n", (void*) this));
  return false; /* FIXMEchpe! */
}

/* by-index methods */

bool
xplayerNPObject::InvokeByIndex (int aIndex,
                              const NPVariant *argv,
                              uint32_t argc,
                              NPVariant *_result)
{
  return false;
}

bool
xplayerNPObject::GetPropertyByIndex (int aIndex,
                                   NPVariant *_result)
{
  return false;
}

bool
xplayerNPObject::SetPropertyByIndex (int aIndex,
                                   const NPVariant *aValue)
{
  return false;
}

bool
xplayerNPObject::RemovePropertyByIndex (int aIndex)
{
  return Throw ("Removing properties is not supported.");
}
