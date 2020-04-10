// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 2020/03/02
*/
#include "dcqcn.h"

namespace inet {

Define_Module(dcqcn);

simsignal_t dcqcn::queueLengthSignal = registerSignal("queueLength");
simsignal_t dcqcn::txRateSignal = registerSignal("txRate");
simsignal_t dcqcn::cnpReceivedSignal = registerSignal("cnpReceived");

void dcqcn::initialize()
{
    //statistics
    emit(txRateSignal, 0);
    emit(cnpReceivedSignal,0);
    lowerOutGate = gate("lowerOut");
    lowerInGate = gate("lowerIn");
    upperOutGate = gate("upperOut");
    upperInGate = gate("upperIn");
    // configuration
    frameCapacity = par("frameCapacity");
    sumpaths = par("sumpaths");
    activate = par("activate");
    gamma = par("gamma");
    baseRTT = par("baseRTT");
    linkspeed = par("linkspeed");
    min_cnp_interval = par("min_cnp_interval");
    AlphaTimer_th = par("AlphaTimer_th");
    RateTimer_th = par("RateTimer_th");
    ByteCounter_th = par("ByteCounter_th");
    frSteps_th = par("frSteps_th");
    Rai = par("Rai");
    Rhai = par("Rhai");
    linkspeed *= 1e9;
    Rai *= 1e6;
    Rhai *= 1e6;
    min_cnp_interval *= 1e-6;
    AlphaTimer_th *= 1e-6;
    RateTimer_th *= 1e-6;

    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

    max_pck_size = 1456; // B

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

void dcqcn::handleMessage(cMessage *msg)
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

void dcqcn::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    EV_TRACE << "Self-message " << timer << " received, type = "<<timer->getKind()<<endl;
    switch (timer->getKind()) {
        case SENDDATA:
        {
            send_data(timer->getFlowId());
            break;
        }
        case STOPDATA:
        {
            break;
        }
        case RATETIMER:
        {
            sender_flowinfo sndinfo = sender_flowMap.find(timer->getFlowId())->second;
            sndinfo.TimeFrSteps++;
            sender_flowMap[timer->getFlowId()] = sndinfo;
            increaseTxRate(timer->getFlowId());
            scheduleAt(simTime()+RateTimer_th,sndinfo.rateTimer);
            break;
        }
        case ALPHATIMER:
        {
            updateAlpha(timer->getFlowId());
            break;
        }
        case CNPTIMER:
        {
            receiver_flowinfo rcvinfo = receiver_flowMap.find(timer->getFlowId())->second;
            if (rcvinfo.ecnPakcetReceived)
            {
                send_cnp(timer->getFlowId());
                rcvinfo.ecnPakcetReceived = false;
                receiver_flowMap[timer->getFlowId()] = rcvinfo;
            }
            else
            {
                cancelEvent(rcvinfo.cnpTimer);
            }
            break;
        }
    }
}

void dcqcn::refreshDisplay() const
{

}

// Record the packet from app to transmit it to the dest
void dcqcn::processUpperpck(Packet *pck)
{
    sender_flowinfo snd_info;

    auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
    L3Address srcAddr = addressReq->getSrcAddress();
    L3Address destAddr = addressReq->getDestAddress();

    auto udpHeader = pck->removeAtFront<UdpHeader>();
    auto payload = pck->peekAtBack<ApplicationPacket>();

    snd_info.remainLength = pck->getByteLength();
    snd_info.srcAddr = srcAddr;
    snd_info.destAddr = destAddr;
    snd_info.srcPort = udpHeader->getSrcPort();
    snd_info.destPort = udpHeader->getDestPort();
    snd_info.flowid = pck->getFlowId();
    snd_info.pckseq = 0;
    snd_info.priority = pck->getPriority();
    snd_info.crcMode = udpHeader->getCrcMode();
    snd_info.crc = udpHeader->getCrc();
    snd_info.currentRate = linkspeed;
    snd_info.targetRate = linkspeed;
    for (auto& region : pck->peekData()->getAllTags<CreationTimeTag>())
        snd_info.creaTime = region.getTag()->getCreationTime();
    // schedule the nxt tx packet time
    snd_info.nxtSendTime = simTime();

    snd_info.SenderState = Normal;
    snd_info.alpha = 1;
    snd_info.ByteCounter = 0;
    snd_info.LastRateTimer = 0;
    snd_info.ByteFrSteps = 0;
    snd_info.TimeFrSteps = 0;
    snd_info.LastAlphaTimer = 0;

    // send packet timer
    snd_info.senddata = new TimerMsg("senddata");
    snd_info.senddata->setKind(SENDDATA);
    snd_info.senddata->setDestAddr(destAddr);
    snd_info.senddata->setFlowId(pck->getFlowId());
    // stop sending timer
    snd_info.stopdata = new TimerMsg("stopdata");
    snd_info.stopdata->setKind(STOPDATA);
    snd_info.stopdata->setDestAddr(destAddr);
    snd_info.stopdata->setFlowId(pck->getFlowId());
    // rate increasing timer
    snd_info.rateTimer = new TimerMsg("rateTimer");
    snd_info.rateTimer->setKind(RATETIMER);
    snd_info.rateTimer->setDestAddr(destAddr);
    snd_info.rateTimer->setFlowId(pck->getFlowId());
    // alpha updating timer
    snd_info.alphaTimer = new TimerMsg("alphaTimer");
    snd_info.alphaTimer->setKind(ALPHATIMER);
    snd_info.alphaTimer->setDestAddr(destAddr);
    snd_info.alphaTimer->setFlowId(pck->getFlowId());
    sender_flowMap[pck->getFlowId()] =snd_info;
    scheduleAt(simTime(),snd_info.senddata);

    updateAllTxRate();

    delete pck;
}

void dcqcn::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"CNP"))
    {
        receive_cnp(pck);
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

void dcqcn::send_data(uint32_t flowid)
{
    //int64_t this_pck_bytes = 0;

    sender_flowinfo snd_info = sender_flowMap.find(flowid)->second;
    std::ostringstream str;
    str << packetName << "-" <<snd_info.flowid<< "-" <<snd_info.pckseq;
    Packet *packet = new Packet(str.str().c_str());
    const auto& payload = makeShared<ApplicationPacket>();
    payload->setSequenceNumber(snd_info.pckseq);
    payload->setFlowid(snd_info.flowid);
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
    auto creationTimeTag = payload->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(snd_info.creaTime);

    packet->insertAtBack(payload);

    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(snd_info.srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    // insert udpHeader, set source and destination port
    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(snd_info.srcPort);
    udpHeader->setDestinationPort(snd_info.destPort);
    udpHeader->setCrc(snd_info.crc);
    udpHeader->setCrcMode(snd_info.crcMode);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);

    // insert dcqcn header
    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(B(16));
    content->setIdentification(snd_info.pckseq);
    packet->insertAtFront(content);
    packet->setPriority(snd_info.priority);
    packet->setPathID(intrand(8));
    packet->setPacketECN(1); // enable ecn
    packet->setTimestamp(simTime());
    packet->setFlowId(flowid);
    snd_info.pckseq += 1;

    EV<<"packet length = "<<packet->getByteLength()<<", current rate = "<<snd_info.currentRate<<endl;
    snd_info.nxtSendTime = double((packet->getByteLength()+70)*8)/snd_info.currentRate + simTime();
    if (snd_info.SenderState != Normal)
    {
        snd_info.ByteCounter += packet->getByteLength();

        if (snd_info.ByteCounter >= ByteCounter_th)
        {
            snd_info.ByteFrSteps++;
            snd_info.ByteCounter = 0;
            sender_flowMap[flowid] =snd_info;
            EV<<"byte counter expired, byte fr steps = "<<snd_info.ByteFrSteps<<endl;
            scheduleAt(snd_info.nxtSendTime,snd_info.senddata);
            increaseTxRate(flowid);
        }
        else
        {
            sender_flowMap[flowid] =snd_info;
            scheduleAt(snd_info.nxtSendTime,snd_info.senddata);
        }
    }
    else
    {
        sender_flowMap[flowid] =snd_info;
        scheduleAt(snd_info.nxtSendTime,snd_info.senddata);
    }
    EV << "prepare to send packet, remaining data size = " << snd_info.remainLength <<endl;
    sendDown(packet);
    // if no flow exits to be transmitted, stop transmitting.
    if (snd_info.remainLength == 0)
    {
        cancelEvent(snd_info.senddata);
        cancelEvent(snd_info.alphaTimer);
        cancelEvent(snd_info.rateTimer);
        delete snd_info.senddata;
        delete snd_info.alphaTimer;
        delete snd_info.rateTimer;
        sender_flowMap.erase(flowid);
    }
}

void dcqcn::send_cnp(uint32_t flowid)
{
    receiver_flowinfo rcvinfo = receiver_flowMap.find(flowid)->second;
    Packet *cnp = new Packet("CNP");
    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(B(16));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(crcNumber);
    content->setIdentification(rcvinfo.cnpSequence);
    cnp->insertAtFront(content);
    cnp->setPathID(intrand(8));
    cnp->setTimestamp(simTime());
    cnp->setPriority(99);
    cnp->setFlowId(flowid);
    cnp->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcvinfo.Sender_srcAddr);
    cnp->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcvinfo.Sender_destAddr);
    cnp->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cnp->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
    EV_INFO << "Sending CNP packet to "<<flowid <<", cnp interval = "<<simTime()-rcvinfo.lastCnpTime<<endl;
    rcvinfo.lastCnpTime = simTime();
    rcvinfo.cnpSequence++;
    receiver_flowMap[flowid] = rcvinfo;

    sendDown(cnp);
    scheduleAt(simTime() + min_cnp_interval, rcvinfo.cnpTimer);
}

