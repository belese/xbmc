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
#include "DBusMessage.h"
#ifdef HAS_DBUS
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "utils/Variant.h"
#include "DBusUtil.h"

CDBusMessage::CDBusMessage(const char *destination, const char *object, const char *interface, const char *method)
{
  m_reply = NULL;
  m_message = dbus_message_new_method_call (destination, object, interface, method);
  m_haveArgs = false;
  
  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: Creating message to %s on %s with interface %s and method %s\n", destination, object, interface, method);
}

CDBusMessage::CDBusMessage(DBusMessage *replyto) 
{
  m_reply = NULL;
  m_message = dbus_message_new_method_return(replyto);
  m_haveArgs = false;

  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: Creating Reply message\n");
}

CDBusMessage::CDBusMessage(DBusMessage *replyto,const char *name, const char *message)
{
  m_reply = NULL;
  m_message = dbus_message_new_error(replyto,name,message);
  m_haveArgs = false;

  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: Creating Error Reply message %s : %s\n",name,message);
}

CDBusMessage::CDBusMessage(const char *object, const char *interface, const char *name)
{
  m_reply = NULL;
  m_message = dbus_message_new_signal(object,interface,name);
  m_haveArgs = false;

  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: Creating Signal Message on %s with interface %s  : %s\n",object,name,interface);
}

template<> 
bool CDBusMessage::AppendArgument(const CVariant variant)
{ 
  bool success = true;
  
  if (variant.isByte())
  {
     const char value = variant.asByte();
     success &= AppendVariant(&value,DBUS_TYPE_BYTE);
  }
  else if (variant.isInteger())
  {
     const int value = variant.asInteger();
     success &= AppendVariant(&value,DBUS_TYPE_INT32);
  }
  else if (variant.isUnsignedInteger())
  {
     const unsigned int value = variant.asUnsignedInteger();
     success &= AppendVariant(&value,DBUS_TYPE_UINT32);
  }
  else if (variant.isString())
  {
	 const char* value = variant.asString().c_str();
     success &= AppendVariant(&value,DBUS_TYPE_STRING);
  }
  else if (variant.isBoolean())
  {
	 const bool value = variant.asBoolean();
     success &= AppendVariant(&value,DBUS_TYPE_BOOLEAN);
  }
  else if (variant.isDouble())
  {
	 const double value = variant.asDouble();
     success &= AppendVariant(&value,DBUS_TYPE_DOUBLE);
  }
  return success;  
}


CDBusMessage::~CDBusMessage()
{
  Close();
}

const char* CDBusMessage::GetSignatureFromType(int type) 
{
    switch (type) {
        case DBUS_TYPE_BOOLEAN: return DBUS_TYPE_BOOLEAN_AS_STRING;
        case DBUS_TYPE_BYTE: return DBUS_TYPE_BYTE_AS_STRING;
        case DBUS_TYPE_INT16: return DBUS_TYPE_INT16_AS_STRING;
        case DBUS_TYPE_UINT16: return DBUS_TYPE_UINT16_AS_STRING;
        case DBUS_TYPE_INT32: return DBUS_TYPE_INT32_AS_STRING;
        case DBUS_TYPE_UINT32: return DBUS_TYPE_UINT32_AS_STRING;
        case DBUS_TYPE_INT64: return DBUS_TYPE_INT64_AS_STRING;
        case DBUS_TYPE_UINT64: return DBUS_TYPE_UINT64_AS_STRING;
        case DBUS_TYPE_DOUBLE: return DBUS_TYPE_DOUBLE_AS_STRING;
        case DBUS_TYPE_STRING: return DBUS_TYPE_STRING_AS_STRING;
        case DBUS_TYPE_OBJECT_PATH: return DBUS_TYPE_OBJECT_PATH_AS_STRING;
        case DBUS_TYPE_SIGNATURE: return DBUS_TYPE_SIGNATURE_AS_STRING;        
        default : return NULL;
    }
}

bool CDBusMessage::OpenContainer(const int type,const char *signature)
{
  PrepareArgument();
  DBusMessageIter sub;
  bool success = dbus_message_iter_open_container(&m_currentItr,type, signature , &sub);
  m_itr.push_back(m_currentItr);
  m_currentItr = sub;
  return success;
}

bool CDBusMessage::CloseContainer()
{
  if (m_itr.empty())
	return false;
  
  DBusMessageIter sub;
  sub = m_currentItr;
  m_currentItr = m_itr.back();
  m_itr.pop_back();
  return dbus_message_iter_close_container(&m_currentItr,&sub);
}

DBusMessage *CDBusMessage::SendSystem()
{
  return Send(DBUS_BUS_SYSTEM);
}

DBusMessage *CDBusMessage::SendSession()
{
  return Send(DBUS_BUS_SESSION);
}

bool CDBusMessage::SendAsyncSystem()
{
  return SendAsync(DBUS_BUS_SYSTEM);
}

bool CDBusMessage::SendAsyncSession()
{
  return SendAsync(DBUS_BUS_SESSION);
}

DBusMessage *CDBusMessage::Send(DBusBusType type)
{
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(type, &error);

  if (dbus_error_is_set(&error))
  {
    CLog::Log(LOGERROR, "DBus: Cannot Get Dbus : %s - %s", error.name, error.message);
    dbus_error_free (&error);  
    return false;
  }
  
  dbus_error_init (&error);
  
  DBusMessage *returnMessage = Send(con, &error);

  if (dbus_error_is_set(&error))
    CLog::Log(LOGERROR, "DBus: Error Cannot send message %s - %s", error.name, error.message);

  dbus_error_free (&error);
  dbus_connection_unref(con);

  return returnMessage;
}

bool CDBusMessage::SendAsync(DBusBusType type)
{
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(type, &error);

  bool result;
  if (con && m_message)
    result = dbus_connection_send(con, m_message, NULL);
  else
    result = false;

  dbus_error_free (&error);
  dbus_connection_unref(con);
  return result;
}

DBusMessage *CDBusMessage::Send(DBusConnection *con, DBusError *error)
{
  if (con && m_message)
  {
    if (m_reply)
      dbus_message_unref(m_reply);
      
    m_reply = dbus_connection_send_with_reply_and_block(con, m_message, -1, error);
  }

  return m_reply;
}

void CDBusMessage::Close()
{
  if (m_message)
    dbus_message_unref(m_message);

  if (m_reply)
    dbus_message_unref(m_reply);
}

void CDBusMessage::PrepareArgument()
{    
  if (!m_haveArgs)
  {   
    dbus_message_iter_init_append(m_message, &m_currentItr);    
  }  

  m_haveArgs = true;
}
#endif
