#include <VotingModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Storage.h>
#include <Node.h>
#include <LedWrapper.h>
#include <NFC.h>
#include "pca10028.h"

extern "C"{
#include <stdlib.h>
#include "nrf_gpio.h"
#include "app_error.h"
#include "nrf_drv_config.h"
#include "app_util_platform.h"
#include "nrf_delay.h"
#include "pca10028.h"
#include "pn532.h"
#include <app_timer.h>
}

int voteIndex = 1;


static void vote(unsigned short uID) {
    ConnectionManager *cm = ConnectionManager::getInstance();
    Node *node = Node::getInstance();
    Conf *config = Conf::getInstance();
    u32 time = (node->globalTime) / APP_TIMER_CLOCK_FREQ;

    nodeID everyone = 0;
    connPacketModule packet;
    packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
    packet.header.sender = node->persistentConfig.nodeId;
    packet.header.receiver = everyone;
    packet.moduleId = moduleID::VOTING_MODULE_ID;
    packet.actionType = 0; // hardcoded from the reference VotingModule.h

    packet.data[0] = uID & 0xff;
    packet.data[1] = (uID >> 8) & 0xff;
    packet.data[2] = (int)(time >> 24) & 0xff;
    packet.data[3] = (int)(time >> 16) & 0xff;
    packet.data[4] = (int)(time >> 8) & 0xff;
    packet.data[5] = (int)time & 0xff;

    bool success = node->PutInRetryStorage(uID);
    if(success) {
        cm->SendMessageToReceiver(NULL, (u8 * ) &packet, SIZEOF_CONN_PACKET_MODULE + 10, true);
        logt("VOTING", "Sending vote with id: %d and time: %d\n", uID, time);

    } else {
        logt("VOTING", "Queue full, unable to send vote with id: %d\n", uID);
    }
}

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

void VotingModule::TimerEventHandler(u16 passedTime, u32 appTimer) {
    // QA CODE: Enable if you are testing the stability of your build locally. Will try to vote  every second.
    //if (!node->isGatewayDevice && (appTimer / 1000 % 5 && appTimer % 2000 == 0)) {
    //    vote(voteIndex);
    //    voteIndex++;
    //}

    // if 10 seconds have passed, trigger retry of votes
    if (!node->isGatewayDevice && (appTimer / 1000) % 30 == 0 && (appTimer / 100) % 100 == 0) {
        logt("VOTING", "Triggering retry of votes");
        for (int i = 0; i < MAX_RETRY_STORAGE_SIZE; i++) {
            if (node->GetVoteFromRetryStorage(i) != 0) {
                vote(node->GetVoteFromRetryStorage(i));
            }
        }
    }

}

void VotingModule::ResetToDefaultConfiguration() {
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
    if(commandArgs.size() >= 2 && commandArgs[1] == moduleName) {
        if(commandName == "action") {
            nodeID destinationNode = (commandArgs[0] == "this") ? node->persistentConfig.nodeId : atoi(commandArgs[0].c_str());
            if(commandArgs.size() == 4 && commandArgs[2] == "set_time") {
                u64 timeStamp = (atoi(commandArgs[3].c_str()) * (u64)APP_TIMER_CLOCK_FREQ);

                node->globalTime = timeStamp;
                app_timer_cnt_get(&node->globalTimeSetAt);

                connPacketUpdateTimestamp packet;

                packet.header.messageType = MESSAGE_TYPE_UPDATE_TIMESTAMP;
                packet.header.sender = node->persistentConfig.nodeId;
                packet.header.receiver = 0;

                cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_UPDATE_TIMESTAMP, true);

                return true;
            }
        }
    }

    //Must be called to allow the module to get and set the config
    return Module::TerminalCommandHandler(commandName, commandArgs);
}


void VotingModule::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength)
{
    //Must call superclass for handling
    Module::ConnectionPacketReceivedEventHandler(inPacket, connection, packetHeader, dataLength);

    if(packetHeader->messageType == MESSAGE_TYPE_MODULE_ACTION_RESPONSE){
        connPacketModule* packet = (connPacketModule*)packetHeader;
        if(packet->moduleId == moduleId)
        {
            if(packet->actionType == VotingModuleActionResponseMessages::RESPONSE_MESSAGE)
            {
                //logt("VOTING", "Voter received acknowledgement from Gateway with userId %u \n", packet->data[0]);
                unsigned short userId = (( (short)packet->data[1] ) << 8) | packet->data[0];
                node->RemoveFromRetryStorage(userId);
            }
        }
    }

    if(node->isGatewayDevice) {
        if(packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
            connPacketModule* packet = (connPacketModule*)packetHeader;
            if(packet->moduleId == moduleId){
                if(packet->actionType == VotingModuleTriggerActionMessages::TRIGGER_MESSAGE){
                    unsigned short uID = (( (unsigned short)packet->data[1] ) << 8) | packet->data[0];

                    u32 timeSent =  (
                                    + (packet->data[2] << 24)
                                    + (packet->data[3] << 16)
                                    + (packet->data[4] << 8 )
                                    + (packet->data[5] ));

                    logt("VOTING", "Gateway %d received voter message from %d with userId %d and time %d\n", node->persistentConfig.nodeId, packetHeader->sender, uID, timeSent);

                    //Send Response acknowledgement
                    connPacketModule outPacket;
                    outPacket.header.messageType = MESSAGE_TYPE_MODULE_ACTION_RESPONSE;
                    outPacket.header.sender = node->persistentConfig.nodeId;
                    outPacket.header.receiver = packetHeader->sender;

                    outPacket.moduleId = moduleId;
                    outPacket.actionType = VotingModuleActionResponseMessages::RESPONSE_MESSAGE;
                    outPacket.data[0] = packet->data[0];
                    outPacket.data[1] = packet->data[1];
                    cm->SendMessageToReceiver(NULL, (u8*)&outPacket, SIZEOF_CONN_PACKET_MODULE + 3 + 1, true);
                    logt("VOTING", "Gateway sent acknowledgement of vote to %d with userId: %d \n", packetHeader->sender, uID);
                }
            }
        }
    }
}
