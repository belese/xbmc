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
#include <string>

class IBlueToothEventsCallback
{
public:
  virtual ~IBlueToothEventsCallback() {}
  
  virtual void OnAdapterFound(const char* path)         = 0;
  virtual void OnAdapterDisappeared(const char* path)   = 0;
  
  virtual void OnInterfaceAdded(const char* UUID)       = 0;
  virtual void OnInterfaceRemoved(const char* UUID)     = 0;
};

class IBluetoothSysCall : public IBlueToothEventsCallback
{
public:	    
  IBluetoothSysCall();
  ~IBluetoothSysCall();
  
  static const char * getDefaultAdapter();
  
  virtual void SetDiscovery(bool state)            = 0;  
  virtual void SetDiscoverable(bool state)         = 0;
  virtual void SetPower(bool state)                = 0; 
    
  virtual bool IsPowered()                         = 0;
  virtual bool IsDiscoverable()                    = 0;
  
  virtual bool registerInterface(const char *UUID) = 0;
};

class IBlueToothDevice
{
public:
  enum DeviceType
  {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_HEADSET,
    DEVICE_TYPE_HEADPHONES,
    DEVICE_TYPE_AUDIO,
    DEVICE_TYPE_PHONE,
    DEVICE_TYPE_COMPUTER,
    DEVICE_TYPE_MODEM,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_VIDEO,
    DEVICE_TYPE_JOYPAD,
    DEVICE_TYPE_TABLET,
    DEVICE_TYPE_PRINTER,
    DEVICE_TYPE_CAMERA
  };
  virtual ~IBlueToothDevice() {};
  
  virtual std::string& GetName(void)  = 0;   
    
  //virtual DeviceType GetDeviceType()  = 0; 
};
