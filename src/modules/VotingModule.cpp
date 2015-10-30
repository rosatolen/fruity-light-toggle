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
}

#define APP_TIMER_PRESCALER     0
#define APP_TIMER_MAX_TIMERS    1
#define APP_TIMER_OP_QUEUE_SIZE 2
#define BUTTON_DEBOUNCE_DELAY   50
#define APP_GPIOTE_MAX_USERS    1

#define MAX_RETRY_STORAGE_SIZE 10

bool INITIALIZED_QUEUE = false;

unsigned short empty = 0;
unsigned short retryStorage[MAX_RETRY_STORAGE_SIZE] = {0,0,0,0,0,0,0,0,0,0};
int currentMinute = 0;

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

static void twi_event_handler(nrf_drv_twi_evt_t *evt) {
	// light up led 1 if transmit succeeded
	//if(evt->type == NRF_DRV_TWI_TX_DONE) nrf_gpio_pin_clear(LED_1);
	// light up led 2 if receive succeeded
	//if(evt->type == NRF_DRV_TWI_RX_DONE) nrf_gpio_pin_clear(LED_2);
}

void VotingModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
	if (!INITIALIZED_QUEUE) {
		srand(12345678);
		INITIALIZED_QUEUE=true;

		uint8_t tx_data[] = {0x0,0x0,0xFF,0x02,0xFE,0xD4,0x02,0x2A,0x00};  // I believe this is the i2c command sequence for firmware version
		uint8_t rx_data[32];

		ret_code_t ret_code;

		const nrf_drv_twi_t           p_twi_instance = NRF_DRV_TWI_INSTANCE(1); // Set up TWI instance 1 with default values

		//    nrf_drv_twi_config_t    p_twi_config = NRF_DRV_TWI_DEFAULT_CONFIG(1);// Set up TWI configuration default values (this is SDA Pin 1, SCL Pin 0)

		nrf_drv_twi_config_t    p_twi_config;
		p_twi_config.scl = 19;
		p_twi_config.sda = 20;
		p_twi_config.frequency = NRF_TWI_FREQ_400K;
		p_twi_config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

		ret_code = nrf_drv_twi_init(&p_twi_instance, &p_twi_config, twi_event_handler); // Initiate twi driver with instance and configuration values
		APP_ERROR_CHECK(ret_code); // Check for errors in return value

		nrf_drv_twi_enable(&p_twi_instance); // Enable the TWI instance

		// transmit firmware version command 0x24 is the address for my PN532
		ret_code = nrf_drv_twi_tx(&p_twi_instance, 0x24, tx_data, sizeof(tx_data), false);
		APP_ERROR_CHECK(ret_code); // Check for errors in return value

		nrf_delay_ms(5);

		// receive firmware version results handler should have the data
		ret_code = nrf_drv_twi_rx(&p_twi_instance, 0x24, rx_data, 12, false);
		APP_ERROR_CHECK(ret_code);
		nrf_delay_ms(5);

		ret_code = nrf_drv_twi_rx(&p_twi_instance, 0x24, rx_data, 12, false);
		APP_ERROR_CHECK(ret_code);
	}

		if (!node->isGatewayDevice) {
		// to use a new minute rate, start counting from 0
		// so if you want to do something every 5th minute, your minute rate is 4
		int minuteRate = 2;
		int minuteRatePlusOne = 3;
		currentMinute = (appTimer/60000 % 1000) % minuteRatePlusOne;
		if (currentMinute == minuteRate && (appTimer/1000 % 5 && appTimer % 10 == 0)) {
			vote((short)(rand() % 10000));
		}
		
		// if 10 seconds have passed, trigger retries
		if ((appTimer / 1000) % 10 == 0 && (appTimer / 100) % 100 == 0) {
			for (int i=0; i < MAX_RETRY_STORAGE_SIZE; i++){
				if (retryStorage[i] != empty) {
					vote(retryStorage[i]);	
				}
			}
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
