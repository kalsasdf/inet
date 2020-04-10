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

#ifndef __INET_MCCMCORE_H
#define __INET_MCCMCORE_H

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
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/networklayer/common/L3Address.h"

#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "inet/transportlayer/tcp/Tcp.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

#include <map>

using namespace std;

namespace inet {

class IInterfaceTable;

class MccmCore : public cSimpleModule
{
  protected:
    // configuration for .ned file
    bool multipath;
    int mp_algorithm; // default 0, for random spreading. 1, for decrease by ratio.
    bool activate;
    int frameCapacity;
    int bits_timeout;
    double linkspeed;
    int credit_size;
    bool useECN;
    bool useRe_order;
    double gama;
    int sumpaths;
    bool rate_control;
    bool randomspreading;
    int max_ooo;
    int sum_tenants;

    //double segma = 1; // for hybrid flow;
    //int times_of_RTT = 1; // for hybrid flow;
    double segma = 0.1; // for ecn based control;
    int times_of_RTT = 2; // for deciding the RTT;

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
    static simsignal_t ooodegreeSignal;
    cOutVector OoO_Vector;

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

    std::multimap<L3Address,Packet*> receiver_orderMap;
    std::map<L3Address,long int> receiver_seqCache;

    simtime_t last_ordering_time = 0;
    simtime_t ordering_time = 0;

    typedef struct {
            simtime_t last_Fbtime;
            simtime_t nowRTT;
            int pck_in_rtt;
            // Used for credit feedback control.
            int creditseq;
            int lastseq;
            int nowseq;
            int sumlost;
            double omega;

            // Used for ECN-based rate control.
            int ecn_in_rtt;
            double ecn_alpha;
            double Trate;

            // Credit sending speed (after the feedback control)
            double newspeed;
            double max_speedsum;

            bool previousincrease;
        }receiver_flowinfo;
    struct receiver_mpinfo{
                int virtualpaths;
                unsigned int path_id[256];
                double speed[256];
                simtime_t next_time[256];
            };
    struct sender_flowinfo{
            // Used for determining whether the flow has been dried.
            simtime_t cretime;
            // for re-ordering
            long int seq_No;
            bool stop_sent;
        };

    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<L3Address, sender_flowinfo> sender_flowMap;

    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<L3Address, receiver_flowinfo> receiver_flowMap;

    std::map<L3Address, receiver_mpinfo> receiver_mpMap;

    std::map<L3Address,simtime_t> delayMap;// Storing the next-scheduling-time of the credits.

  protected:

    double time_jittered = 0;

    // The time used for determine whether the flow has been dried at the sender
    simtime_t delta_cdt_time;
    simtime_t last_cdt_time;
    simtime_t next_cdt_delay;
    simtime_t max_idletime;
    simtime_t timestamp;

    L3Address next_creditaddr;

    // Packets requested by the credits
    int registedCredits;

    receiver_mpinfo treceiver_mpinfo;
    receiver_flowinfo treceiver_flowinfo;
    sender_flowinfo tsender_flowinfo;
    // Credit feedback control
    double alpha;
    double targetratio;
    double wmax;
    double maxrate;
    double currate;
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
    virtual void send_credit(L3Address destaddr, unsigned int pathid, int seq);
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
    virtual void schedule_next_credit(simtime_t delta_t);
    virtual simtime_t get_singleRTT(Packet *pck);

    virtual void multipath_control(L3Address addr, double speed, double oldspeed, double max_sumspeed);

    virtual void orderpackets(L3Address addr);

    virtual void finish() override;

