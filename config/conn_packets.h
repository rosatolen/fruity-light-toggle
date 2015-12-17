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

/*
 * This file contains the structs that are used for packets that are sent
 * over standard BLE connections, such as the mesh-handshake and other packets
 */

#pragma once

#include <types.h>

//Start packing all these structures
//These are packed so that they can be transmitted savely over the air
//Smaller datatypes could be implemented with bitfields?
//Sizeof operator is not to be trusted because of padding
// Pay attention to http://www.keil.com/support/man/docs/armccref/armccref_Babjddhe.htm

#pragma pack(push)
#pragma pack(1)


//########## Message types ###############################################

//Mesh clustering and handshake: Protocol defined
#define MESSAGE_TYPE_CLUSTER_WELCOME 20 //The initial message after a connection setup
#define MESSAGE_TYPE_CLUSTER_ACK_1 21 //Both sides must acknowledge the handshake
#define MESSAGE_TYPE_CLUSTER_ACK_2 22 //Second ack
#define MESSAGE_TYPE_CLUSTER_INFO_UPDATE 23 //When the cluster size changes, this message is used

//Others
#define MESSAGE_TYPE_UPDATE_TIMESTAMP 30 //Used to enable timestamp distribution over the mesh

//Module messages: Protocol defined (yet unfinished)
//MODULE_CONFIG: Used for many different messages that set and get the module config
//MODULE_TRIGGER_ACTION: trigger some custom module action
//MODULE_ACTION_RESPONSE: Response on a triggered action
//MODULE_GENERAL: A message, generated by the module not as a response to an action
#define MESSAGE_TYPE_MODULE_CONFIG 50
#define MESSAGE_TYPE_MODULE_TRIGGER_ACTION 51
#define MESSAGE_TYPE_MODULE_ACTION_RESPONSE 52
#define MESSAGE_TYPE_MODULE_GENERAL 53

//Request a listing of all modules and receive this list
#define MESSAGE_TYPE_MODULES_GET_LIST 56
#define MESSAGE_TYPE_MODULES_LIST 57

//Legacy messages: should be removed
#define MESSAGE_TYPE_ADVINFO 60
#define MESSAGE_TYPE_QOS_CONNECTION_DATA 61
#define MESSAGE_TYPE_QOS_REQUEST 62


//Other packets: User space (IDs 80 - 110)
#define MESSAGE_TYPE_DATA_1 80
#define MESSAGE_TYPE_DATA_2 81
#define MESSAGE_TYPE_HEARTBEAT 90
#define MESSAGE_TYPE_LIGHT_TOGGLE 91


//########## Message structs and sizes ###############################################

//If hasMoreParts is set to true, the next message will only contain 1 byte hasMoreParts + messageType
//and all remaining 19 bytes are used for transferring data, the last message of a split message does not have this flag
//activated
#define SIZEOF_CONN_PACKET_HEADER 6
typedef struct
{
	u8 hasMoreParts : 1; //Set to true if message is split and has more data in the next packet
	u8 messageType : 7;
	nodeID sender;
	nodeID receiver;
	nodeID remoteReceiver;
}connPacketHeader;

//Used for message splitting for all packets after the first one
//This way, we do not need to resend the sender and receiver
#define SIZEOF_CONN_PACKET_SPLIT_HEADER 1
typedef struct
{
	u8 hasMoreParts : 1;
	u8 messageType : 7;
}connPacketSplitHeader;

//CLUSTER_WELCOME
#define SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_WELCOME 10
typedef struct
{
	clusterID clusterId;
	clusterSIZE clusterSize;
	u16 meshWriteHandle;
	clusterSIZE hopsToSink;
}connPacketPayloadClusterWelcome;

#define SIZEOF_CONN_PACKET_CLUSTER_WELCOME (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_WELCOME)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadClusterWelcome payload;
}connPacketClusterWelcome;


