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
#include "DBusUtil.h"
#ifdef HAS_DBUS
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include <dbus/dbus.h>

//TODO
//The CDBusHelper class should be outside this file, 
//but don't fond th e way, added in makefile.in
//and get link error
//It's a base class for handler and object template

CDBusHelper::CDBusHelper()
{  
  //Get the DBUS connection
  DBusError error;
  dbus_error_init (&error);
  
  m_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  if (dbus_error_is_set(&error))
  {
    CLog::Log(LOGERROR, "DBus: Cannot Get Dbus %s : %s", error.name, error.message);
    m_connection = NULL;        
  }
  dbus_error_free (&error);
  if (m_connection!=NULL)
    dbus_connection_set_exit_on_disconnect(m_connection, false);
}

void CDBusHelper::ReadAndDispatchDbusMessage()
{
  //Process and empty Dbus queue here
  DBusDispatchStatus status;
     
  status = dbus_connection_get_dispatch_status (m_connection); 
     
  while (status == DBUS_DISPATCH_DATA_REMAINS)
  {
    if (g_advancedSettings.CanLogComponent(LOGDBUS))
      CLog::Log(LOGDEBUG, "DBus: We have waiting message, let's dispatch");
    status = dbus_connection_dispatch(m_connection);
  }
     
  if (status==DBUS_DISPATCH_NEED_MEMORY)       
    CLog::Log(LOGERROR, "DBus: Dbus_connection_dispatch need memory");			   
}

const char* CDBusHelper::CreateRule(const char *type,const char *path,const char *iface, const char *name)
{	
  //TODO : probably a better way in c++
  std::string rules = "";
    
  if (type)    
    rules = StringUtils::Format("type='%s'",type);
          
  if (path)
  {
    if (!rules.empty())
      rules+=',';
    rules+=StringUtils::Format("path='%s'",path);
  }
        
  if (iface)
  {
    if (!rules.empty())
      rules+=',';   
    rules += StringUtils::Format("interface='%s'",iface);
  }
        
  if (name)
  {
    if (!rules.empty())
      rules+=',';   
    rules += StringUtils::Format("member='%s'",name);
  }
    
  return rules.c_str();     
}

bool CDBusHelper::RegisterMatch(const char *type,const char *path,const char *iface, const char *name)
{
  DBusError error;
  dbus_error_init (&error);   
    
  const char * rule = CDBusHelper::CreateRule(type,path,iface,name);
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: Register Match : %s",rule);
  
  dbus_bus_add_match(m_connection, rule, &error);    
    
  if (dbus_error_is_set(&error))
  {
    CLog::Log(LOGERROR, "DBus: Cannot ADD %s MATCH : Path %s  Iterface : %s Member : %s",type, path, iface,name);
    dbus_error_free (&error);
    return false;
  }
    
  dbus_error_free (&error);          
  return true;        
}

bool CDBusHelper::UnregisterMatch(const char *type,const char *path,const char *iface, const char *name)
{
  DBusError error;
  dbus_error_init (&error);   

  const char * rule = CDBusHelper::CreateRule(type,path,iface,name);
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: UnregisterMatch : %s",rule);
  dbus_bus_remove_match(m_connection, rule, &error);
	
  if (dbus_error_is_set(&error))
  {
    CLog::Log(LOGERROR, "DBus: Cannot REMOVE %s MATCH : Path %s  Iterface : %s Member : %s",type, path, iface,name);     
	dbus_error_free (&error);	
	return false;
  }
	
  dbus_error_free (&error);	
  return true;
}

//deprecated use GET instead
CVariant CDBusUtil::GetVariant(const char *destination, const char *object, const char *interface, const char *property)
{
  return CDBusUtil::Get(destination,object,interface,property);
}

//Helper to get a dbus properties
template <typename T>
bool CDBusUtil::Set(const char *destination, const char *object, const char *interface, const char *property,T value)
{
  return CDBusUtil::CallAsync(destination,object, "org.freedesktop.DBus.Properties", "Set",interface,property,value);
}

CVariant CDBusUtil::Get(const char *destination, const char *object, const char *interface, const char *property)
{
  return CDBusUtil::Call(destination,object, "org.freedesktop.DBus.Properties", "Get",interface,property);
}

CVariant CDBusUtil::GetAll(const char *destination, const char *object, const char *arg)
{
  return CDBusUtil::Call(destination, object, "org.freedesktop.DBus.Properties", "GetAll", arg);
}

