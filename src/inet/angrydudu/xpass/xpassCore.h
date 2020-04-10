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

#ifndef __INET_XPASSCORE_H
#define __INET_XPASSCORE_H

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

#include <map>

using namespace std;

namespace inet {

class IInterfaceTable;

class xpassCore : public cSimpleModule
{
  protected:
    // configuration for .ned file
    bool activate;
    int frameCapacity;
    int bits_timeout;
    double linkspeed;
    int credit_size;
    IInterfaceTable *ift = nullptr;
    CrcMode crcMode = CRC_MODE_UNDEFINED;

    cGate *outGate;
    cGate *inGate;
    cGate *upGate;
    cGate *downGate;

    // statistics
    // statistics
    int numMapReceived;
    int numMapDropped;
    static simsignal_t queueLengthSignal;

    // states for self-scheduling
    enum SelfpckKindValues {
        SENDCRED,
        STOPCRED
    };
    enum creditState {
        INITIAL_STATE,
        SEND_CREDIT_STATE,
        STOP_CREDIT_STATE
    };
    creditState CreState;
  protected:
    // structures for data, credit and informations

    std::multimap<L3Address,Packet*> data_Map;
    std::multimap<L3Address,Packet*> credit_Map;

    struct receiver_flowinfo{
            // Used for determining whether the flow has been dried.
            simtime_t last_Fbtime;
            simtime_t nowRTT;
            int pck_in_rtt;
            // Used for credit feedback control.
            int creditseq;
            int lastseq;
            int nowseq;
            int sumlost;
            // Credit sending speed (after the feedback control)
            double newspeed;
        };
    struct sender_flowinfo{
            // Used for determining whether the flow has been dried.
            simtime_t cretime;
        };
    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<L3Address, sender_flowinfo> sender_flowMap;
    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<L3Address, receiver_flowinfo> receiver_flowMap;

    std::map<L3Address,simtime_t> delayMap;// Storing the next-scheduling-time of the credits.

  protected:

    // The time used for determine whether the flow has been dried at the sender
    simtime_t delta_cdt_time;
    simtime_t last_cdt_time;
    simtime_t next_cdt_delay;
    simtime_t max_idletime;
    simtime_t timestamp;

    L3Address next_creditaddr;

    // The number of the flows
    int sender_flows;
    int receiver_flows;

    // Packets requested by the credits
    int registedCredits;

    receiver_flowinfo treceiver_flowinfo;
    sender_flowinfo tsender_flowinfo;
    // Credit feedback control
    bool previousincrease;
    double targetratio;
    double wmax;
    double maxrate;
    double currate;
    double w;
    double wmin;
    double max_lossratio;

    // judging which destination the credit from
    L3Address tdest;
    L3Address stop_addr;
    L3Address sndsrc;         // The source address of sender
    L3Address rcvsrc;         // The source address of receiver

    // The temporary parameters used for transfer values
    Packet *temp_info;
    Packet *stopcdt_info;         // store the information of credit stop packet at the receiver.
    cMessage *sendcredit = nullptr;
    cMessage *stopcredit = nullptr;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg);
    virtual void refreshDisplay() const override;

    virtual Packet *findCredit(L3Address addr);
    virtual Packet *extractCredit(L3Address addr);
    virtual Packet *insertCredit(L3Address addr,Packet *pck);
    virtual Packet *findData(L3Address addr);
    virtual Packet *extractData(L3Address addr);
    virtual Packet *insertData(L3Address addr,Packet *pck);
    virtual Packet *newflowinfo(Packet *pck);
    virtual int deleteFlowCredits(L3Address addr);
    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);
    /**
     * Process the self message, send credit or stop credit.
     */
    virtual void self_send_credit();
    virtual void self_stop_credit();
    virtual void find_nextaddr();
    /**
     * Send credit and request.
     */
    virtual void send_credreq(L3Address destaddr);
    virtual void send_credit(L3Address destaddr,int seq);
    /**
     * Send credit stop.
     */
    virtual void send_stop(L3Address addr);
    /*
     * Process the Packet from upper layer
     */
    virtual void processUpperpck(Packet *pck);
    /*
     * Process the Packet from lower layer
     */
    virtual void processLowerpck(Packet *pck);
    /*
     * Receive the xxx, and what should todo next?
     */
    virtual void receive_credit(Packet *pck);
    virtual void receive_credreq(Packet *pck);
    virtual void receive_stopcred(Packet *pck);
    virtual void receive_data(Packet *pck);
    // To do feedback control.
    virtual double feedback_control(double speed, int sumlost,int sumcredits);
    virtual void schedule_next_credit(simtime_t delta_t);
    virtual simtime_t get_singleRTT(Packet *pck);
};
} // namespace inet

#endif // ifndef __INET_XPASSCORE_H

