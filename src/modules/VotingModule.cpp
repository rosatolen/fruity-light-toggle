#include <VotingModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Storage.h>
#include <Node.h>
#include "pca10028.h"


extern "C"{
#include <stdlib.h>
}

#define APP_TIMER_PRESCALER     0
#define APP_TIMER_MAX_TIMERS    1
#define APP_TIMER_OP_QUEUE_SIZE 2
#define BUTTON_DEBOUNCE_DELAY   50
#define APP_GPIOTE_MAX_USERS    1

static void vote(unsigned short uID) {
	ConnectionManager *cm = ConnectionManager::getInstance();
	Logger::getInstance().enableTag("VOTING");
	Node *node = Node::getInstance();
	Conf *config = Conf::getInstance();

	nodeID everyone = 0;
	connPacketModule packet;
	packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
	packet.header.sender = node->persistentConfig.nodeId;
	packet.header.receiver = everyone;
	packet.moduleId = moduleID::VOTING_MODULE_ID;
	packet.actionType = 0; // hardcoded from the reference VotingModule.h

	packet.data[0] = uID & 0xff;
	packet.data[1] = (uID >> 8) & 0xff;
	cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE + 3 + 1, true);
	logt("VOTING", "Sending vote with id: %d\n", uID);
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

void VotingModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
	if (!node->isGatewayDevice) {
		// if 10 seconds have passed
		if ((appTimer / 1000) % 10 == 0 && (appTimer / 100) % 100 == 0) {
			unsigned short failed[] = { 3566, 1234, 4321 };
			// using sizeof(failed) is buggy
			for (int i=0; i < 3; i++){
				vote(failed[i]);
			}
		}
		if ((appTimer / 1000) % 5 == 0) {
			//Send a Heartbeat
			connPacketModule packet;
			packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
			packet.header.sender = node->persistentConfig.nodeId;
			packet.header.receiver = 0;
			packet.moduleId = moduleID::VOTING_MODULE_ID;
			packet.actionType = 0; // hardcoded from the reference VotingModule.h
			packet.data[0] = 5;
			cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE + 1 + 1, true);
		}
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

	// TODO when i receive an acknowledgement, check off vote from queue
	// Voter Receives acknowledgement from Gateway
	if(packetHeader->messageType == MESSAGE_TYPE_MODULE_ACTION_RESPONSE){
		connPacketModule* packet = (connPacketModule*)packetHeader;
		// if it is meant for me...
		if(packet->moduleId == moduleId)
		{
			if(packet->actionType ==VotingModuleActionResponseMessages::RESPONSE_MESSAGE)
			{
				//logt("VOTING", "Voter received acknowledgement from Gateway with userId %u \n", packet->data[0]);
				if(packet->data[0] != 5) {
					unsigned short userId = (( (short)packet->data[1] ) << 8) | packet->data[0];
					logt("VOTING", "Voter received acknowledgement from Gateway with userId: %d \n", userId);
				}
				//TODO make voting stop
			}
		}
	}

	if(node->isGatewayDevice) {
		if(packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
			connPacketModule* packet = (connPacketModule*)packetHeader;

			if(packet->moduleId == moduleId){
				if (packet->data[0] == 5) {
					//logt("VOTING", "HEARTBEAT RECEIVED from nodeId:%d\n", packetHeader->sender);
				} else {
					if(packet->actionType == VotingModuleTriggerActionMessages::TRIGGER_MESSAGE){
						unsigned short uID = (( (short)packet->data[1] ) << 8) | packet->data[0];
						logt("VOTING", "Gateway %d received voter message from %d with userId %d \n", node->persistentConfig.nodeId, packetHeader->sender, uID);

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
}
