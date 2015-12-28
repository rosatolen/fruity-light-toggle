#include <LightToggleModule.h>
#include <Logger.h>
#include <Node.h>

extern "C" {
#include "app_button.h"
#include "app_timer.h"
#include "pca10028.h"
}

#define APP_TIMER_PRESCALAR         0
#define BUTTON_DETECTION_DELAY      APP_TIMER_TICKS(50u, APP_TIMER_PRESCALAR)
#define LIGHT_CONNECTED_NODE        10731

void send_light_toggle_packet(uint32_t to_node, uint8_t dataValue) {
    connPacketModule packet;
    Node* node = Node::getInstance();
    packet.header.sender = node->persistentConfig.nodeId;
    packet.header.receiver = to_node;
    packet.header.messageType = MESSAGE_TYPE_LIGHT_TOGGLE;
    packet.data[0] = dataValue;

    ConnectionManager *cm = ConnectionManager::getInstance();
    cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE + 1 + 1, true);
}

static void button_handler(uint8_t pin_no, uint8_t button_action) {
    // BUTTON_1 is connected to PIN 17 on the PCA10028 Board.
    if (button_action == APP_BUTTON_PUSH && pin_no == BUTTON_1) send_light_toggle_packet(LIGHT_CONNECTED_NODE, 255);
}

void LightToggleModule::ButtonInit() {
    static app_button_cfg_t buttons[] = {
        {BUTTON_1, false, BUTTON_PULL, button_handler}
    };

    uint32_t err_code = app_button_init(buttons,
                                        sizeof(buttons) / sizeof(buttons[0]),
                                        BUTTON_DETECTION_DELAY);

    APP_ERROR_CHECK(err_code);
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}

LightToggleModule::LightToggleModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
    : Module(moduleID::LIGHT_TOGGLE_MODULE_ID, node, cm, name, storageSlot) {

    Logger::getInstance().enableTag("LIGHT_TOGGLE");

    _configuration.moduleId = moduleID::LIGHT_TOGGLE_MODULE_ID;
    _configuration.moduleVersion = 1;
    _configuration.moduleActive = true;
    configurationPointer = &_configuration;
}

void LightToggleModule::TimerEventHandler(u16 passedTime, u32 appTimer) {
    return;

    // QA Testing: Enable if checking for queue robustness
    // Send a packet every second
    //if ((appTimer / 1000) % 5 && appTimer % 2000 == 0) {
    //    send_light_toggle_packet(NODE_ID_BROADCAST);
    //}
}

void LightToggleModule::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength) {
    Module::ConnectionPacketReceivedEventHandler(inPacket, connection, packetHeader, dataLength);

    //logt("LIGHT_TOGGLE", "inPacket");
    logt("LIGHT_TOGGLE", "connection->connectionHandle %d", connection->connectionHandle);
    logt("LIGHT_TOGGLE", "connection->writeCharacteristicHandle %d", connection->writeCharacteristicHandle);
    // get gap addr t from connection logt("LIGHT_TOGGLE", "connection->writeCharacteristicHandle %d", connection->writeCharacteristicHandle);

    logt("LIGHT_TOGGLE", "packetHeader->messageType %d", packetHeader->messageType);
    logt("LIGHT_TOGGLE", "packetHeader->sender %d", packetHeader->sender);
    logt("LIGHT_TOGGLE", "packetHeader->receiver %d", packetHeader->receiver);
    logt("LIGHT_TOGGLE", "packetHeader->remoteReceiver %d", packetHeader->remoteReceiver);

    logt("LIGHT_TOGGLE", "dataLength %d", dataLength);

    if (packetHeader->messageType != MESSAGE_TYPE_LIGHT_TOGGLE) return;
    if (packetHeader->sender == node->persistentConfig.nodeId) return;
    if (packetHeader->receiver != node->persistentConfig.nodeId) return;

    u8* data = inPacket->data;
    unsigned char dataValue = data[dataLength - 1];

    connPacketModule* packet = (connPacketModule*)packetHeader;

    if (dataValue == 255)
    {
        node->LedRed->On();
        node->LedBlue->On();
        node->LedGreen->On();
        //node->Relay->On();
        logt("LIGHT_TOGGLE", "LIGHT_TOGGLE RECEIVED from nodeId:%d with message: ON", packetHeader->sender);
    }

    if (dataValue == 0)
    {
        node->LedRed->Off();
        node->LedBlue->Off();
        node->LedGreen->Off();
        //node->Relay->Off();
        logt("LIGHT_TOGGLE", "LIGHT_TOGGLE RECEIVED from nodeId:%d with message: OFF", packetHeader->sender);
    }
}
