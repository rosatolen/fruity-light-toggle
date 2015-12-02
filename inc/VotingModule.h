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

  You should have received a copy of the GNU General Public License ̨
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <Module.h>

class VotingModule: public Module
{
	private:

		struct VotingModuleConfiguration : ModuleConfiguration{
			u16 connectionReportingIntervalMs;
			u16 statusReportingIntervalMs;
			u8 connectionRSSISamplingMode;
			u8 advertisingRSSISamplingMode;
			u16 reserved;
		};

		VotingModuleConfiguration configuration;

		enum VotingModuleTriggerActionMessages{
			TRIGGER_MESSAGE = 0
		};

		enum VotingModuleActionResponseMessages{
			RESPONSE_MESSAGE = 0
		};

u32 lastConnectionReportingTimer;
u32 lastStatusReportingTimer;

public:
VotingModule(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot);

void ConfigurationLoadedHandler();

void ResetToDefaultConfiguration();

void TimerEventHandler(u16 passedTime, u32 appTimer);

void ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength);

bool TerminalCommandHandler(string commandName, vector<string> commandArgs);
};
