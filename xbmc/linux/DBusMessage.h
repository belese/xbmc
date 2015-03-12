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
#include <dbus/dbus.h>
#include <vector>
#include <map>
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

class CDBusMessage
{
public:
  //DBUS Call Method Message
  CDBusMessage(const char *destination, const char *object, const char *interface, const char *method);  
  //DBUS Call Reply Message
  CDBusMessage(DBusMessage *replyto);  
  //DBUS Call Reply Error Message
  CDBusMessage(DBusMessage *replyto,const char *name, const char *message);  
  //DBUS Signal Message
  CDBusMessage(const char *object, const char *interface, const char *name);
    	
  ~CDBusMessage();

  DBusMessage *SendSystem();
  DBusMessage *SendSession();
  
  bool SendAsyncSystem();
  bool SendAsyncSession();
  
  //Theses ones are let public for compatibility
  //but shouldn't be use in public context
  DBusMessage *Send(DBusBusType type);
  DBusMessage *Send(DBusConnection *con, DBusError *error);
    
  template<typename... T> bool AppendArgument(const T... args);
  template<typename T> bool AppendArgument(const T arg);
  
  //use recursively for variatic template
  template <typename T,typename... U> bool AppendArgument(T t,U... args);    
  //use recursively for variatic template and array args  
  template<typename T, typename... U> bool AppendArgument(const T *array, unsigned int length,U...args);  
  
  template<typename T> bool AppendArgument(const T *array, unsigned int length);
  template<typename K, typename V> bool AppendArgument(const std::map<K,V> *map);  
  
              
private:
  static const char* GetSignatureFromType(int type);
    
  static const int GetDbusTypeFromArg(const char *string) {return DBUS_TYPE_STRING;};
  static const int GetDbusTypeFromArg(const unsigned char byte) {return DBUS_TYPE_BYTE;};
  static const int GetDbusTypeFromArg(const bool boolean) {return DBUS_TYPE_BOOLEAN;};
  static const int GetDbusTypeFromArg(const int16_t integer16) {return DBUS_TYPE_INT16;};  
  static const int GetDbusTypeFromArg(const int32_t integer32) {return DBUS_TYPE_INT32;};  
  static const int GetDbusTypeFromArg(const int64_t  integer64) {return DBUS_TYPE_INT64;};
  static const int GetDbusTypeFromArg(const uint16_t uinteger16) {return DBUS_TYPE_UINT16;};
  static const int GetDbusTypeFromArg(const uint32_t uinteger32) {return DBUS_TYPE_UINT32;};  
  static const int GetDbusTypeFromArg(const uint64_t uinteger64) {return DBUS_TYPE_UINT64;};
  static const int GetDbusTypeFromArg(const double vdouble) {return DBUS_TYPE_DOUBLE;};
  
  bool SendAsync(DBusBusType type);

  void Close();
  void PrepareArgument();

  bool OpenContainer(const int type,const char *signature);
  bool CloseContainer();
  
  bool isRoot();
    
  template<typename T> bool AppendBasic(int type,const T arg);  
  template<typename T> bool AppendArray(const T *array, unsigned int length,int type);
  template<typename K, typename V> bool AppendDictEntry(const K *key,const V *value);       
  template<typename K, typename V> bool AppendDict(const std::map<K,V> *map,int keytype,int valuetype);
  template<typename U> bool AppendVariant(const U *value,int type);
  
  DBusMessage *m_message;
  DBusMessage *m_reply;
  DBusMessageIter m_currentItr;
  
  std::vector<DBusMessageIter> m_itr;
  bool m_haveArgs;
};

template< typename... T> 
bool CDBusMessage::AppendArgument(const T... args)
{
	return AppendArgument(args...);
}

template<typename T> 
bool CDBusMessage::AppendArgument(const T arg)
{
  T value = arg;
  return AppendBasic(CDBusMessage::GetDbusTypeFromArg(arg),(T*)&value);
}
 
template<typename K, typename V> 
bool CDBusMessage::AppendArgument(const std::map<K,V> *map)
{
	K keytype;
	V valuetype;
	return AppendDict(map,CDBusMessage::GetDbusTypeFromArg(keytype),CDBusMessage::GetDbusTypeFromArg(valuetype));
}


template <typename T,typename... U>
bool CDBusMessage::AppendArgument(T t,U... args) 
{
  bool success = AppendArgument(t);
  success &= AppendArgument(args...); 
  return success; 
}

template<typename T, typename... U> 
bool CDBusMessage::AppendArgument(const T *array, unsigned int length,U...args)
{
  bool success = AppendArgument(array,length);
  success &= AppendArgument(args...);
  return success;
}

template<typename T> 
bool CDBusMessage::AppendArgument(const T *array, unsigned int length)
{
	T arraytype;
	return AppendArray(array,length,CDBusMessage::GetDbusTypeFromArg(arraytype));
}


template<typename T> 
bool CDBusMessage::AppendBasic(int type,const T value)
{
  PrepareArgument();  
  bool rc = dbus_message_iter_append_basic(&m_currentItr, type, value);
  if (!rc)
     CLog::Log(LOGERROR, "DBus: Cannot Basic AppendArgument %d %s",type, *value);
  return rc;
} 

template<typename T> 
bool CDBusMessage::AppendArray(const T *array, unsigned int length,int type)
{
  PrepareArgument();
  bool success = OpenContainer(DBUS_TYPE_ARRAY,GetSignatureFromType(type));
  for (unsigned int i = 0; i < length && success; i++)
  {    
      success &=AppendArgument(array[i]);   
  }  
  success &=CloseContainer();

  return success;
}

template<typename K, typename V>
bool CDBusMessage::AppendDict(const std::map<K,V> *map,int keytype,int valtype)
{
  PrepareArgument();
  std::string signature = StringUtils::Format("a{%s%s}",GetSignatureFromType(keytype),GetSignatureFromType(valtype));
  bool success = OpenContainer(DBUS_TYPE_ARRAY,signature.c_str());
  for (typename std::map<K,V>::const_iterator it=map->begin(); it!=map->end(); ++it)
  {
		success &= AppendDictEntry(it->first.c_str(),&it->second);
  }	
  success &= CloseContainer();
  return success;	  
}

template<typename K,typename V> 
bool CDBusMessage::AppendDictEntry(const K *key,const V *data)
{
	bool success = OpenContainer(DBUS_TYPE_DICT_ENTRY,NULL);
	success &= AppendArgument(key);	
    success &= AppendArgument(data);    
	success &= CloseContainer();
	return success;
}

template<typename T> 
bool CDBusMessage::AppendVariant(const T *value,int type)
{
  PrepareArgument();
  bool success = OpenContainer(DBUS_TYPE_VARIANT,GetSignatureFromType(type));
  success &= AppendArgument(value);
  success &= CloseContainer();
  return success;
}

#endif
