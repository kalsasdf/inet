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

#ifndef __INET_IRNCORE_H
#define __INET_IRNCORE_H

#include <map>
#include<queue>

#include <stdlib.h>
#include <string.h>

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
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/networklayer/common/L3Address.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

using namespace std;

namespace inet {

class IInterfaceTable;

class irnCore : public cSimpleModule
{
  protected:
    // configuration for .ned file
    bool activate;
    int cacheCapacity;
    double linkspeed;
    int BDP_pcks;
    int Capsize;
    int BDP_N;
    IInterfaceTable *ift = nullptr;
    CrcMode crcMode = CRC_MODE_UNDEFINED;

    cGate *lowOutGate;
    cGate *lowInGate;
    cGate *upOutGate;
    cGate *upInGate;

    // statistics
    // statistics
    int numMapReceived;
    int numMapDropped;
    static simsignal_t queueLengthSignal;

    cMessage *start_RTO_Timer = nullptr;
    // states for self-scheduling
    enum SelfpckKindValues {
        reset_Timer
    };

  protected:
    // structures for data, credit and informations

    L3Address local_src;         // The source address of sender
    L3Address DestAddr;
    simtime_t RTO_low;
    simtime_t RTO_high;

    std::multimap<L3Address,Packet*> data_Map;
    std::map<L3Address,int> inflight_Map;
    std::map<int,Packet*> BDP_Cap;

    struct snd_QPInfo
    {
        int nextSNtoSend;
        int lastAckedPsn;
        int retransmitSN;
        int bits_to_shift;
        int recoverSN;
        int rate_factor;
        uint64_t sack_bitmap;
        bool doRetransmit;
        bool inRecovery;
        bool quickTimeout;
        bool findNewHole;
    };
    struct MetaData
    {
        short opcode;
        int seqNo;
    };

    //struct to store output of receiveAck module.
    struct AckInfo
    {
        bool ackSyndrome;
        int cumAck;
        int curMSN;
        int sackNo;
        short numCQEDone;
    };

    //struct to store relevant QP context.
    struct rcv_QPInfo
    {
        int expectedSeq;
        int curMSN;
        uint64_t ooo_bitmap;
        uint64_t ooo_bitmap2;
    };

    snd_QPInfo tsndInfo;
    rcv_QPInfo trcvInfo;

    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<L3Address, snd_QPInfo> sender_QPMap;
    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<L3Address, rcv_QPInfo> receiver_QPMap;

    std::map<L3Address,simtime_t> delayMap;// Storing the next-scheduling-time of the credits.


  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg);
    virtual void refreshDisplay() const override;

    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);
    /*
     * Process the Packet from upper layer
     */
    virtual void processUpperpck(Packet *pck);
    /*
     * Process the Packet from lower layer
     */
    virtual void processLowerpck(Packet *pck);

    virtual void snd_sendData(Packet *pck, bool retransmit);
    virtual void snd_receiveAck(Packet *pck);
    virtual void snd_decidebyRTOlow(Packet *pck);
    virtual void snd_decidebyRTOhigh(Packet *pck);
    virtual void snd_findNewHole(L3Address addr);
    virtual Packet *snd_findData(L3Address addr);
    virtual Packet *snd_extractData(L3Address addr);
    virtual Packet *snd_insertData(L3Address addr,Packet *pck);
    virtual Packet *snd_copytoBDPmap(int seq,Packet *pck);

    virtual void rcv_sendAck(L3Address destaddr, int sackNo, int cumAck, bool ackSyndrome, simtime_t pckTS);
    virtual void rcv_receiveData(Packet *pck);

};
} // namespace inet

#endif // ifndef __INET_IRNCORE_H

