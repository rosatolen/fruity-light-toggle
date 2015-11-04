#include <VotingModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Storage.h>
#include <Node.h>
#include "pca10028.h"

extern "C"{
#include <stdlib.h>
#include "nrf_gpio.h"
#include "nrf_drv_twi.h"
#include "app_error.h"
#include "nrf_drv_config.h"
#include "app_util_platform.h"
#include "nrf_delay.h"
#include "pn532.h"
}

#define APP_TIMER_PRESCALER     0
#define APP_TIMER_MAX_TIMERS    1
#define APP_TIMER_OP_QUEUE_SIZE 2
#define BUTTON_DEBOUNCE_DELAY   50
#define APP_GPIOTE_MAX_USERS    1

#define MAX_RETRY_STORAGE_SIZE 30

unsigned short empty = 0;
unsigned short retryStorage[MAX_RETRY_STORAGE_SIZE] = {0};
int currentMinute = 0;
bool UART_CONFIGURED = false;

void removeFromRetryStorage(unsigned short userId) {
	unsigned short tempStorage[MAX_RETRY_STORAGE_SIZE] = {empty,empty,empty,empty,empty};
	for (int i=0, j=0; i < MAX_RETRY_STORAGE_SIZE; i++) {
		if (retryStorage[i] != userId) {
			tempStorage[j] = retryStorage[i];
			j++;
		}
	}
	for (int i=0; i < MAX_RETRY_STORAGE_SIZE; i++) {
		if(tempStorage[i] != empty){
			retryStorage[i] = tempStorage[i];
		} else {
			retryStorage[i] = empty;
		}
	}
}

void putInRetryStorage(unsigned short userId) {
	int index = 0;
	unsigned short temp[MAX_RETRY_STORAGE_SIZE] = { empty,empty,empty,empty,empty };

	for (int i=0; i < MAX_RETRY_STORAGE_SIZE; i++) {
		if(retryStorage[i] == userId) {
			break;
		}
		if (retryStorage[i] == empty) {
			retryStorage[i] = userId;
			break;
		}
		if(i == MAX_RETRY_STORAGE_SIZE-1) {
			removeFromRetryStorage(retryStorage[0]);
			retryStorage[i] = userId;
		}
	}
}

static void vote(unsigned short uID) {
	ConnectionManager *cm = ConnectionManager::getInstance();
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
	putInRetryStorage(uID);
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

void VotingModule::TimerEventHandler(u16 passedTime, u32 appTimer) {
#ifdef ENABLE_NFC
	if (!UART_CONFIGURED) {
		uart_115200_config(RTS_PIN_NUMBER, /*TX_PIN_NUM*/ 19, CTS_PIN_NUMBER, /*RX_PIN_NUM*/ 20);
        UART_CONFIGURED = true;
	}

	if (!node->isGatewayDevice) {
		// Check tag exists every second
		if (appTimer/1000 % 5 && appTimer % 1000 == 0) {
			wakeup();
			unsigned short userId = in_list_passive_target();
			if (userId != 0) vote(userId);
	    }

        // if 10 seconds have passed, trigger retries
        if ((appTimer / 1000) % 30 == 0 && (appTimer / 100) % 100 == 0) {
            for (int i=0; i < MAX_RETRY_STORAGE_SIZE; i++){
                if (retryStorage[i] != empty) {
                    vote(retryStorage[i]);
                }
            }
        }
	}
#endif
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
		if(packet->moduleId == moduleId)
		{
			if(packet->actionType == VotingModuleActionResponseMessages::RESPONSE_MESSAGE)
			{
				//logt("VOTING", "Voter received acknowledgement from Gateway with userId %u \n", packet->data[0]);
				unsigned short userId = (( (short)packet->data[1] ) << 8) | packet->data[0];
				removeFromRetryStorage(userId);
			}
		}
	}

	if(node->isGatewayDevice) {
		if(packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
			connPacketModule* packet = (connPacketModule*)packetHeader;
			if(packet->moduleId == moduleId){
				if(packet->actionType == VotingModuleTriggerActionMessages::TRIGGER_MESSAGE){
					unsigned short uID = (( (unsigned short)packet->data[1] ) << 8) | packet->data[0];
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