/*
* When an RP (i.e. the flow sender) gets a CNP,
*  it reduces its currentRate and updates the value
*  of the rate reduction factor alpha,like DCTCP, and remembers
*  currentRate as targetRate for later recovery. The values
*  are updated as follows:
*  targetRate = currentRate,
*  currentRate = currentRate * (1 − alpha/2),
*  alpha= (1− gamma) * alpha + gamma,
*
*  Initial value of alpha is 1, gamma = default(1/256) is defined by manager
*/
void dcqcn::receive_cnp(Packet *pck)
{
    L3Address destAddr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    if (sender_flowMap.find(pck->getFlowId()) != sender_flowMap.end())
    {
        emit(cnpReceivedSignal,1);
        sender_flowinfo sndinfo = sender_flowMap.find(pck->getFlowId())->second;
        // cut rate
        sndinfo.targetRate = sndinfo.currentRate;
        sndinfo.currentRate = sndinfo.currentRate * (1 - sndinfo.alpha/2);
        sndinfo.alpha = (1 - gamma) * sndinfo.alpha + gamma;
        EV<<"after cutting, the current rate = "<<sndinfo.currentRate<<
                ", target rate = "<<sndinfo.targetRate<<endl;
        emit(txRateSignal, sndinfo.currentRate);

        // reset timers and counter
        //sndinfo.alpha = 1;
        sndinfo.ByteCounter = 0;
        sndinfo.LastRateTimer = 0;
        sndinfo.ByteFrSteps = 0;
        sndinfo.TimeFrSteps = 0;
        sndinfo.LastAlphaTimer = 0;
        sndinfo.iRhai = 0;
        cancelEvent(sndinfo.alphaTimer);
        cancelEvent(sndinfo.rateTimer);

        sender_flowMap[pck->getFlowId()] = sndinfo;

        // update alpha
        scheduleAt(simTime()+AlphaTimer_th,sndinfo.alphaTimer);

        // schedule to rate increase event
        scheduleAt(simTime()+RateTimer_th,sndinfo.rateTimer);
    }
    else
    {
    }
}

