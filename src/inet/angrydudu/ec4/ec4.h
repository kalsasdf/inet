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

#ifndef __INET_MPC_H
#define __INET_MPC_H

#include <map>
#include <queue>

#include <stdlib.h>
#include <string.h>
#include<algorithm>
#include<vector>
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/TimeTag_m.h"
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

#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/common/L4Tools.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/transportlayer/udp/Udp.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"


#include "inet/angrydudu/TimerMsg/TimerMsg_m.h"

using namespace std;

namespace inet {

class IInterfaceTable;

class ec4 : public cSimpleModule
{
  protected:
    typedef enum{
        Normal,
        Fast_Recovery,
        Additive_Increase,
        Hyper_Increase
    }SenderState;
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
    double gamma;
    int sumpaths;
    bool rate_control;
    bool randomspreading;
    bool useTokens;
    int max_ooo;
    int percdtTokens;
    int seq;
    int frSteps_th;
    int ByteCounter_th;
    double Rai;
    double Rhai;
    int64_t max_pck_size;

    TimerMsg *sendcredit=nullptr;
    TimerMsg *stopcredit=nullptr;
    TimerMsg *alphaTimer=nullptr;
    TimerMsg *rateTimer=nullptr;

    int64_t globaltimes = 1;

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
        STOPCRED,
        RATETIMER,
        ALPHATIMER
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
    std::multimap<L3Address,Packet*> temp_orderMap;
    std::map<L3Address,long int> receiver_seqCache;

    simtime_t last_ordering_time = 0;
    simtime_t ordering_time = 0;
    simtime_t AlphaTimer_th;
    simtime_t RateTimer_th;
    const char *packetName = "EC4Data";

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
            double modeflag;
            double alpha;
            double gamma;

            // Used for ECN-based rate control.
            int ecn_in_rtt;
            double ecn_alpha;
            int ByteCounter;
            int iRhai;
            int ByteFrSteps;
            int TimeFrSteps;
            simtime_t LastAlphaTimer;
            simtime_t LastRateTimer;

            // Credit sending speed (after the feedback control)
            double newspeed;
            double targetRate;
            double max_speedsum;

            bool previousincrease;
            SenderState SenderState;
        }receiver_flowinfo;
    struct receiver_mpinfo{
                int virtualpaths;
                unsigned int path_id[256];
                double speed[256];
                simtime_t next_time[256];
            };
    struct sender_flowinfo{
        L3Address srcAddr;
        L3Address destAddr;
        int pckseq;
        uint32_t flowid;
        int srcPort;
        int destPort;
        int socketId;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        int64_t remainLength;
        simtime_t creaTime;
        simtime_t nxtSendTime;
        simtime_t LastAlphaTimer; // for updating Alpha
        simtime_t LastRateTimer; // for gaining rate
        int ByteCounter; // for gaining rate
        double currentRate;
        double targetRate;
        double maxTxRate;
        double alpha;
        int iRhai;
        SenderState SenderState;
        int ByteFrSteps; // rate have been increased for frSteps times.
        int TimeFrSteps; // rate have been increased for frSteps times.
           /* // Used for determining whether the flow has been dried.
            simtime_t cretime;
            // for re-ordering
            //long int seq_N;
            vector<int> seq_No;
            bool stop_sent;*/
        };
    struct sender_tokeninfo{
                // Used for determining whether the flow has been dried.
                int tokens;
                int cdt_in_rtt;
                int ecn_in_rtt;
                short freshPathID;
                // for re-ordering
                double EcnPrtt;
                simtime_t lastEcnTime;
            };

    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<L3Address, sender_flowinfo> sender_flowMap;

    std::map<L3Address, sender_tokeninfo> sender_tokens;

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

    receiver_flowinfo treceiver_flowinfo;
    //sender_flowinfo tsender_flowinfo;
    sender_tokeninfo tsender_tokeninfo;
    // Credit feedback control
    double alpha;
    double targetratio;
    double targetecnratio;
    double wmax;
    double maxrate;
    double currate;
    double wmin;
    double max_lossratio;
    double modethreshold;


    // judging which destination the credit from
    L3Address tdest;
    L3Address stop_addr;
    L3Address sndsrc;         // The source address of sender
    L3Address rcvsrc;         // The source address of receiver

    // The temporary parameters used for transfer values
    Packet *temp_info;
    Packet *stopcdt_info;         // store the information of credit stop packet at the receiver.


  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void updateAlpha(L3Address addr);
    virtual void increaseTxRate(L3Address addr);
    virtual void handleSelfMessage(cMessage *msg);
    virtual void refreshDisplay() const override;
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

    virtual void finish() override;

    virtual receiver_flowinfo feedback_control(receiver_flowinfo tinfo,bool sumrate)
    {
        if (rate_control)
        {
            double oldspeed = tinfo.newspeed;
            double nowrate = tinfo.newspeed;
            int sumlost = tinfo.sumlost;
            int sumcredits = tinfo.pck_in_rtt+tinfo.sumlost;
            double w = tinfo.omega;
            double MODEFLAG=tinfo.modeflag;
            double ECN_a=tinfo.alpha;
            double g=tinfo.gamma;
            double lossratio = double(sumlost)/double(sumcredits);
            double ecnratio = double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt);
            if (lossratio<0||lossratio>1) lossratio = 1;
            EV << "feedback_control(), nowRTT = "<< tinfo.nowRTT << ", ecnratio = "<< ecnratio<<",loss ratio="<<lossratio<<endl;
                //modified by dinghuang
            if(ecnratio<=targetecnratio){
            }
            else{
                //tinfo.targetRate=nowrate;
                EV<<"ecn functioning.old ECN_a="<<ECN_a<<endl;
                        //",targetRate="<<tinfo.targetRate<<endl;
                ECN_a=(1-g)*ECN_a+g*ecnratio;
                EV<<"new ECN_a="<<ECN_a<<endl;
                nowrate=nowrate*(1-(targetecnratio+ECN_a)/2);
                EV<<"after ecn decreasing,nowrate="<<nowrate<<endl;
                tinfo.alpha=ECN_a;
                tinfo.ByteCounter=0;
                tinfo.LastRateTimer=0;
                tinfo.ByteFrSteps=0;
                tinfo.TimeFrSteps=0;
                tinfo.LastAlphaTimer=0;
                tinfo.LastAlphaTimer=0;
                tinfo.iRhai=0;
                cancelEvent(alphaTimer);
                cancelEvent(rateTimer);
                // update alpha
                scheduleAt(simTime()+AlphaTimer_th,alphaTimer);

                // schedule to rate increase event
                scheduleAt(simTime()+RateTimer_th,rateTimer);
            }

            if(lossratio<=targetratio)
            {
                //tinfo.targetRate=nowrate;
                EV<<"loss increasing speed.before w="<<w<<endl;
                        //",targetRate="<<tinfo.targetRate<<endl;
                w = (w+wmax)/2;
                nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio);
                EV<<"now w="<<w<<endl;
            }
            else
            {
                EV<<"loss decreasing speed."<<endl;
                EV<<"before w="<<w<<endl;
                nowrate = nowrate*(1-lossratio)*(1+targetratio);
                if (w/2>wmin)
                    w = w/2;
                else
                    w = wmin;
                EV<<"now w="<<w<<endl;
            }
            tinfo.omega = w;
            EV<<"oldspeed="<<oldspeed<<",newspeed="<<nowrate<<endl;
            tinfo.newspeed = nowrate;
            return tinfo;
        }
        else
        {
            tinfo.newspeed = maxrate;
            return tinfo;
        }
    };
};
} // namespace inet

#endif // ifndef __INET_MPC_H

