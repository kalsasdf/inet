// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#ifndef __INET_HOMA_H
#define __INET_HOMA_H

#include "homa_headers.h"
#include "prio_scheduler.h"

using namespace std;

namespace inet {

class IInterfaceTable;

class homa : public cSimpleModule
{
  protected:
    // configuration for .ned file
    bool activate;
    int frameCapacity;
    double linkspeed;
    int sumpaths;
    simtime_t baseRTT;
    int64_t max_pck_size;

    const char *packetName = "HomaData";
    IInterfaceTable *ift = nullptr;
    CrcMode crcMode = CRC_MODE_UNDEFINED;
    uint16_t crcNumber = 0;

    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    // statistics
    // statistics
    int numMapReceived;
    int numMapDropped;
    static simsignal_t queueLengthSignal;

    // states for self-scheduling
    enum SelfpckKindValues {
        SENDDATA,
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
    std::queue<L3Address> nextAddrQueue;
    std::queue<short> nextSchePrioQueue;

    typedef struct {
        simtime_t nowRTT;
        int pck_in_rtt;
    }receiver_flowinfo;

    typedef struct {
        L3Address srcAddr;
        L3Address destAddr;
        int pckseq;
        int flowid;
        int srcPort;
        int destPort;
        int socketId;
        CrcMode crcMode;
        uint16_t crc;
        int64_t remainLength;
        int64_t RTTbytes;
        simtime_t creaTime;
        uint16_t unscheduledPrio;
    }sender_flowinfo;


    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<L3Address, sender_flowinfo> sender_flowMap;

    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<L3Address, receiver_flowinfo> receiver_flowMap;

    std::map<L3Address,simtime_t> delayMap;// Storing the next-scheduling-time of the credits.


  protected:
    enum PrioResolutionMode {
        STATIC_CDF_UNIFORM,
        STATIC_CBF_UNIFORM,
        STATIC_CBF_GRADUATED,
        EXPLICIT,
        FIXED_UNSCHED,
        INVALID_PRIO_MODE        // Always the last value
    };
  private:
    uint32_t maxSchedPktDataBytes;
    const WorkloadEstimator::CdfVector* cdf;
    const WorkloadEstimator::CdfVector* cbf;
    const WorkloadEstimator::CdfVector* cbfLastCapBytes;
    const WorkloadEstimator::CdfVector* remainSizeCbf;
    std::vector<uint32_t> prioCutOffs;
    WorkloadEstimator* distEstimator;
    PrioResolutionMode prioResMode;
    HomaConfigDepot* homaConfig;
    std::vector<uint32_t> eUnschedPrioCutoff;

  protected:

    receiver_flowinfo treceiver_flowinfo;
    sender_flowinfo tsender_flowinfo;

    // judging which destination the credit from
    L3Address sender_srcAddr;         // The source address of sender
    L3Address receiver_srcAddr;         // The source address of receiver

    L3Address next_destAddr;
    short next_schePrio;

    // The temporary parameters used for transfer values
    cMessage *senddata = nullptr;
    cMessage *stopdata = nullptr;

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
    /**
     * Process the self message, send credit or stop credit.
     */
    virtual void find_nextaddr();
    /**
     * Send credit and request.
     */
    virtual void send_data(L3Address destaddr);
    virtual void send_resend(L3Address destaddr);
    virtual void send_grant(L3Address destaddr, unsigned int pathid, int seq);
    virtual void send_busy(L3Address addr);
    /*
     * Process the Packet from upper layer
     */
    virtual void processUpperpck(Packet *pck);
    /*
     * Process the Packet from lower layer
     */
    virtual void processLowerpck(Packet *pck);
    /*
     * Receive XXX, and what should TODO next?
     */
    virtual void receive_grant(Packet *pck);
    virtual void receive_resend(Packet *pck);
    virtual void receive_busy(Packet *pck);
    virtual void receive_data(Packet *pck);
    // To do feedback control.
    virtual void schedule_next_grant(simtime_t delta_t);

    virtual bool orderpackets(L3Address addr);

    virtual void finish() override;

    virtual uint16_t getMesgPrio(uint32_t msgSize);
    virtual void setPrioCutOffs();



};
} // namespace inet

#endif // ifndef __INET_HOMA_H