void dcqcn::receive_data(Packet *pck)
{
    const auto& data_head = pck->removeAtFront<Ipv4Header>();
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();
    auto destAddr = l3AddressInd->getDestAddress();
    receiver_flowinfo rcvinfo;

    if(receiver_flowMap.find(pck->getFlowId()) == receiver_flowMap.end())
    {
        rcvinfo.lastCnpTime = 0;
        rcvinfo.nowHTT = simTime() - pck->getTimestamp();
        rcvinfo.ecnPakcetReceived = false;
        rcvinfo.cnpSequence = 0;
        rcvinfo.Sender_srcAddr = srcAddr;
        rcvinfo.Sender_destAddr = destAddr;
        rcvinfo.cnpTimer = new TimerMsg("cnpTimer");
        rcvinfo.cnpTimer->setKind(CNPTIMER);
        rcvinfo.cnpTimer->setDestAddr(srcAddr);
        rcvinfo.cnpTimer->setFlowId(pck->getFlowId());

        receiver_flowMap[pck->getFlowId()] = rcvinfo;
    }
    else
    {
        rcvinfo = receiver_flowMap.find(pck->getFlowId())->second;
        rcvinfo.nowHTT = simTime() - pck->getTimestamp();
        receiver_flowMap[pck->getFlowId()] = rcvinfo;
    }
    if (pck->getPacketECN() == 3) // ecn==1, enabled; ecn==3, marked.
    {
        if (rcvinfo.cnpTimer->isScheduled())
        {
            rcvinfo.ecnPakcetReceived = true;
            receiver_flowMap[pck->getFlowId()] = rcvinfo;
        }
        else
        {
            send_cnp(pck->getFlowId());
        }
    }
    EV<<"receive packet, sequence number = "<<data_head->getIdentification()<<", ecn = "<<pck->getPacketECN()<<endl;
    sendUp(pck);
}

