// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 12/04/2019
*/

#include "MidWare.h"

namespace inet {

Define_Module(MidWare);

// Bytes
simsignal_t MidWare::BufferOccupancySignal = registerSignal("BufferOccupancy");

void MidWare::initialize()
{
    //statistics
    const char *crcModeString = par("crcMode");
    LinkSpeed = par("linkspeed");
    LinkSpeed = LinkSpeed * 1e9;
    if (!strcmp(crcModeString, "declared"))
        crcMode = CRC_DECLARED_CORRECT;
    else if (!strcmp(crcModeString, "computed"))
        crcMode = CRC_COMPUTED;
    else
        throw cRuntimeError("Unknown crc mode: '%s'", crcModeString);
    bufferQueues.clear();
    isSuspendState.clear();
    bufferedPcks = 0;

    emit(BufferOccupancySignal, (unsigned long)bufferedPcks);
    WATCH(bufferedPcks);

    OutGate = gate("phys$o");
    InGate = gate("phys$i");
    startMsg = new cMessage("startTimer");
    startMsg->setKind(START);
    senddata = new cMessage("sendTimer");
    senddata->setKind(SEND);
    stopdata = new cMessage("stopTimer");
    stopdata->setKind(STOP);
    txPcks = cPacketQueue();
    transmissionChannel = check_and_cast_nullable<cDatarateChannel *>(OutGate->findTransmissionChannel());
    scheduleAt(simTime(),startMsg);
}

void MidWare::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleSelfMessage(msg);// sendcredit
    else
    {
        if (msg->arrivedOn("phys$i"))
        {
            if (string(msg->getFullName()).find("Suspend") != string::npos)
            {
                processReceivedSuspend(check_and_cast<Packet*>(msg));
            }
            else
            {
                processReceivedPck(check_and_cast<Packet*>(check_and_cast<EthernetSignal *>(msg)->decapsulate()));
            }
        }
        else
        {
            throw cRuntimeError("Packet comes from unknown gate.");
        }
    }
}

void MidWare::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << pck << " received\n";

    switch (pck->getKind()) {
        case START:
            sendInterfaceRegister();
            break;
        case SEND:
            sendRegistedPcks();
            break;
        case STOP:
            break;
    }
}

void MidWare::refreshDisplay() const
{

}

void MidWare::processReceivedPck(Packet *pck)
{
    bufferedPcks += pck->getByteLength();
    emit(BufferOccupancySignal, (unsigned long)bufferedPcks);
    const auto& phyHeader = pck->removeAtFront<EthernetPhyHeader>();
    const auto& macheader = pck->removeAtFront<EthernetMacHeader>();
    const auto& ipv4header = pck->peekAtFront<Ipv4Header>();
    EV<<"received packet destination address = "<<ipv4header->getDestAddress()<<endl;
    if (isSuspendState.find(ipv4header->getDestAddress()) == isSuspendState.end())
        isSuspendState[ipv4header->getDestAddress()] = true;

    //cPacketQueue packetQueue = it->second;
    pck->insertAtFront(macheader);
    pck->insertAtFront(phyHeader);
    if (isSuspendState.find(ipv4header->getDestAddress())->second)
    {
        if (bufferQueues.find(ipv4header->getDestAddress()) == bufferQueues.end())
            bufferQueues[ipv4header->getDestAddress()] = cPacketQueue();
        cPacketQueue packetQueue = bufferQueues.find(ipv4header->getDestAddress())->second;
        packetQueue.insert(pck);
        bufferQueues[ipv4header->getDestAddress()] = packetQueue;
    }
    else
    {
        txPcks.insert(pck);
    }
    EV<<"bufferQueues size = "<<bufferQueues.size()<<endl;
}

void MidWare::processReceivedSuspend(Packet *pck)
{
    const auto& ipv4header = pck->peekAtFront<Ipv4Header>();
    // suspend maybe received earlier than any other packets
    if (bufferQueues.find(ipv4header->getDestAddress()) == bufferQueues.end())
        bufferQueues[ipv4header->getDestAddress()] = cPacketQueue();

    if (isSuspendState.find(ipv4header->getDestAddress()) == isSuspendState.end())
        isSuspendState[ipv4header->getDestAddress()] = true;

    if (pck->getPacketECN() == 0)
    {
        isSuspendState[ipv4header->getDestAddress()] = false;
        EV<<"received suspend source IF address = "<<ipv4header->getDestAddress()<<endl;
        cPacketQueue packetQueue = bufferQueues.find(ipv4header->getDestAddress())->second;
        while (packetQueue.getLength() != 0)
        {
            txPcks.insert(packetQueue.pop());
        }
        bufferQueues.erase(ipv4header->getDestAddress());
        EV<<"bufferQueues size = "<<bufferQueues.size()<<endl;
        if (txPcks.getLength() != 0)
        {
            if(!senddata->isScheduled())
            {
                scheduleAt(simTime(),senddata);
            }
        }
    }
    else if (pck->getPacketECN() == 1)
    {
        isSuspendState[ipv4header->getDestAddress()] = true;
    }
    else
        throw cRuntimeError("unspecified suspend type!");
    delete pck;
}

void MidWare::sendOut(Packet *pck)
{
    EV<<"sendOut packet "<<pck->getFullName()<<endl;
    send(pck,OutGate);
}

void MidWare::finish()
{
    bufferQueues.clear();
    isSuspendState.clear();
    cancelAndDelete(senddata);
    cancelAndDelete(stopdata);
    cancelAndDelete(startMsg);
}

void MidWare::sendInterfaceRegister()
{
    Packet *IFR = new Packet("IfRegisterSignal");
    const auto& ipv4Header = makeShared<Ipv4Header>();
    ipv4Header->setCrcMode(crcMode);
    ipv4Header->setCrc(0);
    switch (crcMode) {
        case CRC_DECLARED_CORRECT:
            // if the CRC mode is declared to be correct, then set the CRC to an easily recognizable value
            ipv4Header->setCrc(0xC00D);
            break;
        case CRC_DECLARED_INCORRECT:
            // if the CRC mode is declared to be incorrect, then set the CRC to an easily recognizable value
            ipv4Header->setCrc(0xBAAD);
            break;
        case CRC_COMPUTED: {
            ipv4Header->setCrc(0);
            // crc will be calculated in fragmentAndSend()
            break;
        }
        default:
            throw cRuntimeError("Unknown CRC mode");
    }
    ipv4Header->setChunkLength(b(64));
    IFR->insertAtFront(ipv4Header);
    send(IFR,OutGate);
}

void MidWare::sendRegistedPcks()
{
    if (txPcks.getLength() != 0)
    {
        Packet * firstPck = check_and_cast<Packet*>(txPcks.pop());
        simtime_t txDuration = double(firstPck->getBitLength())/LinkSpeed;

        bufferedPcks -= firstPck->getByteLength();
        emit(BufferOccupancySignal, (unsigned long)bufferedPcks);
        EV<<"sendRegistedPcks(), the packet bit length = "<<firstPck->getBitLength()<<" bits, txDuration = "<<txDuration<<endl;
        auto signal = new EthernetSignal(firstPck->getFullName());
        signal->encapsulate(firstPck);
        //bufferedPcks--;
        send(signal,OutGate);
        scheduleAt(transmissionChannel->getTransmissionFinishTime(),senddata);
    }
    else
        scheduleAt(simTime(),stopdata);
}
} // namespace inet