//Call without arg
CVariant CDBusUtil::Call(const char *destination, const char *object, const char *interface, const char *method)
{
	CDBusMessage message(destination, object, interface, method);
	DBusMessage *reply = message.SendSystem();
    if (reply)    
	  return CDBusUtil::Decode(reply);		    
}

//Call with arg(s)
template< typename... Args>
CVariant CDBusUtil::Call(const char *destination, const char *object, const char *interface, const char *method, Args... args)
{
  CDBusMessage message(destination, object, interface, method);  
  message.AppendArgument(args...);
  DBusMessage *reply = message.SendSystem();
  if (reply)    
	return CDBusUtil::Decode(reply);		    
}

//Call asynchronous without arg
bool CDBusUtil::CallAsync(const char *destination, const char *object, const char *interface, const char *method)
{
	CDBusMessage message(destination, object, interface, method);
	return message.SendAsyncSystem();
}

//Call asynchronous with arg(s)
template< typename... Args>
bool CDBusUtil::CallAsync(const char *destination, const char *object, const char *interface, const char *method, Args... args)
{ 
  CDBusMessage message(destination, object, interface, method);  
  message.AppendArgument(args...);  
  return message.SendAsyncSystem();
}

CVariant CDBusUtil::Decode(DBusMessage *message)
{
  //If only one args, return args
  //return CVariant array of args
  CVariant properties;
	
  DBusMessageIter iter;
  
  if (dbus_message_iter_init(message, &iter))
  {      
	do
	{
	  properties.push_back(ParseType(&iter));      
    } while (dbus_message_iter_next(&iter));
  }
	  
  if (properties.size() == 1)
	return properties[0];

  return properties;	
}

CVariant CDBusUtil::ParseType(DBusMessageIter *itr)
{
  CVariant value;
  const char *    string  = NULL;
  dbus_int32_t    int32   = 0;
  dbus_uint32_t   uint32  = 0;
  dbus_int64_t    int64   = 0;
  dbus_uint64_t   uint64  = 0;
  dbus_bool_t     boolean = false;
  double          doublev = 0;

  int type = dbus_message_iter_get_arg_type(itr);
  switch (type)
  {
  case DBUS_TYPE_OBJECT_PATH:
  case DBUS_TYPE_STRING:
    dbus_message_iter_get_basic(itr, &string);
    value = string;
    break;
  case DBUS_TYPE_UINT32:
    dbus_message_iter_get_basic(itr, &uint32);
    value = (uint64_t)uint32;
    break;
  case DBUS_TYPE_BYTE:
  case DBUS_TYPE_INT32:
    dbus_message_iter_get_basic(itr, &int32);
    value = (int64_t)int32;
    break;
  case DBUS_TYPE_UINT64:
    dbus_message_iter_get_basic(itr, &uint64);
    value = (uint64_t)uint64;
    break;
  case DBUS_TYPE_INT64:
    dbus_message_iter_get_basic(itr, &int64);
    value = (int64_t)int64;
    break;
  case DBUS_TYPE_BOOLEAN:
    dbus_message_iter_get_basic(itr, &boolean);
    value = (bool)boolean;
    break;
  case DBUS_TYPE_DOUBLE:
    dbus_message_iter_get_basic(itr, &doublev);
    value = (double)doublev;
    break;
  case DBUS_TYPE_ARRAY:
    DBusMessageIter array;
    dbus_message_iter_recurse(itr, &array);
    if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_DICT_ENTRY)
    {    
        //it's a dict inside this array
        value = CVariant::VariantTypeObject;
        DBusMessageIter dictEntry;        
	    do
        {
          dbus_message_iter_recurse(&array,&dictEntry);							
	      const char * key = NULL;
	      dbus_message_iter_get_basic(&dictEntry, &key);
          dbus_message_iter_next(&dictEntry);
          CVariant val = ParseType(&dictEntry);
          if (!val.isNull() && key)
          {         
             value[key] = val;
          }
        }while (dbus_message_iter_next(&array));
	}
	else
	{
      //standard array
      value = CVariant::VariantTypeArray;
      do
      {
        CVariant item = ParseType(&array);
        if (!item.isNull())
          value.push_back(item);
      } while (dbus_message_iter_next(&array));
    }
    break;
  case DBUS_TYPE_VARIANT:
    DBusMessageIter variant;
    dbus_message_iter_recurse(itr, &variant);
    value = ParseType(&variant);
    break;
  }
  CLog::Log(LOGINFO, "BELESE: GetValue %s", value.asString().c_str());
  
  return value;
}

#endif
