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

#include <VotingModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Storage.h>
#include <Node.h>
#include "pca10028.h"


extern "C"{
#include <stdlib.h>
#include "app_button.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "app_timer.h"
#include "app_gpiote.h"
}

#define APP_TIMER_PRESCALER     0
#define APP_TIMER_MAX_TIMERS    1
#define APP_TIMER_OP_QUEUE_SIZE 2
#define BUTTON_DEBOUNCE_DELAY   50
#define APP_GPIOTE_MAX_USERS    1

static bool initialized = false;
static bool acknowledged = false;

static void vote() {
    ConnectionManager *cm = ConnectionManager::getInstance();
    Logger::getInstance().enableTag("VOTING");
    Node *node = Node::getInstance();
    Conf *config = Conf::getInstance();

    nodeID everyone = 0; // send message to myself
    logt("VOTING", "Trying to vote.\n");
    connPacketModule packet;
    packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
    packet.header.sender = node->persistentConfig.nodeId;
    packet.header.receiver = everyone;
    packet.moduleId = moduleID::LOOPY_MESSAGES_ID;
    packet.actionType = 0; // hardcoded from the reference VotingModule.h
    const char userId[] = "1234";
    strncpy((char *) packet.data, userId, 5);
    logt("VOTING", "Sending message data: %s \n", packet.data);
    cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE + 1, true);
    //TODO keep track of this vote!
}



static void button_handler(uint8_t pin_no, uint8_t button_action)
{
    if (button_action == APP_BUTTON_PUSH)
    {
        nrf_gpio_pin_toggle(LED_4);
        vote();
    }
}

static app_button_cfg_t p_button[] = {
    {BUTTON_3, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler}
};

    VotingModule::VotingModule(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
: Module(moduleId, node, cm, name, storageSlot)
{
    //Register callbacks n' stuff
    Logger::getInstance().enableTag("VOTING");

    //Save configuration to base class variables
    //sizeof configuration must be a multiple of 4 bytes
    configurationPointer = &configuration;
    configurationLength = sizeof(VotingModuleConfiguration);

    lastConnectionReportingTimer = 0;
    lastStatusReportingTimer = 0;

    //Start module configuration loading
    LoadModuleConfiguration();
}

void VotingModule::ConfigurationLoadedHandler()
{
    //Does basic testing on the loaded configuration
    Module::ConfigurationLoadedHandler();

    //Version migration can be added here
    if(configuration.moduleVersion == 1){/* ... */};

    //Do additional initialization upon loading the config


    //Start the Module...

}

void VotingModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
    //Every reporting interval, the node should send its connections
    if(configuration.connectionReportingIntervalMs != 0 && node->appTimerMs - lastConnectionReportingTimer > configuration.connectionReportingIntervalMs)
  {
        logt("VOTING", "In Timer Event Handler\n");
        //Do stuff on timer...
        if(!acknowledged) {
            logt("VOTING", "Am not acknowledged\n");
            vote();
        }

        lastConnectionReportingTimer = node->appTimerMs;
    }
}

void VotingModule::ResetToDefaultConfiguration()
{
    //Set default configuration values
    configuration.moduleId = moduleId;
    configuration.moduleActive = true;
    configuration.moduleVersion = 1;

    lastConnectionReportingTimer = 0;
    lastStatusReportingTimer = 0;

    configuration.statusReportingIntervalMs = 0;
    configuration.connectionReportingIntervalMs = 30 * 1000;
    //Set additional config values...
}

bool VotingModule::TerminalCommandHandler(string commandName, vector<string> commandArgs)
{
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


void VotingModule::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength)
{
    //Must call superclass for handling
    Module::ConnectionPacketReceivedEventHandler(inPacket, connection, packetHeader, dataLength);

    if(initialized == false) {
        // init leds
        nrf_gpio_cfg_output(LED_4);
        nrf_gpio_pin_set(LED_4);

        // init gpiote and button
        uint8_t size = sizeof(p_button)/sizeof(p_button[0]);
        APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, NULL);
        APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);

        uint32_t error_code = app_button_init(p_button, size, BUTTON_DEBOUNCE_DELAY);
        APP_ERROR_CHECK(error_code);

        error_code = app_button_enable();
        APP_ERROR_CHECK(error_code);

        logt("VOTING", "initialized button pin\n");
        initialized = true;
    }

    // TODO when i receive an acknowledgement, check off vote from queue
    // Voter Receives acknowledgement from Gateway
    if(packetHeader->messageType == MESSAGE_TYPE_MODULE_ACTION_RESPONSE){
        connPacketModule* packet = (connPacketModule*)packetHeader;
        // if it is meant for me...
        if(packet->moduleId == moduleId)
        {
            if(packet->actionType ==VotingModuleActionResponseMessages::RESPONSE_MESSAGE)
            {
                logt("VOTING", "Voter received acknowledgement from Gateway.\n");
		acknowledged=true;
            }
        }
    }

    //if this is gateway device and message type is correct
    if(node->isGatewayDevice) {

        if(packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
        connPacketModule* packet = (connPacketModule*)packetHeader;

        if(packet->moduleId == moduleId){
            if(packet->actionType == VotingModuleTriggerActionMessages::TRIGGER_MESSAGE){
                logt("VOTING", "Gateway %d received voter message from %d with data: %s\n", node->persistentConfig.nodeId, packetHeader->sender, packet->data);

                //Send Response acknowledgement
                connPacketModule outPacket;
                outPacket.header.messageType = MESSAGE_TYPE_MODULE_ACTION_RESPONSE;
                outPacket.header.sender = node->persistentConfig.nodeId;
                outPacket.header.receiver = packetHeader->sender;

                outPacket.moduleId = moduleId;
                outPacket.actionType = VotingModuleActionResponseMessages::RESPONSE_MESSAGE;

                cm->SendMessageToReceiver(NULL, (u8*)&outPacket, SIZEOF_CONN_PACKET_MODULE + 2, true);
                logt("VOTING", "Gateway sent acknowledgement of vote to %d \n", packetHeader->sender);
                }
            }
        }
    }
    else {
            logt("VOTER", "I am not a gateway! :D\n", packetHeader->sender);
            logt("VOTER", "I'm passing on a vote from node %d \n", packetHeader->sender);
    }
}
