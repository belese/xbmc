#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#ifdef HAS_DBUS
#include <dbus/dbus.h>
#include "DBusUtil.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <string>
#include <vector>
#include <queue>
#include <functional>

template <class T>
class CDbusObject : private CDBusHelper
{
public :
  CDbusObject(const char* path);
  ~CDbusObject();

protected:
  typedef void (T::*DBusObjectCb) (CVariant args);

  bool RegisterSignal(const char *iface, const char *name, DBusObjectCb cb);
  
  void Reply();
  template< typename... Args>
  void Reply(Args... args);  
  void ReplyError(const char* name,const char* description);
  
  bool PumpEvent();
  
private :
  typedef struct
  {    
    CVariant args;
    DBusObjectCb handler;
    DBusMessage *reply;
  } DBusObjectQueueInfo;
  
  std::queue<DBusObjectQueueInfo> m_queueinfo ;
  
  typedef struct
  {        
    const char* iface;
    const char* name;
    DBusObjectCb handler;
  } DBusObjectSignalInfo;
  
  std::vector<DBusObjectSignalInfo> m_objectinfo;      
    
  const char * m_path;
  DBusMessage *m_curreply;
  
  bool FilterMessage(DBusMessage *msg);    
  static DBusHandlerResult onMessage(DBusConnection *connection, DBusMessage *msg, void *instance);
};

template <class T>
CDbusObject<T>::CDbusObject(const char *path)
{ 
	m_path = path	
	//register dbus object
	if (m_connection)
	{
	  static const DBusObjectPathVTable vtable = { 0 };
	
	  DBusError error;
      dbus_error_init(&error);
    
      dbus_connection_register_object_path(m_connection, m_path, &vtable, &error);
    
      if (dbus_error_is_set(&error))    
        CLog::Log(LOGERROR, "DBus: Error %s - %s", error.name, error.message);
    
      dbus_error_free(&error);
		
	  if (!dbus_connection_add_filter(m_connection,CDbusObject<T>::onMessage,this,NULL))
        CLog::Log(LOGERROR, "DBus: Cannot register CB filter function");
    }
}

template <class T>
CDbusObject<T>::~CDbusObject()
{
  for(unsigned int i =0 ;i < m_objectinfo.size();i++)
  {
	UnregisterMatch("method",m_objectinfo[i].path,m_objectinfo[i].iface,m_objectinfo[i].name);    
  }  
  //remove filter CB
  //remove object path
}

template <class T>
bool CDbusObject<T>::RegisterMethod(const char *iface, const char *name, DBusHandlerCb cb)
{	
    if ( !RegisterMatch ( "method", m_path, iface, name) )
      return false;
       
    DBusHandlerSignalInfo info = {iface, name, cb };
    m_objectinfo.push_back( info);
    CLog::Log(LOGINFO, "Belese : Register Signal Match %s %s %s",path,iface,name);
    return true;   
}


template <class T>
bool CDbusObject<T>::FilterMessage(DBusMessage *msg)
{	
	if (!dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD && !dbus_message_has_path(msg,m_path))
      return false;
    
    bool globalhandled = false;
    
    for (unsigned int i = 0; i < m_objectinfo.size(); i++)
    {
    	bool handled = true;		
		if (handled && m_objectinfo[i].iface)
		  handled &= dbus_message_has_interface(msg,m_objectinfo[i].iface);
		if (handled && m_objectinfo[i].name)
		  handled &= dbus_message_has_member(msg,m_objectinfo[i].name);    
		
		if (handled)
		{
		  globalhandled = true;		  
		  CVariant properties = CDBusUtil::Decode(msg);      
		  DBusHandlerQueueInfo info = {properties,m_signalinfo[i].handler,msg};
	      m_queueinfo.push(info);	      
	    }
	}
    return globalhandled;  
}

