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
#include "settings/AdvancedSettings.h"

template <class T>
class CDbusHandler : private CDBusHelper
{
public :
  CDbusHandler();
  ~CDbusHandler();

protected:
  typedef void (T::*DBusHandlerCb) (CVariant args);

  bool RegisterSignal(const char *path,const char *iface, const char *name, DBusHandlerCb cb);
  bool PumpEvent();

private :
  typedef struct
  {
    CVariant args;
    DBusHandlerCb handler;
  } DBusHandlerQueueInfo;

  std::queue<DBusHandlerQueueInfo> m_queueinfo;

  typedef struct
  {
    const char* path;
    const char* iface;
    const char* name;
    DBusHandlerCb handler;
  } DBusHandlerSignalInfo;

  std::vector<DBusHandlerSignalInfo> m_signalinfo;

  bool FilterMessage(DBusMessage *msg);

  static DBusHandlerResult onMessage(DBusConnection *connection, DBusMessage *msg, void *instance);
};

template <class T>
CDbusHandler<T>::CDbusHandler()
{
  if (!dbus_connection_add_filter(m_connection,CDbusHandler<T>::onMessage,this,NULL))
    CLog::Log(LOGERROR, "DBus: Cannot register CB filter function");
}

template <class T>
CDbusHandler<T>::~CDbusHandler()
{
  for(unsigned int i =0 ;i < m_signalinfo.size();i++)
  {
    if (g_advancedSettings.CanLogComponent(LOGDBUS))
      CLog::Log(LOGDEBUG, "DBus : Unregister Signal Match %s %s %s",m_signalinfo[i].path,m_signalinfo[i].iface,m_signalinfo[i].name);
    UnregisterMatch("signal",m_signalinfo[i].path,m_signalinfo[i].iface,m_signalinfo[i].name);
  }
}

template <class T>
bool CDbusHandler<T>::RegisterSignal(const char *path,const char *iface, const char *name, DBusHandlerCb cb)
{
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus : Register Signal Match %s %s %s",path,iface,name);

  if ( !RegisterMatch ("signal", path, iface, name) )
    return false;

  DBusHandlerSignalInfo info = { path, iface, name, cb };
  m_signalinfo.push_back( info);

  return true;
}

template <class T>
bool CDbusHandler<T>::FilterMessage(DBusMessage *msg)
{
  if (!dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL)
    return false;

  bool globalhandled = false;

  for (unsigned int i = 0; i < m_signalinfo.size(); i++)
  {
    bool handled = true;
    if (m_signalinfo[i].path)
      handled &= dbus_message_has_path(msg,m_signalinfo[i].path);
    if (handled && m_signalinfo[i].iface)
      handled &= dbus_message_has_interface(msg,m_signalinfo[i].iface);
    if (handled && m_signalinfo[i].name)
      handled &= dbus_message_has_member(msg,m_signalinfo[i].name);

    if (handled)
    {
      globalhandled = true;
      CVariant properties = CDBusUtil::Decode(msg);
      DBusHandlerQueueInfo info = {properties,m_signalinfo[i].handler};
      m_queueinfo.push(info);
    }
  }
  return globalhandled;
}

//static proxy to call instance filter funciton
template <class T>
DBusHandlerResult CDbusHandler<T>::onMessage(DBusConnection *connection, DBusMessage *msg, void *instance)
{
  bool handled = ((CDbusHandler<T>*)instance)->FilterMessage(msg);

  dbus_message_unref(msg);

  if (!handled)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  return DBUS_HANDLER_RESULT_HANDLED;
}

template <class T>
bool CDbusHandler<T>::PumpEvent()
{
  //Check new message and dispatch
  //This will dispatch message for all class that has registered signal or method
  //and call all filter CB
  ReadAndDispatchDbusMessage();

  if (m_queueinfo.empty())
    return false;

  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus : We have a new Message in queue, call the handler");

  //empty queue and call all matching register CBs
  while (!m_queueinfo.empty())
  {
    DBusHandlerQueueInfo msg = m_queueinfo.front();
    m_queueinfo.pop();
    ((T*)this->*(msg.handler))(msg.args);
  }

  return true;
}
#endif
