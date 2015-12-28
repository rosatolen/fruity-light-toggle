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

#include <Node.h>
#include <Connection.h>
#include <GATTController.h>
#include <GAPController.h>
#include <ConnectionManager.h>
#include <Logger.h>

extern "C"{
}


//The Connection Class does have methods like Connect,... but connections, service
//discovery or encryption are handeled by the Connectionmanager so that we can control
//The parallel flow of multiple connections


Connection::Connection(u8 id, ConnectionManager* cm, Node* node, ConnectionDirection direction)
{
	this->cm = cm;
	this->connectionId = id;
	this->node = node;
	this->direction = direction;
	this->packetSendQueue = new PacketQueue(packetSendBuffer, PACKET_SEND_BUFFER_SIZE);
	connectedClusterSize = 0;
	packetReassemblyPosition = 0;
	packetSendPosition = 0;

	Init();
}

void Connection::Init(){
	connectedClusterId = 0;
	unreliableBuffersFree = cm->txBuffersPerLink; //FIXME: request from softdevice
	reliableBuffersFree = 1;
	partnerId = 0;
	connectionHandle = BLE_CONN_HANDLE_INVALID;

	isConnected = false;
	handshakeDone = false;
	handshakeStarted = 0;
	writeCharacteristicHandle = 0;
	packetReassemblyPosition = 0;
	packetSendPosition = 0;

	rssiSamplesNum = 0;
	rssiSamplesSum = 0;
	rssiAverage = 0;

	hopsToSink = -1;

	this->packetSendQueue->Clean();
}

//Once a connection has been connected in the connection manager, these parameters
//Are passed to the connect function
bool Connection::Connect(ble_gap_addr_t* address, u16 writeCharacteristicHandle)
{
	if(isConnected) return false;

	Init();

	memcpy(&partnerAddress, address, sizeof(ble_gap_addr_t));
	this->writeCharacteristicHandle = writeCharacteristicHandle;

	return true;
}


/*######## PUBLIC FUNCTIONS ###################################*/

void Connection::DiscoverCharacteristicHandles(void)
{
	GATTController::bleDiscoverHandles(connectionHandle);
}

void Connection::Disconnect(void)
{
	GAPController::disconnectFromPeripheral(connectionHandle);
}


/*######## PRIVATE FUNCTIONS ###################################*/

/*######## HANDLER ###################################*/
void Connection::ConnectionSuccessfulHandler(ble_evt_t* bleEvent)
{
	this->handshakeStarted = node->appTimerMs;

	if (direction == CONNECTION_DIRECTION_IN)
		logt("CONN", "Incoming connection %d connected", connectionId);
	else
		logt("CONN", "Outgoing connection %d connected", connectionId);

	this->connectionHandle = bleEvent->evt.gap_evt.conn_handle;
	this->isConnected = true;

}


void Connection::StartHandshake(void)
{
	if (this->handshakeDone)
	{
		logt("HANDSHAKE", "Handshake for connId:%d is already finished", connectionId);
		return;
	}


	logt("HANDSHAKE", "############ Handshake starting ###############");

	//After the Handles have been discovered, we start the Handshake
	connPacketClusterWelcome packet;
	packet.header.messageType = MESSAGE_TYPE_CLUSTER_WELCOME;
	packet.header.sender = node->persistentConfig.nodeId;
	packet.header.receiver = NODE_ID_HOPS_BASE + 1; //Node id is unknown, but this allows us to send the packet only 1 hop

	packet.payload.clusterId = node->clusterId;
	packet.payload.clusterSize = node->clusterSize;
	packet.payload.meshWriteHandle = GATTController::getMeshWriteHandle();

	//Now we set the hop counter to the closest sink
	//If we are sink ourself, we set it to 1, otherwise we use our
	//shortest path to reach a sink and increment it by one.
	//If there is no known sink, we set it to 0.
	packet.payload.hopsToSink = cm->GetHopsToShortestSink(this);

	logt("HANDSHAKE", "OUT => conn(%d) CLUSTER_WELCOME, cID:%x, cSize:%d", connectionId, packet.payload.clusterId, packet.payload.clusterSize);

	cm->SendMessage(this, (u8*) &packet, SIZEOF_CONN_PACKET_CLUSTER_WELCOME, true);

}

