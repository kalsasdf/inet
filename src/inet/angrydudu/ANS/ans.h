// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#ifndef __INET_ANS_H
#define __INET_ANS_H

#include "../ANS/ans_headers.h"

using namespace std;

namespace inet {

class IInterfaceTable;

class ans: public cSimpleModule
{
protected:
    typedef struct {
        L3Address Sender_srcAddr;
        L3Address Sender_destAddr;
        simtime_t nowHTT;
        int grantSequence;
        int pck_in_rtt;
        uint32_t flowid;
    }receiver_flowinfo;

    typedef struct {
        L3Address srcAddr;
        L3Address destAddr;
        uint32_t flowid;
        int pckseq;
        int srcPort;
        int destPort;
        int socketId;
        CrcMode crcMode;
        uint16_t crc;
        int64_t remainLength;
        int64_t RTTbytes;
        simtime_t creaTime;
        TimerMsg *senddata = nullptr;
    }sender_flowinfo;

  protected:
    // configuration for .ned file
    bool activate;
    int frameCapacity;
    double linkspeed;
    int sumpaths;
    simtime_t baseRTT;
    int64_t max_pck_size;

    const char *packetName = "ansData";
    IInterfaceTable *ift = nullptr;
    CrcMode crcMode = CRC_MODE_UNDEFINED;
    uint16_t crcNumber = 0;
    std::vector<uint32_t> prioCutOffs;
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
        STOPDATA
    };

  protected:
    // structures for data, grant and informations

    std::multimap<L3Address,Packet*> data_Map;
    std::multimap<L3Address,Packet*> grant_Map;
    std::queue<L3Address> nextAddrQueue;
    std::queue<short> nextSchePrioQueue;


    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<uint32_t, sender_flowinfo> sender_flowMap;

    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<uint32_t, receiver_flowinfo> receiver_flowMap;

    std::map<uint32_t,simtime_t> delayMap;// Storing the next-scheduling-time of the grants.

  protected:

    receiver_flowinfo treceiver_flowinfo;
    sender_flowinfo tsender_flowinfo;

    // judging which destination the grant from
    L3Address sender_srcAddr;         // The source address of sender
    L3Address receiver_srcAddr;         // The source address of receiver

    L3Address next_destAddr;
    short next_schePrio;

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
     * Process the self message, send grant or stop grant.
     */
    virtual void find_nextaddr();
    /**
     * Send grant and request.
     */
    virtual void send_data(uint32_t flowid);
    virtual void send_resend(uint32_t flowid);
    virtual void send_grant(uint32_t flowid, unsigned int pathid, int seq);
    virtual void send_busy(uint32_t addr);
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

    virtual bool orderpackets(uint32_t flowid);

    virtual void finish() override;

};
} // namespace inet

#endif // ifndef __INET_ANS_H