    virtual receiver_flowinfo feedback_control(receiver_flowinfo tinfo,bool sumrate)
    {
        if (rate_control)
        {
            if (multipath)
            {
                double nowrate = tinfo.max_speedsum;
                int sumlost = tinfo.sumlost;
                int sumcredits = tinfo.pck_in_rtt+tinfo.sumlost;
                double w = tinfo.omega;
                double lossratio = double(sumlost)/double(sumcredits);
                if (sumrate)
                {
                    EV << "feedback_control(), adjust max_sumspeed, sumlost = "<< sumlost << ", sumcredits = "<< sumcredits<<endl;
                    EV << "feedback_control(), adjust max_sumspeed, loss ratio = "<< lossratio <<endl;
                    if(lossratio<=targetratio)
                    {
                        if(tinfo.previousincrease)
                        {
                            w = (w+wmax)/2;
                        }
                        nowrate = (1-w)*nowrate + w*maxrate;
                        tinfo.previousincrease = true;
                    }
                    else
                    {
                        nowrate = nowrate*(1-lossratio)*(1+targetratio);
                        if (w/2>wmin)
                            w = w/2;
                        else
                            w = wmin;
                        tinfo.previousincrease = false;
                    }
                    tinfo.omega = w;
                    //tinfo.newspeed = nowrate;
                    tinfo.max_speedsum = nowrate;
                    return tinfo;
                }
                else
                {
                    double nowrate = tinfo.newspeed;
                    EV << "feedback_control(), loss ratio = "<< lossratio <<endl;
                    nowrate = nowrate*(1-lossratio/2);
                    tinfo.newspeed = nowrate;
                    //EV << "ecnbased_control(), after speed changing, newspeed = "<< tinfo.newspeed <<endl;
                    return tinfo;
                }
            }
                else
                {
                    double nowrate = tinfo.newspeed;
                    int sumlost = tinfo.sumlost;
                    int sumcredits = tinfo.pck_in_rtt+tinfo.sumlost;
                    double w = tinfo.omega;
                    double lossratio = double(sumlost)/double(sumcredits);
                    EV << "feedback_control(), sumlost = "<< sumlost << ", sumcredits = "<< sumcredits<<endl;
                    EV << "feedback_control(), loss ratio = "<< lossratio <<endl;
                    if(lossratio<=targetratio)
                    {
                        if(tinfo.previousincrease)
                        {
                            w = (w+wmax)/2;
                        }
                        nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio);
                        tinfo.previousincrease = true;
                    }
                    else
                    {
                        //nowrate = nowrate*(1-tinfo.ecn_alpha)*(1+targetratio);// TODO
                        nowrate = nowrate*(1-lossratio)*(1+targetratio);
                        if (w/2>wmin)
                            w = w/2;
                        else
                            w = wmin;
                        tinfo.previousincrease = false;
                    }
                    tinfo.omega = w;
                    tinfo.newspeed = nowrate;
                    return tinfo;
                }
        }
        else
        {
            tinfo.newspeed = maxrate;
            return tinfo;
        }
    };


    virtual receiver_flowinfo ecnbased_control(receiver_flowinfo tinfo,bool sumrate)
    {
        if(rate_control){
            if(multipath)
            {
                double ecnratio = double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt);
                EV<<"multipath enabled, omega = "<<tinfo.omega<<", segma = "<<segma<<endl;
                if(sumrate)
                {
                    double nowrate = tinfo.max_speedsum;
                    double w = tinfo.omega;
                    EV << "ecnbased_control(), adjust max_speedsum, ecn-marking ratio = "<< ecnratio << ", old sum speed = " <<tinfo.max_speedsum<<endl;
                    if(ecnratio<=targetratio)
                    {
                        if(tinfo.previousincrease)
                        {
                            w = (w+wmax)/2;
                        }
                        nowrate = (1-w)*nowrate + w*maxrate;
                        tinfo.previousincrease = true;
                    }
                    else if(ecnratio<=max_lossratio)
                    {
                        nowrate = nowrate*(1-ecnratio*segma)*(1+targetratio*segma);
                        if (w/2>wmin)
                            w = w/2;
                        else
                            w = wmin;
                        tinfo.previousincrease = false;
                    }
                    else
                    {
                        nowrate = nowrate*(1-max_lossratio*segma)*(1+targetratio*segma);
                        if (w/2>wmin)
                            w = w/2;
                        else
                            w = wmin;
                        tinfo.previousincrease = false;
                    }
                    tinfo.omega = w;
                    //tinfo.newspeed = nowrate;
                    tinfo.max_speedsum = nowrate;
                    return tinfo;
                }
                else
                {
                    double nowrate = tinfo.newspeed;
                    EV << "ecnbased_control(), ecn-marking ratio = "<< ecnratio <<endl;
                    nowrate = nowrate*(1-ecnratio/2);
                    tinfo.newspeed = nowrate;
                    //EV << "ecnbased_control(), after speed changing, newspeed = "<< tinfo.newspeed <<endl;
                    return tinfo;
                }
            }
            else
            {
                double ecnratio = double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt);
                double w = tinfo.omega;
                double nowrate = tinfo.newspeed;
                EV << "feedback_control(), ecn ratio = "<< ecnratio <<", omega = "<<w<<endl;
                if(ecnratio<=targetratio)
                {
                    if(tinfo.previousincrease)
                    {
                        w = (w+wmax)/2;
                    }
                    nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio*segma);
                    tinfo.previousincrease = true;
                }
                else if(ecnratio<=max_lossratio)
                {
                    nowrate = nowrate*(1-ecnratio*segma)*(1+targetratio*segma);
                    if (w/2>wmin)
                        w = w/2;
                    else
                        w = wmin;
                    tinfo.previousincrease = false;
                }
                else
                {
                    nowrate = nowrate*(1-max_lossratio*segma)*(1+targetratio*segma);
                    if (w/2>wmin)
                        w = w/2;
                    else
                        w = wmin;
                    tinfo.previousincrease = false;
                }
                tinfo.omega = w;
                tinfo.newspeed = nowrate;
                return tinfo;
            }
        }
        else
        {
            tinfo.newspeed = maxrate;
            return tinfo;
        }
    };
};
} // namespace inet

#endif // ifndef __INET_MCCMCORE_H

