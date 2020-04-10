// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#include "homa.h"

namespace inet {

Define_Module(homa);

simsignal_t homa::queueLengthSignal = registerSignal("queueLength");

void homa::initialize()
{
    //statistics
    emit(queueLengthSignal, int(data_Map.size()));
    lowerOutGate = gate("lowerOut");
    lowerInGate = gate("lowerIn");
    upperOutGate = gate("upperOut");
    upperInGate = gate("upperIn");
    // configuration
    frameCapacity = par("frameCapacity");
    sumpaths = par("sumpaths");
    activate = par("activate");
    linkspeed = par("linkspeed");
    linkspeed = linkspeed * 1000000000;
    baseRTT = par("baseRTT");
    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

    senddata = new cMessage("senddata", SENDDATA);
    stopdata = new cMessage("stopdata", STOPCRED);
    max_pck_size = 1456;

    // define crcMode
    const char *crcModeString = par("crcMode");
    if (!strcmp(crcModeString, "declared"))
    {
        crcMode = CRC_DECLARED_CORRECT;
        crcNumber = 0xC00D;
    }
    else if (!strcmp(crcModeString, "computed"))
    {
        crcMode = CRC_COMPUTED;
        crcNumber = 0;
    }
    else
        throw cRuntimeError("Unknown crc mode: '%s'", crcModeString);

    setPrioCutOffs(); // set priority cutoffs for unscheduled packets

    registerService(Protocol::udp, gate("upperIn"), gate("lowerIn"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("upperOut"));
}

void homa::handleMessage(cMessage *msg)
{
    if(activate)
    {
        numMapReceived++;
        if (msg->isSelfMessage())
            handleSelfMessage(msg);// sendcredit
        else
        {
            if (msg->arrivedOn("upperIn"))
            {
                if (string(msg->getFullName()).find("Data") != string::npos)
                    processUpperpck(check_and_cast<Packet*>(msg));
                else
                    send(msg,lowerOutGate);
            }
            else if (msg->arrivedOn("lowerIn"))
            {
                processLowerpck(check_and_cast<Packet*>(msg));
            }
        }
    }
    else
    {
        if (msg->arrivedOn("upperIn"))
        {
            send(msg,lowerOutGate);
        }
        else if (msg->arrivedOn("lowerIn"))
        {
            send(msg,upperOutGate);
        }
    }
}

void homa::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << pck << " received\n";

    switch (pck->getKind()) {
        case SENDDATA:
            send_data(next_destAddr);
            EV<<"nextAddrQueue size = "<<nextAddrQueue.size()<<endl;
            //nextAddrQueue.pop();
            EV<<"nextAddrQueue size = "<<nextAddrQueue.size()<<endl;
            break;
        case STOPCRED:
            break;
    }
}

void homa::refreshDisplay() const
{

}

void homa::processUpperpck(Packet *pck)
{
    sender_flowinfo snd_info;

    auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
    L3Address srcAddr = addressReq->getSrcAddress();
    L3Address destAddr = addressReq->getDestAddress();

    auto udpHeader = pck->removeAtFront<UdpHeader>();
    auto payload = pck->peekAtBack<ApplicationPacket>();

    snd_info.RTTbytes = int64_t(baseRTT.dbl()*linkspeed/8);
    EV<<"snd_info.RTTbytes = "<<snd_info.RTTbytes<<endl;
    if (pck->getByteLength()>snd_info.RTTbytes)
    {
        snd_info.remainLength = pck->getByteLength()-snd_info.RTTbytes;
    }
    else
    {
        snd_info.RTTbytes = pck->getByteLength();
        snd_info.remainLength = 0;
    }
    snd_info.srcAddr = srcAddr;
    snd_info.destAddr = destAddr;
    snd_info.srcPort = udpHeader->getSrcPort();
    snd_info.destPort = udpHeader->getDestPort();
    snd_info.flowid = payload->getFlowid();
    snd_info.pckseq = 0;
    snd_info.crcMode = udpHeader->getCrcMode();
    snd_info.crc = udpHeader->getCrc();
    snd_info.unscheduledPrio = getMesgPrio(snd_info.RTTbytes);
    for (auto& region : pck->peekData()->getAllTags<CreationTimeTag>())
        snd_info.creaTime = region.getTag()->getCreationTime();

    sender_flowMap[addressReq->getDestAddress()] =snd_info;
    next_destAddr = destAddr;

    scheduleAt(simTime(),senddata);
    //send_data(destAddr);
    delete pck;
}

void homa::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"Grant"))
    {
        receive_grant(pck);
    }
    else if (!strcmp(pck->getFullName(),"Resend"))
    {
        receive_resend(pck);
    }
    else if (!strcmp(pck->getFullName(),"Busy"))
    {
        receive_busy(pck);
    }
    else if (string(pck->getFullName()).find("Data") != string::npos)
    {
        receive_data(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void homa::sendDown(Packet *pck)
{
    EV << "sendDown(), remaining data size = " << sender_flowMap.find(next_destAddr)->second.remainLength <<endl;
    send(pck,lowerOutGate);
}

void homa::sendUp(Packet *pck)
{
    EV<<"homa, oh sendup!"<<endl;
    send(pck,upperOutGate);
}

void homa::send_data(L3Address destAddr)
{
    bool in_first_rtt = false;
    int64_t this_pck_bytes = 0;
    uint16_t homaprio = 0;

    sender_flowinfo snd_info = sender_flowMap.find(destAddr)->second;
    std::ostringstream str;
    str << packetName << "-" <<snd_info.flowid<< "-" <<snd_info.pckseq;
    Packet *packet = new Packet(str.str().c_str());
    const auto& payload = makeShared<ApplicationPacket>();
    payload->setSequenceNumber(snd_info.pckseq);
    payload->setFlowid(snd_info.flowid);
    auto creationTimeTag = payload->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(snd_info.creaTime);

    if (snd_info.RTTbytes > 0) // TODO: the last packet of RTTbytes
    {
        in_first_rtt = true;
        this_pck_bytes = max_pck_size;
        if (snd_info.RTTbytes - this_pck_bytes < 0) // RTT bytes is smaller than the MTU size
        {
            if (snd_info.remainLength + snd_info.RTTbytes - this_pck_bytes < 0)
            {// sum of remain length and RTT bytes is smaller than the MTU size
                this_pck_bytes = snd_info.RTTbytes + snd_info.remainLength;
                snd_info.remainLength = 0;
            }
            else
            {
                snd_info.remainLength += snd_info.RTTbytes - this_pck_bytes;
            }
            snd_info.RTTbytes = 0;
        }
        else
        {
            snd_info.RTTbytes = snd_info.RTTbytes - this_pck_bytes;
        }
        payload->setChunkLength(B(this_pck_bytes));
        homaprio = snd_info.unscheduledPrio;
        EV_INFO << "Sending Homa packet " << packet->getName() << " in first rtt, remaining bytes = "<< snd_info.RTTbytes <<", homaprio = "<<homaprio<<".\n";
    }
    else
    {
        if (snd_info.remainLength > max_pck_size)
        {
            payload->setChunkLength(B(max_pck_size));
            snd_info.remainLength = snd_info.remainLength - max_pck_size;
        }
        else
        {
            payload->setChunkLength(B(snd_info.remainLength));
            snd_info.remainLength = 0;
        }
    }
    packet->insertAtBack(payload);

    snd_info.pckseq = snd_info.pckseq + 1;

    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(snd_info.srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    // set source and destination port
    udpHeader->setSourcePort(snd_info.srcPort);
    udpHeader->setDestinationPort(snd_info.destPort);
    udpHeader->setCrc(snd_info.crc);
    udpHeader->setCrcMode(snd_info.crcMode);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);

    // insert homa header
    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(B(16));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(crcNumber);
    content->setIdentification(snd_info.pckseq);
    packet->insertAtFront(content);
    packet->setSchedulingPriority(next_schePrio);


    sender_flowMap[destAddr] =snd_info;

    sendDown(packet);
    if (in_first_rtt)
    {
        if (snd_info.RTTbytes > 0)
        {
            scheduleAt(simTime() + double(max_pck_size*8)/linkspeed,senddata);
        }
    }
    else
    {
        EV_INFO << "Sending Homa packet " << packet->getName() << " scheduled by grant.\n";
    }
}

void homa::send_grant(L3Address destAddr,unsigned int pathid, int seq)
{
    Packet *grant = new Packet("Grant");

    short prio = 1;
    const auto& content = makeShared<Ipv4Header>();

    content->setChunkLength(B(16));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(crcNumber);
    content->setIdentification(seq);

    grant->insertAtFront(content);

    grant->setSchedulingPriority(pathid);
    grant->setTimestamp(simTime());
    grant->addTagIfAbsent<L3AddressReq>()->setDestAddress(destAddr);
    grant->addTagIfAbsent<L3AddressReq>()->setSrcAddress(receiver_srcAddr);
    grant->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x2E);
    grant->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    grant->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
    grant->setSchedulingPriority(1);//TODO: adaptive adjusted priority

    EV_INFO << "Sending GRANT packet to"<< destAddr <<".\n";

    sendDown(grant);
}

void homa::send_resend(L3Address destaddr)
{

}

void homa::send_busy(L3Address addr)
{

}

void homa::receive_grant(Packet *pck)
{
    L3Address destAddr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    if (sender_flowMap.find(destAddr)->second.remainLength > 0)
    {
        next_destAddr = destAddr;
        next_schePrio = pck->getSchedulingPriority();
        scheduleAt(simTime(),senddata);
    }
    else
    {
    }
}

void homa::receive_resend(Packet *pck)
{

}

void homa::receive_busy(Packet *pck)
{

}

void homa::receive_data(Packet *pck)
{
    const auto& data_head = pck->removeAtFront<Ipv4Header>();

    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();
    send_grant(srcAddr,1,data_head->getIdentification());
    sendUp(pck);
}

void homa::schedule_next_grant(simtime_t delta_t)
{

}

void homa::find_nextaddr()
{

}

bool homa::orderpackets(L3Address addr)
{
    return false;
}

void homa::finish()
{

}

uint16_t homa::getMesgPrio(uint32_t msgSize)
{
    size_t mid, high, low;
    low = 0;
    high = prioCutOffs.size() - 1;
    while(low < high) {
        mid = (high + low) / 2;
        if (msgSize <= prioCutOffs.at(mid)) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return high;
}

void homa::setPrioCutOffs()
{
    EV<<"push cutoffsize = "<<endl;
    prioCutOffs.clear();

    std::istringstream ss(
        par("explicitUnschedPrioCutoff").stdstringValue());
    uint32_t cutoffSize;
    while (ss >> cutoffSize) {
        eUnschedPrioCutoff.push_back(cutoffSize);
        EV<<"push cutoffsize = "<<cutoffSize<<endl;
    }

    prioCutOffs = eUnschedPrioCutoff;
    return;
}
} // namespace inet


