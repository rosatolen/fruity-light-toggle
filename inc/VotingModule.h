/**

  Copyright (c) 2014-2015 "M-Way Solutions GmbH"
  FruityMesh - Bluetooth Low Energy mesh protocol [http://mwaysolutions.com/]

  This file is part of FruityMesh

  FruityMesh is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <Module.h>

class VotingModule: public Module
{
	private:

		//Module configuration that is saved persistently (size must be multiple of 4)
		struct VotingModuleConfiguration : ModuleConfiguration{
			//Insert more persistent config values here
			//Insert more persistent config values here
			u16 connectionReportingIntervalMs;
			u16 statusReportingIntervalMs;
			u8 connectionRSSISamplingMode; //typeof RSSISampingModes
			u8 advertisingRSSISamplingMode; //typeof RSSISampingModes
			u16 reserved;
		};

		VotingModuleConfiguration configuration;

		enum VotingModuleTriggerActionMessages{
			TRIGGER_MESSAGE = 0
		};

		enum VotingModuleActionResponseMessages{
			RESPONSE_MESSAGE = 0
		};

		/*
		//####### Module messages (these need to be packed)
#pragma pack(push)
#pragma pack(1)

#define SIZEOF_TEMPLATE_MODULE_***_MESSAGE 10
typedef struct
{
		//Insert values here

		}VotingModule***Message;

#pragma pack(pop)
//####### Module messages end
		 */
u32 lastConnectionReportingTimer;
u32 lastStatusReportingTimer;

public:
VotingModule(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot);

void ConfigurationLoadedHandler();

void ResetToDefaultConfiguration();

void TimerEventHandler(u16 passedTime, u32 appTimer);

//void BleEventHandler(ble_evt_t* bleEvent);

void ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength);

//void NodeStateChangedHandler(discoveryState newState);

bool TerminalCommandHandler(string commandName, vector<string> commandArgs);
};
