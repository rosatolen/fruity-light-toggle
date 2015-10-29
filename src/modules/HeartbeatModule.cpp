#include <HeartbeatModule.h>

#include <Logger.h>
#include <Node.h>

typedef struct
{
  nodeID partnerId;
  // i8 rssi; TODO
}connection;

typedef struct
{
	connPacketHeader header;
  connection inConn;
  connection outConn[3];
}connPacketHeartbeat;

HeartbeatModule::HeartbeatModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
	: Module(moduleID::HEARTBEAT_MODULE_ID, node, cm, name, storageSlot)
{
	Logger::getInstance().enableTag("HEARTBEAT");

  _configuration.moduleId = moduleID::HEARTBEAT_MODULE_ID;
  _configuration.moduleVersion = 1;
  _configuration.moduleActive = true;
  configurationPointer = &_configuration;
}

void fillConnectionStruct(Connection *conn, connection &connStruct){
  if( conn && conn->handshakeDone ){
    connStruct.partnerId = conn->partnerId;
  }else {
    connStruct.partnerId = 0;
  }
}

void HeartbeatModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
    // fire every 5 seconds. TODO: make this not confusing and flakey
		if ((appTimer / 1000) % 5 == 0 && appTimer % 10 == 0) {
			connPacketHeartbeat packet;

			packet.header.messageType = MESSAGE_TYPE_HEARTBEAT;
			packet.header.sender = node->persistentConfig.nodeId;
			packet.header.receiver = NODE_ID_BROADCAST;

      fillConnectionStruct(cm->inConnection,packet.inConn);
      fillConnectionStruct(cm->outConnections[0],packet.outConn[0]);
      fillConnectionStruct(cm->outConnections[1],packet.outConn[1]);
      fillConnectionStruct(cm->outConnections[2],packet.outConn[2]);

			cm->SendMessageToReceiver(NULL, (u8*)&packet, sizeof(connPacketHeartbeat), true);
		}
}

void HeartbeatModule::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength)
{
	Module::ConnectionPacketReceivedEventHandler(inPacket, connection, packetHeader, dataLength);

	if(packetHeader->messageType != MESSAGE_TYPE_HEARTBEAT)
    return;

  connPacketHeartbeat* packet = (connPacketHeartbeat*)packetHeader;

  logt("HEARTBEAT", "HEARTBEAT RECEIVED from nodeId:%d inConn:%d outConns:[%d,%d,%d]\n", 
      packetHeader->sender,
      packet->inConn.partnerId,
      packet->outConn[0].partnerId,
      packet->outConn[1].partnerId,
      packet->outConn[2].partnerId);
}
