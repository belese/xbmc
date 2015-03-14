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

#include "IBluetoothSysCall.h"
#include "IBluetoothProfileSysCall.h"
#include <vector>

class CNullBluetoothSyscall : public IBluetoothSysCall
{
public:
  void SetDiscovery(bool state) {}
  void SetDiscoverable(bool state) {}
  void SetPower(bool state) {}
    
  bool IsPowered() {return false;}
  bool IsDiscoverable() {return false;}
  
  bool registerInterface(const char *UUID) {return false;}	
  const char * getDefaultAdapter() {return NULL;}
  
  void OnAdapterFound(const char* path) {}
  void OnAdapterDisappeared(const char* path) {}
  
  void OnInterfaceAdded(const char* UUID) {}
  void OnInterfaceRemoved(const char* UUID) {}
};

class CBluetoothManager : public IBlueToothEventsCallback
{
public:
  CBluetoothManager();
  ~CBluetoothManager();

  void Initialise();
  void InitialiseAdapter(const char* adapter);
  
  void SetDiscovery(bool state);  
  void SetDiscoverable(bool state);
  void SetPower(bool state); 
    
  bool IsPowered();
  bool IsDiscoverable();
  
  bool RegisterUuid(const char* uuid,IBlueToothProfileCallback* callback);
	
private:
  typedef struct
  {
	  const char* uuid;
	  IBlueToothProfileCallback* callback;
  } BluetoothProfileInfo;
  
  std::vector<BluetoothProfileInfo> m_profile;
  
  void OnAdapterFound(const char* path);
  void OnAdapterDisappeared(const char* path);
  
  void OnInterfaceAdded(const char* UUID);
  void OnInterfaceRemoved(const char* UUID);

  const char* m_adapter;
  IBluetoothSysCall *m_instance;
};

extern CBluetoothManager g_bluetoothManager;

