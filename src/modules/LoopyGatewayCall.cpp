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

#include <LoopyGatewayCall.h>
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

static void button_handler(uint8_t pin_no, uint8_t button_action)
{
    if (button_action == APP_BUTTON_PUSH)
    {
        nrf_gpio_pin_toggle(LED_4);

        ConnectionManager *cm = ConnectionManager::getInstance();
        Logger::getInstance().enableTag("VOTER");
        Node *node = Node::getInstance();
        Conf *config = Conf::getInstance();

        nodeID everyone = 0; // send message to myself
        logt("VOTER", "Trying to vote.\n");
        connPacketModuleAction packet;
        packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
        packet.header.sender = node->persistentConfig.nodeId;
        packet.header.receiver = everyone;
        packet.moduleId = moduleID::LOOPY_MESSAGES_ID;
        packet.actionType = 0; // hardcoded from the reference LoopyGatewayCall.h
        strncpy((char *) packet.data, "hello", 6);
        logt("VOTER", "Sending message data: %s \n", packet.data);
        cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE_ACTION + 1, true);
    }
}

static app_button_cfg_t p_button[] = {
    {BUTTON_3, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler}
};

LoopyGatewayCall::LoopyGatewayCall(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
: Module(moduleId, node, cm, name, storageSlot)
{
    //Register callbacks n' stuff
    Logger::getInstance().enableTag("VOTER");

    //Save configuration to base class variables
    //sizeof configuration must be a multiple of 4 bytes
    configurationPointer = &configuration;
    configurationLength = sizeof(LoopyGatewayCallConfiguration);

    //Start module configuration loading
    LoadModuleConfiguration();
}

void LoopyGatewayCall::ConfigurationLoadedHandler()
{
    //Does basic testing on the loaded configuration
    Module::ConfigurationLoadedHandler();

    //Version migration can be added here
    if(configuration.moduleVersion == 1){/* ... */};

    //Do additional initialization upon loading the config


    //Start the Module...

}

void LoopyGatewayCall::TimerEventHandler(u16 passedTime, u32 appTimer)
{
    //Do stuff on timer...

}

void LoopyGatewayCall::ResetToDefaultConfiguration()
{
    //Set default configuration values
    configuration.moduleId = moduleId;
    configuration.moduleActive = false;
    configuration.moduleVersion = 1;

    //Set additional config values...
}

bool LoopyGatewayCall::TerminalCommandHandler(string commandName, vector<string> commandArgs)
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


void LoopyGatewayCall::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength)
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

        logt("VOTER", "initialized button pin\n");
        initialized = true;
    }


    //if this is gateway device and message type is correct
    if(node->isGatewayDevice && packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
        connPacketModuleAction* packet = (connPacketModuleAction*)packetHeader;

        //Check if our module is meant and we should trigger an action
        if(packet->moduleId == moduleId){
            if(packet->actionType == LoopyGatewayCallTriggerActionMessages::TRIGGER_MESSAGE){
                logt("VOTER", "Voter message received with data: %s\n", packet->data);

                //Send Response acknowledgement
                connPacketModuleAction outPacket;
                outPacket.header.messageType = MESSAGE_TYPE_MODULE_ACTION_RESPONSE;
                outPacket.header.sender = node->persistentConfig.nodeId;
                outPacket.header.receiver = packetHeader->sender;

                outPacket.moduleId = moduleId;
                outPacket.actionType = LoopyGatewayCallActionResponseMessages::RESPONSE_MESSAGE;
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
            if(packet->actionType ==LoopyGatewayCallActionResponseMessages::RESPONSE_MESSAGE)
            {
                 logt("LOOPY", "Loopy message came back from %u with data %d, %d\n", packet->header.sender, packet->data[0], packet->data[1]);
            }
        }
    }
}