void Connection::EncryptConnection(void){

	/*ble_opt_t bleOption;

	bleOption.gap_opt.passkey.p_passkey

	sd_ble_opt_set(BLE_GAP_OPT_PASSKEY)*/


}


#define __________________HANDLER__________________
void Connection::DisconnectionHandler(ble_evt_t* bleEvent)
{
	//Reason?
	u8 disconnectReason = bleEvent->evt.gap_evt.params.disconnected.reason;

	logt("DISCONNECT", "Disconnected %d from connId:%d, HCI:%d %s", partnerId, connectionId, disconnectReason, Logger::getHciErrorString(disconnectReason));

	//If for some reason BLE device disconnected make sure our inFeignHandShakeMode is set to false
	if ((partnerId == BLE_NODE_ID || (partnerId == BLE_NODE_ID + 1)) && disconnectReason == 19)
	{
	    node->inFeignHandshakeMode = false;
	    logt("TC", "Disconnected, Node Handshake set to off");
	}

	//Decrease Cluster size
	this->connectedClusterSize = 0;
	node->clusterSize -= this->connectedClusterSize;


	//Reset variables
	this->Init();
	logt("TC", "Current Connections are: %d, %d, %d", cm->connections[0]->partnerId, cm->connections[1]->partnerId, cm->connections[2]->partnerId);
}


