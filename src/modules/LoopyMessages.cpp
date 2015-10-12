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

#include <LoopyMessages.h>
#include <Logger.h>
#include <Utility.h>
#include <Storage.h>
#include <Node.h>


extern "C"{
#include <stdlib.h>
}

    LoopyMessages::LoopyMessages(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
: Module(moduleId, node, cm, name, storageSlot)
{
    //Register callbacks n' stuff
    Logger::getInstance().enableTag("LOOPY");

    //Save configuration to base class variables
    //sizeof configuration must be a multiple of 4 bytes
    configurationPointer = &configuration;
    configurationLength = sizeof(LoopyMessagesConfiguration);

    //Start module configuration loading
    LoadModuleConfiguration();
}

void LoopyMessages::ConfigurationLoadedHandler()
{
    //Does basic testing on the loaded configuration
    Module::ConfigurationLoadedHandler();

    //Version migration can be added here
    if(configuration.moduleVersion == 1){/* ... */};

    //Do additional initialization upon loading the config


    //Start the Module...

}

void LoopyMessages::TimerEventHandler(u16 passedTime, u32 appTimer)
{
    //Do stuff on timer...

}

void LoopyMessages::ResetToDefaultConfiguration()
{
    //Set default configuration values
    configuration.moduleId = moduleId;
    configuration.moduleActive = false;
    configuration.moduleVersion = 1;

    //Set additional config values...

}

bool LoopyMessages::TerminalCommandHandler(string commandName, vector<string> commandArgs)
{
    if(commandName == "loopy"){
        //Get the id of the target node
        nodeID targetNodeId = atoi(commandArgs[0].c_str());
        logt("LOOPY", "Trying to send message to node %u", targetNodeId);

        //Send loopy message to that node
        connPacketModuleAction packet;
        packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
        packet.header.sender = node->persistentConfig.nodeId;
        packet.header.receiver = targetNodeId;

        packet.moduleId = moduleId;
        packet.actionType = LoopyMessagesTriggerActionMessages::TRIGGER_MESSAGE;
        packet.data[0] = 123;


        cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE_ACTION + 1, true);
        return true;
    }

    //React on commands, return true if handled, false otherwise
    if(commandArgs.size() >= 2 && commandArgs[1] == moduleName)
    {
        if(commandName == "uart_module_trigger_action")
        {
            if(commandArgs[1] != moduleName) return false;

            if(commandArgs.size() == 3 && commandArgs[2] == "argument_a")
            {


                return true;
            }
            else if(commandArgs.size() == 3 && commandArgs[2] == "argument_b")
            {


                return true;
            }

            return true;

        }
    }

    //Must be called to allow the module to get and set the config
    return Module::TerminalCommandHandler(commandName, commandArgs);
}


void LoopyMessages::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength)
{
    //Must call superclass for handling
    Module::ConnectionPacketReceivedEventHandler(inPacket, connection, packetHeader, dataLength);

    if(packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
        connPacketModuleAction* packet = (connPacketModuleAction*)packetHeader;

        //Check if our module is meant and we should trigger an action
        if(packet->moduleId == moduleId){
            if(packet->actionType == LoopyMessagesTriggerActionMessages::TRIGGER_MESSAGE){
                logt("LOOPY", "Loopy message received with data: %d", packet->data[0]);

                //Send Response acknowledgement
                connPacketModuleAction outPacket;
                outPacket.header.messageType = MESSAGE_TYPE_MODULE_ACTION_RESPONSE;
                outPacket.header.sender = node->persistentConfig.nodeId;
                outPacket.header.receiver = packetHeader->sender;

                outPacket.moduleId = moduleId;
                outPacket.actionType = LoopyMessagesActionResponseMessages::RESPONSE_MESSAGE;
                outPacket.data[0] = packet->data[0];
                outPacket.data[1] = 111;

                cm->SendMessageToReceiver(NULL, (u8*)&outPacket, SIZEOF_CONN_PACKET_MODULE_ACTION + 2, true);

            }
        }
    }

    //Parse Module responses
    if(packetHeader->messageType == MESSAGE_TYPE_MODULE_ACTION_RESPONSE){
        connPacketModuleAction* packet = (connPacketModuleAction*)packetHeader;

        //Check if our module is meant and we should trigger an action
        if(packet->moduleId == moduleId)
        {
            if(packet->actionType ==LoopyMessagesActionResponseMessages::RESPONSE_MESSAGE)
            {
                 logt("LOOPY", "Loopy message came back from %u with data %d, %d", packet->header.sender, packet->data[0], packet->data[1]);
            }
        }
    }
}

