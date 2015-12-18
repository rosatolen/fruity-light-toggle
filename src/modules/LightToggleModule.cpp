#include <LightToggleModule.h>
#include <Logger.h>
#include <Node.h>

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

    // Send a packet every second
    if ((appTimer / 1000) % 5 && appTimer % 2000 == 0) {
        connPacketModule packet;

        packet.header.messageType = MESSAGE_TYPE_LIGHT_TOGGLE;
        packet.header.sender = node->persistentConfig.nodeId;
        packet.header.receiver = NODE_ID_BROADCAST;

        // ON data type
        packet.data[0] = 0xff;

        cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE + 2 + 1, true);
    }
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

    connPacketModule* packet = (connPacketModule*)packetHeader;
    node->Relay->On();

    logt("LIGHT_TOGGLE", "LIGHT_TOGGLE RECEIVED from nodeId:%d with message: ON",
            packetHeader->sender);

    //node->LedRed->On();
    //node->LedBlue->On();
    //node->LedGreen->On();
}
