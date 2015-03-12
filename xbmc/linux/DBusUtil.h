#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#ifdef HAS_DBUS
#include "DBusMessage.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "utils/log.h"



#include <string>
#include <vector>
#include <queue>
#include <mutex>

class CDBusHelper
{    
public : 
  CDBusHelper();
  //CDBusHelper(DBusConnection *connection);
  ~CDBusHelper() {};
  
protected :  
  DBusConnection* m_connection;
  
  void ReadAndDispatchDbusMessage();    

  bool RegisterMatch(const char *type,const char *path,const char *iface, const char *name);    
  bool UnregisterMatch(const char *type,const char *path,const char *iface, const char *name);
private :    
  static const char* CreateRule(const char *type,const char *path,const char *iface, const char *name); 
};

class CDBusUtil
{
public:
  //**
  //deprecated : use generic Get instead
  static CVariant GetVariant(const char *destination, const char *object, const char *interface, const char *property);
  //**
  
  template <typename T> static bool Set(const char *destination, const char *object, const char *interface, const char *property,T value);
  
  static CVariant Get(const char *destination, const char *object, const char *interface, const char *property);
  static CVariant GetAll(const char *destination, const char *object, const char *interface);

  static CVariant Call(const char *destination, const char *object, const char *interface, const char *method);
  template< typename... Args> static CVariant Call(const char *destination, const char *object, const char *interface, const char *method, Args ... args);

  static bool CallAsync(const char *destination, const char *object, const char *interface, const char *method);
  template< typename... Args> static bool CallAsync(const char *destination, const char *object, const char *interface, const char *method, Args ... args);
    
  static CVariant Decode(DBusMessage *message);
  
private:
  static CVariant ParseType(DBusMessageIter *itr);
};
 
#endif
