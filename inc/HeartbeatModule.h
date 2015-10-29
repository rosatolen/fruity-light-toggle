#pragma once

#include <Module.h>

class HeartbeatModule : public Module
{
  public:
		HeartbeatModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot);

		void TimerEventHandler(u16 passedTime, u32 appTimer);
		void ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength);
};