//CLUSTER_ACK_1
#define SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_ACK_1 3
typedef struct
{
	clusterSIZE hopsToSink;
	u8 reserved;
}connPacketPayloadClusterAck1;

#define SIZEOF_CONN_PACKET_CLUSTER_ACK_1 (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_ACK_1)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadClusterAck1 payload;
}connPacketClusterAck1;


//CLUSTER_ACK_2
#define SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_ACK_2 6
typedef struct
{
	clusterID clusterId;
	clusterSIZE clusterSize;
}connPacketPayloadClusterAck2;

#define SIZEOF_CONN_PACKET_CLUSTER_ACK_2 (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_ACK_2)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadClusterAck2 payload;
}connPacketClusterAck2;


//CLUSTER_INFO_UPDATE
#define SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_INFO_UPDATE 12
typedef struct
{
	clusterID currentClusterId;
	clusterID newClusterId;
	clusterSIZE clusterSizeChange;
	clusterSIZE hopsToSink;
	
}connPacketPayloadClusterInfoUpdate;

#define SIZEOF_CONN_PACKET_CLUSTER_INFO_UPDATE (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_CLUSTER_INFO_UPDATE)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadClusterInfoUpdate payload;
}connPacketClusterInfoUpdate;


//DATA_PACKET
#define SIZEOF_CONN_PACKET_PAYLOAD_DATA_1 (MAX_DATA_SIZE_PER_WRITE - SIZEOF_CONN_PACKET_HEADER)
typedef struct
{
	u8 length;
	u8 data[SIZEOF_CONN_PACKET_PAYLOAD_DATA_1 - 1];
	
}connPacketPayloadData1;

#define SIZEOF_CONN_PACKET_DATA_1 (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_DATA_1)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadData1 payload;
}connPacketData1;




//DATA_2_PACKET
#define SIZEOF_CONN_PACKET_PAYLOAD_DATA_2 (MAX_DATA_SIZE_PER_WRITE - SIZEOF_CONN_PACKET_HEADER)
typedef struct
{
	u8 length;
	u8 data[SIZEOF_CONN_PACKET_PAYLOAD_DATA_2 - 1];

}connPacketPayloadData2;

#define SIZEOF_CONN_PACKET_DATA_2 (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_DATA_2)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadData1 payload;
}connPacketData2;


//Timestamp synchronization packet
#define SIZEOF_CONN_PACKET_UPDATE_TIMESTAMP (SIZEOF_CONN_PACKET_HEADER + 8)
typedef struct
{
	connPacketHeader header;
	u64 timestamp;
}connPacketUpdateTimestamp;

//This message is used for different module request message types


#define SIZEOF_CONN_PACKET_MODULE (SIZEOF_CONN_PACKET_HEADER + 4) //This size does not include the data reagion which is variable, add the used data region size to this size
typedef struct
{
	connPacketHeader header;
	u16 moduleId;
	u8 requestHandle; //Set to 0 if this packet does not need to be identified for reliability (Used to implement end-to-end acknowledged requests)
	u8 actionType;
	u8 data[MAX_DATA_SIZE_PER_WRITE - SIZEOF_CONN_PACKET_HEADER - 4]; //Data can be larger and will be transmitted in subsequent packets

}connPacketModule;


//ADVINFO_PACKET
#define SIZEOF_CONN_PACKET_PAYLOAD_ADV_INFO 9
typedef struct
{
	u8 peerAddress[6];
	u16 inverseRssiSum;
	u8 packetCount;

}connPacketPayloadAdvInfo;

//ADV_INFO
//This packet is used to distribute receied advertising messages in the mesh
//if the packet has passed the filterung
#define SIZEOF_CONN_PACKET_ADV_INFO (SIZEOF_CONN_PACKET_HEADER + SIZEOF_CONN_PACKET_PAYLOAD_ADV_INFO)
typedef struct
{
	connPacketHeader header;
	connPacketPayloadAdvInfo payload;
}connPacketAdvInfo;




//End Packing
#pragma pack(pop)