void dcqcn::updateAllTxRate()
{
    double flow_amount = sender_flowMap.size();
    auto it = sender_flowMap.begin();
    for(; it!=sender_flowMap.end(); ++ it)
    {
        sender_flowinfo flowinfo = it->second;
        flowinfo.maxTxRate = linkspeed/flow_amount;
        EV<<"updateTxRate(), before update, txRate = "<<flowinfo.currentRate<<", nxtSendTime = "<<
                flowinfo.nxtSendTime<<", max TX rate = "<<flowinfo.maxTxRate<<endl;
        if (flowinfo.currentRate > flowinfo.maxTxRate)
        {
            flowinfo.nxtSendTime = simTime() + (flowinfo.nxtSendTime - simTime()) *
                    flowinfo.currentRate/flowinfo.maxTxRate; // update nxt sending time
            flowinfo.currentRate = flowinfo.maxTxRate;   // update current TX rate
            sender_flowMap[it->first] = flowinfo;
            cancelEvent(flowinfo.senddata);
            scheduleAt(flowinfo.nxtSendTime,flowinfo.senddata);
        }
        EV<<"updateTxRate(), after update, currentRate = "<<flowinfo.currentRate<<", nxtSendTime = "<<
                flowinfo.nxtSendTime<<endl;
        sender_flowMap[it->first] = flowinfo;
    }
}

void dcqcn::increaseTxRate(uint32_t flowid)
{
    sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;
    double oldrate = sndinfo.currentRate;

    if (max(sndinfo.ByteFrSteps,sndinfo.TimeFrSteps) < frSteps_th)
    {
        sndinfo.SenderState = Fast_Recovery;
    }
    else if (min(sndinfo.ByteFrSteps,sndinfo.TimeFrSteps) > frSteps_th)
    {
        sndinfo.SenderState = Hyper_Increase;
    }
    else
    {
        sndinfo.SenderState = Additive_Increase;
    }
    EV<<"entering incease rate, sender state = "<<sndinfo.SenderState<<endl;

    if (sndinfo.SenderState == Fast_Recovery)
    {// Fast Recovery
        sndinfo.currentRate = (sndinfo.currentRate + sndinfo.targetRate) / 2;
    }
    else if (sndinfo.SenderState == Additive_Increase)
    {// Additive Increase
        sndinfo.targetRate += Rai;
        sndinfo.targetRate = (sndinfo.targetRate > sndinfo.maxTxRate) ? sndinfo.maxTxRate : sndinfo.targetRate;

        sndinfo.currentRate = (sndinfo.currentRate + sndinfo.targetRate) / 2;
    }
    else if (sndinfo.SenderState == Hyper_Increase)
    {// Hyper Increase
        sndinfo.iRhai++;
        sndinfo.targetRate += sndinfo.iRhai*Rhai;
        sndinfo.targetRate = (sndinfo.targetRate > sndinfo.maxTxRate) ? sndinfo.maxTxRate : sndinfo.targetRate;
        sndinfo.currentRate = (sndinfo.currentRate + sndinfo.targetRate) / 2;
    }
    else
    {// Normal state

    }
    EV<<"after increasing, the current rate = "<<sndinfo.currentRate<<
            ", target rate = "<<sndinfo.targetRate<<endl;

    emit(txRateSignal, sndinfo.currentRate);
    sndinfo.nxtSendTime = simTime() + (sndinfo.nxtSendTime - simTime()) *
            oldrate/sndinfo.currentRate; // update nxt sending time
    sender_flowMap[flowid] = sndinfo;
    cancelEvent(sndinfo.senddata);
    scheduleAt(sndinfo.nxtSendTime,sndinfo.senddata);
}

void dcqcn::updateAlpha(uint32_t flowid)
{

    sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;
    sndinfo.alpha = (1 - gamma) * sndinfo.alpha;
    scheduleAt(simTime() + AlphaTimer_th, sndinfo.alphaTimer);
    sender_flowMap[flowid] = sndinfo;
}

bool dcqcn::orderpackets(uint32_t flowid)
{
    return false;
}

void dcqcn::sendDown(Packet *pck)
{
    EV << "sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void dcqcn::sendUp(Packet *pck)
{
    EV<<"dcqcn, oh sendup!"<<endl;
    send(pck,upperOutGate);
}

void dcqcn::finish()
{

}

} // namespace inet