void Connection::ReceivePacketHandler(connectionPacket* inPacket)
{
	u8* data = inPacket->data;
	u16 dataLength = inPacket->dataLength;
	bool reliable = inPacket->reliable;

	//If it is a packet from the handshake, we keep it, otherwise, we forwared it to the node
	connPacketHeader* packetHeader = (connPacketHeader*) data;


	//#################### Feign handshake for non-mesh device connecting over BLE ###############
	//Inspect current connections to this node
    logt("TC", "Current Connections are: %d, %d, %d", cm->connections[0]->partnerId, cm->connections[1]->partnerId, cm->connections[2]->partnerId);

	if (packetHeader->messageType == MESSAGE_TYPE_UART_WRITE)
	{
        for(int i=0; i<Config->meshMaxOutConnections; i++){

            //Sometimes the nodeID is offset by one. Don't know why
            if (cm->connections[i]->partnerId == BLE_NODE_ID || cm->connections[i]->partnerId == BLE_NODE_ID+1)
            {
                //Get target NodeID and Data Value
                unsigned short targetNodeId = (( (short)data[2] ) << 8) | data[1];
                unsigned char dataValue = data[3];

                logt("TC", "Node ID is: %d", targetNodeId);
                logt("TC", "Data Value is: %d", dataValue);

                //Fix the packet header
                inPacket->dataLength = SIZEOF_CONN_PACKET_HEADER + 1 + 1;
                packetHeader->messageType = MESSAGE_TYPE_LIGHT_TOGGLE;
                packetHeader->receiver = targetNodeId;
                packetHeader->sender = BLE_NODE_ID+1;
                data[inPacket->dataLength - 1] = dataValue;

                if (targetNodeId == node->persistentConfig.nodeId)
                {
                    //Send packet to our modules

                    for (int n=0; n<inPacket->dataLength; n++)
                    {
                        logt("TC", "Data Value at %d is: %d", n, inPacket->data[n]);
                    }

                    cm->connectionManagerCallback->messageReceivedCallback(inPacket);
                }

                else
                {
                    //Broadcast packet to everyone else if its not ours
                    cm->SendMessageOverConnections(this, data, inPacket->dataLength, reliable);
                }

                return;
            }
        }

		if (node->inFeignHandshakeMode == false)
		{
		    logt("TC", "Recognised as UART Write Message, first pass");
		    packetHeader->receiver = NODE_ID_BROADCAST;
		    packetHeader->sender = BLE_NODE_ID+1;
		    packetHeader->messageType = MESSAGE_TYPE_CLUSTER_WELCOME;
		    dataLength = SIZEOF_CONN_PACKET_CLUSTER_WELCOME;

		    node->inFeignHandshakeMode = true;
		}

		else
		{
            logt("TC", "Recognised as UART Write Message, second pass");

            packetHeader->receiver = node->persistentConfig.nodeId;
            packetHeader->sender = BLE_NODE_ID+1;
            packetHeader->messageType = MESSAGE_TYPE_CLUSTER_ACK_1;
            dataLength = SIZEOF_CONN_PACKET_CLUSTER_ACK_1;

            node->inFeignHandshakeMode = false;
		}
	}

	/*#################### ROUTING ############################*/

	//We are the last receiver for this packet
	if(
			packetHeader->receiver == node->persistentConfig.nodeId //We are the receiver
			|| packetHeader->receiver == NODE_ID_HOPS_BASE + 1 //The packet was meant to travel only one hop
			|| (packetHeader->receiver == NODE_ID_SHORTEST_SINK && node->persistentConfig.deviceType == deviceTypes::DEVICE_TYPE_SINK) //Packet was meant for the shortest sink and we are a sink
	){

	}
	//The packet should continue to the shortest sink
	else if(packetHeader->receiver == NODE_ID_SHORTEST_SINK)
	{
		Connection* connection = cm->GetConnectionToShortestSink(NULL);

		if(connection){
			cm->SendMessage(connection, data, dataLength, reliable);
		}
		//We could send it as a broadcast or we just drop it if we do not know any sink
		else {

		}
	}
	//This could be either a packet to a specific node, group, with some hops left or a broadcast packet
	else
	{
		//Some packets need to be modified before relaying them

		//We increment the hops to sink so that each node knows how many hops the sink is away
		if(packetHeader->messageType == MESSAGE_TYPE_CLUSTER_INFO_UPDATE){
			//Increment hops to sink
			connPacketClusterInfoUpdate* packet = (connPacketClusterInfoUpdate*)packetHeader;
			if(packet->payload.hopsToSink > -1) packet->payload.hopsToSink++;
		}

		//If the packet should travel a number of hops, we decrement that part
		if(packetHeader->sender > NODE_ID_HOPS_BASE && packetHeader->sender < NODE_ID_HOPS_BASE + 31000){
			packetHeader->sender--;
		}

		//Send to all other connections
		cm->SendMessageOverConnections(this, data, dataLength, reliable);
	}




	/*#################### HANDSHAKE ############################*/

	/******* Cluster welcome *******/
    if (packetHeader->messageType == MESSAGE_TYPE_CLUSTER_WELCOME)
	{
		if (dataLength == SIZEOF_CONN_PACKET_CLUSTER_WELCOME)
		{
			//Now, compare that packet with our data and see if he should join our cluster
			connPacketClusterWelcome* packet = (connPacketClusterWelcome*) data;
			//FIXME: My own cluster size might have changed since I sent my packet, that means,
			//Thatthe other node might decide on different data than I do, which might mean
			//That both think that they are the bigger cluster

			//Save mesh write handle
			writeCharacteristicHandle = packet->payload.meshWriteHandle;


			logt("HANDSHAKE", "############ Handshake starting ###############");


			logt("HANDSHAKE", "IN <= %d CLUSTER_WELCOME clustID:%x, clustSize:%d, toSink:%d", packet->header.sender, packet->payload.clusterId, packet->payload.clusterSize, packet->payload.hopsToSink);

			//PART 1: We do have the same cluster ID. Ouuups, should not have happened, run Forest!
			if (packet->payload.clusterId == node->clusterId)
			{
				logt("HANDSHAKE", "CONN %d disconnected because it had the same clusterID before handshake", connectionId);
				this->Disconnect();
			}
			//PART 2: This is more probable, he's in a different cluster
			else if (packet->payload.clusterSize < node->clusterSize || (packet->payload.clusterSize == node->clusterSize && packet->payload.clusterId < node->clusterId))
			{
				//I am the bigger cluster
				logt("HANDSHAKE", "I am bigger");

				if(direction == CONNECTION_DIRECTION_IN) StartHandshake();

			}
			else
			{

				//I am the smaller cluster
				logt("HANDSHAKE", "I am smaller");

				//Kill other Connections
				cm->DisconnectOtherConnections(this);

				//Update my own information on the connection
				//this->connectedClusterId = packet->payload.clusterId;
				//this->connectedClusterSize += packet->payload.clusterSize;
				this->partnerId = packet->header.sender;
				this->hopsToSink = packet->payload.hopsToSink < 0 ? -1 : packet->payload.hopsToSink + 1;

				logt("HANDSHAKE", "ClusterSize Change from %d to %d", node->clusterSize, this->connectedClusterSize + 1);

				//node->clusterId = this->connectedClusterId;
				//node->clusterSize += this->connectedClusterSize + 1;
				//this->handshakeDone = true;


				//Send an update to the connected cluster to increase the size by one
				//This is also the ACK message for our connecting node
				connPacketClusterAck1 packet;

				packet.header.messageType = MESSAGE_TYPE_CLUSTER_ACK_1;
				packet.header.sender = node->persistentConfig.nodeId;
				packet.header.receiver = this->partnerId;

				packet.payload.hopsToSink = cm->GetHopsToShortestSink(this);
				packet.payload.reserved = 0;

				logt("HANDSHAKE", "OUT => %d CLUSTER_ACK_1, hops:%d", packet.header.receiver, packet.payload.hopsToSink);

				cm->SendMessage(this, (u8*) &packet, SIZEOF_CONN_PACKET_CLUSTER_ACK_1, true);

				//Update advertisement packets
				//node->UpdateJoinMePacket(NULL);



			}
		}
		else
		{
			logt("CONN", "wrong size for CLUSTER_WELCOME");
		}
		/******* Cluster ack 1 (another node confirms that it is joining our cluster) *******/
	}
	else if (packetHeader->messageType == MESSAGE_TYPE_CLUSTER_ACK_1)
	{
		if (dataLength == SIZEOF_CONN_PACKET_CLUSTER_ACK_1)
		{

			connPacketClusterAck1* packet = (connPacketClusterAck1*) data;

			logt("HANDSHAKE", "IN <= %d  CLUSTER_ACK_1, hops:%d", packet->header.sender, packet->payload.hopsToSink);

			//Update node data
			node->clusterSize += 1;
			this->hopsToSink = packet->payload.hopsToSink < 0 ? -1 : packet->payload.hopsToSink + 1;

			logt("HANDSHAKE", "ClusterSize Change from %d to %d", node->clusterSize-1, node->clusterSize);

			logt("HANDSHAKE", "{\"handshakeMessage\" : {\"message\" : \"IN <= CLUSTER_WELCOME\", \"clustID\" : \"%d\", \"clustSize\" : \"%d\", \"toSink\" : \"%d\", \"nodeId\" : \"%d\"}}",  node->clusterId, node->clusterSize, packet->payload.hopsToSink, packet->header.sender);

			//Update connection data
			this->connectedClusterId = node->clusterId;
			this->partnerId = packet->header.sender;
			this->connectedClusterSize += 1;
			this->handshakeDone = true;

			//Broadcast cluster update to other connections
			connPacketClusterInfoUpdate outPacket;
			outPacket.header.messageType = MESSAGE_TYPE_CLUSTER_INFO_UPDATE;
			outPacket.header.sender = node->persistentConfig.nodeId;
			outPacket.header.receiver = 0;

			outPacket.payload.clusterSizeChange = 1;
			outPacket.payload.currentClusterId = node->clusterId;
			outPacket.payload.newClusterId = 0;


			logt("HANDSHAKE", "OUT => ALL MESSAGE_TYPE_CLUSTER_INFO_UPDATE clustChange:1");

			//Send message to all other connections and update the hops to sink accordingly
			for(int i=0; i<Config->meshMaxConnections; i++){
				if(cm->connections[i] == this || !cm->connections[i]->handshakeDone) continue;
				outPacket.payload.hopsToSink = cm->GetHopsToShortestSink(cm->connections[i]);
				cm->SendMessage(cm->connections[i], (u8*) &outPacket, SIZEOF_CONN_PACKET_CLUSTER_INFO_UPDATE, true);
			}

			//Confirm to the new node that it just joined our cluster => send ACK2
			connPacketClusterAck2 outPacket2;
			outPacket2.header.messageType = MESSAGE_TYPE_CLUSTER_ACK_2;
			outPacket2.header.sender = node->persistentConfig.nodeId;
			outPacket2.header.receiver = this->partnerId;

			outPacket2.payload.clusterId = node->clusterId;
			outPacket2.payload.clusterSize = node->clusterSize;


			logt("HANDSHAKE", "OUT => %d CLUSTER_ACK_2 clustId:%d, clustSize:%d", this->partnerId, node->clusterId, node->clusterSize);

			cm->SendMessage(this, (u8*) &outPacket2, SIZEOF_CONN_PACKET_CLUSTER_ACK_2, true);

			//Update our advertisement packet
			node->UpdateJoinMePacket(NULL);


			logt("HANDSHAKE", "############ Handshake done ###############");

			//Notify Node of handshakeDone
			node->HandshakeDoneHandler(this);

		}
		else
		{
			logt("CONN", "wrong size for ACK1");
		}

		/******* Cluster ack 2 *******/
	}
	else if (packetHeader->messageType == MESSAGE_TYPE_CLUSTER_ACK_2)
	{
		if (dataLength == SIZEOF_CONN_PACKET_CLUSTER_ACK_2)
		{

			connPacketClusterAck2* packet = (connPacketClusterAck2*) data;

			logt("HANDSHAKE", "IN <= %d CLUSTER_ACK_2 clusterID:%d, clusterSize:%d", packet->header.sender, packet->payload.clusterId, packet->payload.clusterSize);

			logt("HANDSHAKE", "ClusterSize Change from %d to %d", node->clusterSize, packet->payload.clusterSize);

			logt("HANDSHAKE", "{\"handshakeMessage\" : {\"message\" : \"IN <= CLUSTER_WELCOME\", \"clustID\" : \"%d\", \"clustSize\" : \"%d\", \"nodeId\" : \"%d\"}}",  packet->payload.clusterId, packet->payload.clusterSize, packet->header.sender);

			this->connectedClusterId = packet->payload.clusterId;
			this->connectedClusterSize += packet->payload.clusterSize - 1; // minus myself

			node->clusterId = packet->payload.clusterId;
			node->clusterSize += packet->payload.clusterSize - 1; // minus myself


			this->handshakeDone = true;

			//Update our advertisement packet
			node->UpdateJoinMePacket(NULL);


			logt("HANDSHAKE", "############ Handshake done ###############");

			//Notify Node of handshakeDone
			node->HandshakeDoneHandler(this);

		}
		else
		{
			logt("CONN", "wrong size for ACK2");
		}

	}


	/*#################### MESSAGE PROCESSING ############################*/
	else
	{
		//Check wether we should care for this packet
		if(
				packetHeader->receiver == node->persistentConfig.nodeId //Directly addressed at us
				|| packetHeader->receiver == NODE_ID_BROADCAST //broadcast packet for all nodes
				|| (packetHeader->receiver >= NODE_ID_HOPS_BASE && packetHeader->receiver < NODE_ID_HOPS_BASE + 1000) //Broadcasted for a number of hops
				|| (packetHeader->receiver == NODE_ID_SHORTEST_SINK && node->persistentConfig.deviceType == deviceTypes::DEVICE_TYPE_SINK)
		){
			//Forward that Packet to the Node
			cm->connectionManagerCallback->messageReceivedCallback(inPacket);
		}
	}

}

#define __________________HELPER______________________
/*######## HELPERS ###################################*/

void Connection::PrintStatus(void)
{
	const char* directionString = (direction == CONNECTION_DIRECTION_IN) ? "< IN " : "> OUT";

	trace("%s %u, handshake:%u, clusterId:%x, clusterSize:%u, toSink:%d, Queue:%u-%u(%u), relBuf:%u, unrelBuf:%u" EOL, directionString, this->partnerId, this->handshakeDone, this->connectedClusterId, this->connectedClusterSize, this->hopsToSink, (packetSendQueue->readPointer - packetSendQueue->bufferStart), (packetSendQueue->writePointer - packetSendQueue->bufferStart), packetSendQueue->_numElements, reliableBuffersFree, unreliableBuffersFree);

}

i8 Connection::GetAverageRSSI()
{
	if(isConnected) return rssiAverage;
	else return 0;
}
/* EOF */

