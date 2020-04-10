// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#ifndef __INET_DCQCN_H
#define __INET_DCQCN_H

#include "dcqcn_headers.h"

using namespace std;

namespace inet {

class IInterfaceTable;

class dcqcn : public cSimpleModule
{
protected:
    typedef enum{
        Normal,
        Fast_Recovery,
        Additive_Increase,
        Hyper_Increase
    }SenderState;

    typedef struct {
        simtime_t nowHTT; // Half Trip Time, from sender to receiver.
        simtime_t lastCnpTime;
        TimerMsg *cnpTimer = nullptr;
        bool ecnPakcetReceived;
        int cnpSequence;
        uint32_t flowid;
        L3Address Sender_srcAddr;
        L3Address Sender_destAddr;
    }receiver_flowinfo;

    typedef struct {
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
        TimerMsg *senddata = nullptr;
        TimerMsg *stopdata = nullptr;
        TimerMsg *alphaTimer = nullptr;
        TimerMsg *rateTimer = nullptr;
    }sender_flowinfo;
protected:
    // configuration for .ned file
    bool activate;
    int frameCapacity;
    double linkspeed;
    double gamma;
    int sumpaths;
    simtime_t baseRTT;
    int64_t max_pck_size;
    simtime_t nxtTxTime;
    sender_flowinfo nxtTxFlow;
    simtime_t min_cnp_interval;
    simtime_t AlphaTimer_th;
    simtime_t RateTimer_th;
    int ByteCounter_th;
    int frSteps_th;
    double Rai;
    double Rhai;

    const char *packetName = "DcqcnData";
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

    static simsignal_t txRateSignal;

    static simsignal_t cnpReceivedSignal;

    // states for self-scheduling
    enum SelfpckKindValues {
        SENDDATA,
        STOPDATA,
        ALPHATIMER,
        RATETIMER,
        CNPTIMER
    };

protected:

    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<uint32_t, sender_flowinfo> sender_flowMap;

    // The destinations addresses of the flows at receiver, The flow information at receiver
    std::map<uint32_t, receiver_flowinfo> receiver_flowMap;

protected:

    // judging which destination the credit from
    L3Address sender_srcAddr;         // The source address of sender
    L3Address receiver_srcAddr;         // The source address of receiver

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
    //virtual void scheduleNxtPck();
    /**
     * Send credit and request.
     */
    virtual void send_data(uint32_t flowid);
    virtual void send_cnp(uint32_t flowid);
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
    virtual void receive_cnp(Packet *pck);
    virtual void receive_data(Packet *pck);

    /*
     * update all flows' maximum TX rate and current TX rate
     */
    virtual void updateAllTxRate();

    /*
     * rate increase at the sender
     */
    virtual void increaseTxRate(uint32_t flowid);

    virtual void updateAlpha(uint32_t flowid);

    /*
     * re-order packets based on the packets sequence number
     */
    virtual bool orderpackets(uint32_t flowid);

    virtual void finish() override;
};

} // namespace inet

#endif // ifndef __INET_DCQCN_H

