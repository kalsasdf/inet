// Copyright (C) 2018 Angrydudu
/*
 *
    Best Effort
    DSCP_BE = 0; //data

    Expedited Forwarding, RFC 2598
    DSCP_EF   = 0x2E; // 101110 credit

    DSCP_AF11 = 0x0A; // 001010 ACK
    DSCP_AF12 = 0x0C; // 001100 credit_req/stop
    DSCP_AF13 = 0x0E; // 001110

    DSCP_AF21 = 0x12; // 010010
    DSCP_AF22 = 0x14; // 010100
    DSCP_AF23 = 0x16; // 010110

    DSCP_AF31 = 0x1A; // 011010
    DSCP_AF32 = 0x1C; // 011100
    DSCP_AF33 = 0x1E; // 011110

    DSCP_AF41 = 0x22; // 100010
    DSCP_AF42 = 0x24; // 100100
    DSCP_AF43 = 0x26; // 100110

    DSCP_CS1  = 0x08; // 001000
    DSCP_CS2  = 0x10; // 010000
    DSCP_CS3  = 0x18; // 011000
    DSCP_CS4  = 0x20; // 100000
    DSCP_CS5  = 0x28; // 101000
    DSCP_CS6  = 0x30; // 110000
    DSCP_CS7  = 0x38; // 111000
*/

#include <map>
#include <queue>

#include <stdlib.h>
#include <string.h>

#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Message.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/TcpIpChecksum.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/queue/QueueBase.h"
#include "inet/common/IProtocolRegistrationListener.h"

#include "inet/applications/common/SocketTag_m.h"

#include "inet/networklayer/arp/ipv4/ArpPacket_m.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/common/EcnTag_m.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/common/MulticastTag_m.h"
#include "inet/networklayer/common/NextHopAddressTag_m.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4SocketCommand_m.h"
#include "inet/networklayer/ipv4/IcmpHeader_m.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/ipv4/Icmp.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/networklayer/common/L3Address.h"

#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "inet/transportlayer/tcp/Tcp.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/common/L4Tools.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/transportlayer/udp/Udp.h"
