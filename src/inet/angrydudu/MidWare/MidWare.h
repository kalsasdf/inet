// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 12/04/2019
*/

#ifndef __INET_MIDWARE_H
#define __INET_MIDWARE_H

#include "inet/linklayer/ethernet/EtherMacFullDuplex.h"
#include "inet/common/queue/IPassiveQueue.h"
#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/linklayer/ethernet/EtherMacBase.h"
#include "inet/common/ProtocolTag_m.h"
//#include "inet/common/ProtocolGroup.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/checksum/EthernetCRC.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/Ethernet.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/queue/IPassiveQueue.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/INETUtils.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Message.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/TcpIpChecksum.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
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
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/ipv4/Icmp.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/common/queue/QueueBase.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#include "inet/common/queue/FifoQueue.h"
#include <list>
#include <map>

#include "../ANS/ans_headers.h"

using namespace std;

namespace inet {

class IInterfaceTable;

class MidWare : public cSimpleModule
{
  protected:

    static simsignal_t bufferedPcksSignal;

    std::map<Ipv4Address,cPacketQueue> bufferQueues;
    std::map<Ipv4Address,bool> isSuspendState;

    cGate *OutGate;
    cGate *InGate;
    double LinkSpeed;
    cPacketQueue txPcks;

    CrcMode crcMode = CRC_MODE_UNDEFINED;

    uint32_t bufferedPcks;
    // statistics
    static simsignal_t BufferOccupancySignal;
    cChannel *transmissionChannel = nullptr;    // transmission channel

    // state
    // The temporary parameters used for transfer values
    cMessage *startMsg = nullptr;
    // The temporary parameters used for transfer values
    cMessage *senddata = nullptr;
    cMessage *stopdata = nullptr;
    enum SelfMsgKinds { START = 1, SEND, STOP };

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg);
    virtual void refreshDisplay() const override;

    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendOut(Packet *pck);
    /*
     * Process the Packet from network
     */
    virtual void processReceivedPck(Packet *pck);

    /*
     * Process the suspend from switch
     */
    virtual void processReceivedSuspend(Packet *pck);

    virtual void finish() override;

    virtual void sendInterfaceRegister();

    virtual void sendRegistedPcks();

};
} // namespace inet

#endif // ifndef __INET_MIDWARE_H

