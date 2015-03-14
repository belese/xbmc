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
#include "BluetoothManager.h" 
#include "utils/log.h"

CBluetoothManager::CBluetoothManager()
{
}

void CBluetoothManager::Initialise()
{
  //get the default adapter
  const char* adapter = IBluetoothSysCall::getDefaultAdapter();
  if(!adapter)
	CLog::Log(LOGINFO, "Bluetooth: no adapter found");
  else
    InitialiseAdapter(adapter);   
    m_instance = new CNullBluetoothSyscall(); //todo delete in destructor  
}

void CBluetoothManager::InitialiseAdapter(const char* adapter)
{
	m_adapter = adapter;
}

void CBluetoothManager::SetDiscovery(bool state)
{
	m_instance->SetDiscovery(state);
}
  
void CBluetoothManager::SetDiscoverable(bool state)
{
	m_instance->SetDiscoverable(state);
}

void CBluetoothManager::SetPower(bool state)
{
	m_instance->SetPower(state);
}
    
bool CBluetoothManager::IsPowered()
{
	return m_instance->IsPowered();
}

bool CBluetoothManager::IsDiscoverable()
{
	return m_instance->IsDiscoverable();
}
  
bool CBluetoothManager::RegisterUuid(const char *uuid,IBlueToothProfileCallback* callback)
{
  BluetoothProfileInfo info = {uuid,callback};
  m_profile.push_back(info);
  return true;
}
void CBluetoothManager::OnAdapterFound(const char* adapter)
{	
  CLog::Log(LOGINFO, "Bluetooth: new adapter found");	
  if (m_adapter != NULL)
  {
	//we have already a adapter, do nothing  
	return;
  }
  
  InitialiseAdapter(adapter);    	
}

void CBluetoothManager::OnAdapterDisappeared(const char* adapter)
{
	if (strcmp(m_adapter,adapter) == 0)
	{
	  m_adapter = NULL;
	  Initialise();
	}
}

void CBluetoothManager::OnInterfaceAdded(const char* uuid)
{
	//look if we have registered profile for this uuid
	for(unsigned int i = 0; i < m_profile.size(); i++)
	{
	  if (strcmp(m_profile[i].uuid,uuid) == 0)
	    (m_profile[i].callback)->OnProfileAdded(uuid);	    
	}	
}

void CBluetoothManager::OnInterfaceRemoved(const char* uuid)
{
	for(unsigned int i = 0; i < m_profile.size(); i++)
	{
	  if (strcmp(m_profile[i].uuid,uuid) == 0)
	    (m_profile[i].callback)->OnProfileRemoved(uuid);	    
	}
}