template <class T>
DBusHandlerResult CDbusObject<T>::onMessage(DBusConnection *connection, DBusMessage *msg, void *instance)
{	
  //static proxy to call instance filter funciton
  bool handled = ((CDbusObject<T>*)instance)->FilterMessage(msg);
    
  if (!handled)
  {
    dbus_message_unref(msg);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  //keep message for reply, unref once reply is done
  return DBUS_HANDLER_RESULT_HANDLED;	    
}  

template <class T>
bool CDbusObject<T>::PumpEvent()
{
	//Check new message and dispatch
	//This will dispatch message for all class that has registered signal or method
	//and call all filter CB	
	CLog::Log(LOGINFO, "Belese : PumpEvent");
	ReadAndDispatchDbusMessage();
	
	if (m_queueinfo.empty())
	  return false;
	
	CLog::Log(LOGINFO, "Belese : We have a new msg!!!");
	
	//empty queue and call all matching register CBs
	while (!m_queueinfo.empty())
    {
        DBusHandlerQueueInfo msg = m_queueinfo.front();
        m_queueinfo.pop();        
        m_curreply = msg.reply;
        ((T*)this->*(msg.handler))(msg.args);
        dbus_message_unref(m_curreply);
        m_curreply = NULL;
    }
    
    return true;
}
#endif


















#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#ifdef HAS_DBUS
#include <dbus/dbus.h>
#include "DBusUtil.h"
#include "utils/log.h"

#include <string>
#include <vector>

template <class T>
class CDBusObject
{
public :
  CDBusObject(const char * path);
  ~CDBusObject();
  
  const char* GetPath() { return m_path.c_str(); }

protected:
  typedef void (T::*DBusMethodCb) (CVariant args);

  bool RegisterMethod(const char *iface, const char *name, DBusMethodCb cb);
  bool PumpEvent();
  
  template< typename... Args>
  void Reply(Args... args);
  
  void ReplyError();
  
private :
  typedef struct
  {
    unsigned int id;
    DBusMethodCb handler;
  } DBusObjectInfo;
  
  std::string m_path;
  std::vector<DBusObjectInfo> m_objectinfo;  
  std::queue<DBusQueueInfo> m_queueinfo ;  
};


bool CDBusDispatcher::RegisterObject(const char *path)
{
	static const DBusObjectPathVTable vtable = { 0 };
    DBusError error;
    dbus_error_init(&error);
    
    dbus_connection_register_object_path(m_connection, path, &vtable, &error);
    
    if (dbus_error_is_set(&error))
    {
      CLog::Log(LOGERROR, "DBus: Error %s - %s", error.name, error.message);     
      dbus_error_free(&error);
      return false;
    }
    
    dbus_error_free(&error);
    return true;    
}

template <class T>
CDBusObject<T>::CDBusObject(const char *path)
{
  if (path!=NULL)
  {
    m_path = path;
    CDBusDispatcher::RegisterObject(m_path);
  }
}

template <class T>
CDBusObject<T>::~CDBusObject()
{
  if (!m_path.empty())
    CDBusDispatcher::UnregisterObject(m_path);
}

template <class T>
void CDBusObject<T>::RegisterMethod(const char *iface, const char *name, DBusObjectHandler method)
{
  int id = CDBusDispatcher::RegisterMatch('method',m_path,iface,name,&m_queueinfo);
  
  if( id == 0 )
    return false;
    
  DBusHandlerInfo info = {id, cb};
  m_handlerinfo.push_back(info); 
  
  CLog::Log(LOGINFO, "Belese : Register method : %s %s %s",m_path,iface,name);
  
  return true;
}

template <class T,typename U,typename... Args>
void CDBusObject<T>::AddArgument(CDBusMessage *message,U t,Args... args) 
{    
    message->AppendArgument(t);
    AddArgument(message,args...);    
}

template <class T,typename U>
void CDBusObject<T>::AddArgument(CDBusMessage *message,U t) 
{
    CLog::Log(LOGINFO, "DBus: AddLastArgument %s", t);
    message->AppendArgument(t);
}
#endif
