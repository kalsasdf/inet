// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 05/09/2019
*/

#include "ans.h"

namespace inet {

Define_Module(ans);

simsignal_t ans::queueLengthSignal = registerSignal("queueLength");

void ans::initialize()
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
    linkspeed = linkspeed * 1e9;
    baseRTT = par("baseRTT");
    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

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
    registerService(Protocol::udp, gate("upperIn"), gate("lowerIn"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("upperOut"));
}

void ans::handleMessage(cMessage *msg)
{
    if(activate)
    {
        numMapReceived++;
        if (msg->isSelfMessage())
            handleSelfMessage(msg);// sendgrant
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

void ans::refreshDisplay() const
{

}

void ans::processUpperpck(Packet *pck)
{
    sender_flowinfo snd_info;

    auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
    L3Address srcAddr = addressReq->getSrcAddress();
    L3Address destAddr = addressReq->getDestAddress();

    auto udpHeader = pck->removeAtFront<UdpHeader>();
    auto payload = pck->peekAtBack<ApplicationPacket>();

    snd_info.RTTbytes = int64_t(baseRTT.dbl()*linkspeed/8);
    if (pck->getByteLength()>snd_info.RTTbytes)
    {
        snd_info.remainLength = pck->getByteLength()-snd_info.RTTbytes;
    }
    else
    {
        snd_info.RTTbytes = pck->getByteLength();
        snd_info.remainLength = 0;
    }
    EV<<"snd_info.RTTbytes = "<<snd_info.RTTbytes<<", remain length = "<<snd_info.remainLength<<endl;
    snd_info.srcAddr = srcAddr;
    snd_info.destAddr = destAddr;
    snd_info.srcPort = udpHeader->getSrcPort();
    snd_info.destPort = udpHeader->getDestPort();
    snd_info.flowid = pck->getFlowId();
    snd_info.pckseq = 0;
    snd_info.crcMode = udpHeader->getCrcMode();
    snd_info.crc = udpHeader->getCrc();
    for (auto& region : pck->peekData()->getAllTags<CreationTimeTag>())
        snd_info.creaTime = region.getTag()->getCreationTime();


    // send packet timer
    snd_info.senddata = new TimerMsg("senddata");
    snd_info.senddata->setKind(SENDDATA);
    snd_info.senddata->setDestAddr(destAddr);
    snd_info.senddata->setFlowId(pck->getFlowId());

    sender_flowMap[pck->getFlowId()] =snd_info;
    next_destAddr = destAddr;

    scheduleAt(simTime(),snd_info.senddata);
    //send_data(destAddr);
    delete pck;
}

void ans::handleSelfMessage(cMessage *msg)
{
    // Process different self-messages (timer signals)
    TimerMsg *timer = check_and_cast<TimerMsg *>(msg);
    EV_TRACE << "Self-message " << timer << " received\n";

    switch (timer->getKind()) {
        case SENDDATA:
            send_data(timer->getFlowId());
            break;
    }
}

void ans::send_data(uint32_t flowid)
{
    bool in_first_rtt = false;
    int64_t this_pck_bytes = 0;
    uint16_t ansprio = 0;

    sender_flowinfo snd_info = sender_flowMap.find(flowid)->second;
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
        EV_INFO << "Sending ans packet " << packet->getName() << " in first rtt, remaining bytes = "<< snd_info.RTTbytes <<", ansprio = "<<ansprio<<".\n";
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

    // insert ans header
    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(B(16));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(crcNumber);
    content->setIdentification(snd_info.pckseq);
    packet->insertAtFront(content);
    packet->setSchedulingPriority(next_schePrio);
    packet->setFlowId(flowid);


    sender_flowMap[flowid] =snd_info;

    sendDown(packet);
    if (in_first_rtt)
    {
        if (snd_info.RTTbytes > 0)
        {
            scheduleAt(simTime() + double(max_pck_size*8)/linkspeed,snd_info.senddata);
        }
    }
    else
    {
        EV_INFO << "Sending ans packet " << packet->getName() << " scheduled by grant.\n";
    }
}

void ans::processLowerpck(Packet *pck)
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

void ans::sendDown(Packet *pck)
{
    EV << "sendDown(), remaining data size = " << sender_flowMap.find(pck->getFlowId())->second.remainLength <<endl;
    send(pck,lowerOutGate);
}

void ans::sendUp(Packet *pck)
{
    EV<<"ans, oh sendup!"<<endl;
    send(pck,upperOutGate);
}

void ans::send_grant(uint32_t flowid,unsigned int pathid, int seq)
{
    Packet *grant = new Packet("Grant");

    receiver_flowinfo rcvinfo = receiver_flowMap.find(flowid)->second;

    const auto& content = makeShared<Ipv4Header>();

    content->setChunkLength(B(16));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(crcNumber);
    content->setIdentification(seq);

    grant->insertAtFront(content);

    grant->setSchedulingPriority(pathid);
    grant->setTimestamp(simTime());
    grant->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcvinfo.Sender_srcAddr);
    grant->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcvinfo.Sender_destAddr);
    grant->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x2E);
    grant->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    grant->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
    grant->setSchedulingPriority(1);//TODO: adaptive adjusted priority
    grant->setFlowId(flowid);

    EV_INFO << "Sending grant packet to"<< rcvinfo.Sender_srcAddr <<".\n";
    rcvinfo.grantSequence++;

    receiver_flowMap[flowid] = rcvinfo;

    sendDown(grant);
}

void ans::send_resend(uint32_t flowid)
{

}

void ans::send_busy(uint32_t addr)
{

}

void ans::receive_grant(Packet *pck)
{
    sender_flowinfo sndinfo = sender_flowMap.find(pck->getFlowId())->second;
    if (sndinfo.remainLength > 0)
    {
        scheduleAt(simTime(),sndinfo.senddata);
    }
    else
    {
    }
}

void ans::receive_resend(Packet *pck)
{

}

void ans::receive_busy(Packet *pck)
{

}

void ans::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();
    auto destAddr = l3AddressInd->getDestAddress();
    const auto& data_head = pck->removeAtFront<Ipv4Header>();
    receiver_flowinfo rcvinfo;
    if(receiver_flowMap.find(pck->getFlowId()) == receiver_flowMap.end())
    {
        rcvinfo.nowHTT = simTime() - pck->getTimestamp();
        rcvinfo.grantSequence = 0;
        rcvinfo.Sender_srcAddr = srcAddr;
        rcvinfo.Sender_destAddr = destAddr;

        receiver_flowMap[pck->getFlowId()] = rcvinfo;
    }
    else
    {
        rcvinfo = receiver_flowMap.find(pck->getFlowId())->second;
        rcvinfo.nowHTT = simTime() - pck->getTimestamp();
        receiver_flowMap[pck->getFlowId()] = rcvinfo;
    }
    send_grant(pck->getFlowId(),1,data_head->getIdentification());
    sendUp(pck);
}

void ans::schedule_next_grant(simtime_t delta_t)
{

}

void ans::find_nextaddr()
{

}

bool ans::orderpackets(uint32_t flowid)
{
    return false;
}

void ans::finish()
{

}

} // namespace inet


