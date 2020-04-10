// Copyright (C) 2018 Angrydudu

#include "irnCore.h"
#include "inet/common/INETDefs.h"


namespace inet {

Define_Module(irnCore);

simsignal_t irnCore::queueLengthSignal = registerSignal("queueLength");

void irnCore::initialize()
{
    //statistics
    emit(queueLengthSignal, int(data_Map.size()));
    lowOutGate = gate("lowerOut");
    lowInGate = gate("lowerIn");
    upOutGate = gate("upperOut");
    upInGate = gate("upperIn");
    // configuration
    activate = par("activate");
    cacheCapacity = par("cacheCapacity");
    linkspeed = par("linkspeed");
    BDP_pcks = par("BDPpcks");
    BDP_N = par("BDP_N");

    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

    RTO_low = 300*pow(0.1,6); // 300us
    RTO_high = 1500*pow(0.1,6); // 1500us
    Capsize = 100;

    data_Map.clear();
    delayMap.clear();
    sender_QPMap.clear();
    receiver_QPMap.clear();
    BDP_Cap.clear();

    const char *crcModeString = par("crcMode");
    if (!strcmp(crcModeString, "declared"))
        crcMode = CRC_DECLARED_CORRECT;
    else if (!strcmp(crcModeString, "computed"))
        crcMode = CRC_COMPUTED;
    else
        throw cRuntimeError("Unknown crc mode: '%s'", crcModeString);

    registerService(Protocol::udp, gate("upperIn"), gate("lowerIn"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("upperOut"));

    start_RTO_Timer = new cMessage("start_RTO_Timer", reset_Timer);

    numMapReceived = 0;
    numMapDropped = 0;

    tsndInfo = {1,0,0,0,0,0,0,false,false,false,false};
    trcvInfo = {1,0,0,0};

    if (activate)
    {
        // statistics
        WATCH(numMapReceived);  // By using WATCH() function, we can see the value in the simulator.
        WATCH(numMapDropped);
    }
}

void irnCore::handleMessage(cMessage *msg)
{
    if(activate)
    {
        // Doing the following things when the IRN is working.
        // if the packet comes from the UDP model, do processUpperpck
        // otherwise comes from the ip model and do processLowerpck

        if (msg->isSelfMessage())
            handleSelfMessage(msg);// sendcredit
        else
        {
            if (msg->arrivedOn("upperIn"))
            {
                numMapReceived++;
                processUpperpck(check_and_cast<Packet*>(msg));
            }
            else if (msg->arrivedOn("lowerIn"))
            {
                processLowerpck(check_and_cast<Packet*>(msg));
            }
        }
    }
    else
    {
        // Doing the following things if the IRN is closed.

        if (msg->arrivedOn("upperIn"))
        {
            numMapReceived++;
            sendDown(check_and_cast<Packet*>(msg));
        }
        else if (msg->arrivedOn("lowerIn"))
        {
            sendUp(check_and_cast<Packet*>(msg));
        }
    }
}

void irnCore::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << pck << " received\n";
    auto sndQPInfo = sender_QPMap.find(DestAddr)->second;
    switch (pck->getKind()) {
        case reset_Timer:
        {
            if (inflight_Map.find(DestAddr)->second <= BDP_N)
            {
                // if have been in Recovery mode, and the packet have been resent.
                if (!sndQPInfo.inRecovery)
                {
                    sndQPInfo.inRecovery = true;
                    sndQPInfo.retransmitSN = sndQPInfo.lastAckedPsn + 1;
                    sndQPInfo.recoverSN = sndQPInfo.nextSNtoSend - 1;
                }
                sndQPInfo.doRetransmit = true;
                sndQPInfo.quickTimeout = false;
                sender_QPMap[DestAddr] = sndQPInfo;
                scheduleAt(simTime()+RTO_low,start_RTO_Timer);
                EV<<" handleSelfMessage(), in flight packets < BDP_N, use RTO_low, retransmit the seq "<<sndQPInfo.retransmitSN<<", restart the RTO timer."<<endl;
            }
            else if (sndQPInfo.quickTimeout == true)
            {
                // if have been in Recovery mode
                if (!sndQPInfo.inRecovery)
                {
                    sndQPInfo.inRecovery = true;
                    sndQPInfo.retransmitSN = sndQPInfo.lastAckedPsn + 1;
                    sndQPInfo.recoverSN = sndQPInfo.nextSNtoSend - 1;
                }
                sndQPInfo.doRetransmit = true;
                sndQPInfo.quickTimeout = false;
                sender_QPMap[DestAddr] = sndQPInfo;
                scheduleAt(simTime()+RTO_high,start_RTO_Timer);
                EV<<" handleSelfMessage(), in flight packets = "<< inflight_Map.find(DestAddr)->second <<", use RTO_high, retransmit the seq "<<sndQPInfo.retransmitSN<<endl;
            }
            else
            {
                sndQPInfo.quickTimeout = true;
                sender_QPMap[DestAddr] = sndQPInfo;
                scheduleAt(simTime()+(RTO_high - RTO_low),start_RTO_Timer);
                EV<<" handleSelfMessage(), in flight packets > BDP_N, change to RTO_high"<<endl;
            }

        }
            break;
    }
    if (sndQPInfo.doRetransmit)
    {
        if(BDP_Cap.find(sndQPInfo.retransmitSN) != BDP_Cap.end())
        {
            snd_sendData(BDP_Cap.find(sndQPInfo.retransmitSN)->second,true);
            snd_copytoBDPmap(sndQPInfo.retransmitSN,BDP_Cap.find(sndQPInfo.retransmitSN)->second);
            sndQPInfo.doRetransmit = false;
            sndQPInfo.findNewHole = false;
            EV<<"snd_receiveAck, retransmit pck, the sAck = "<< sndQPInfo.retransmitSN <<endl;
        }
    }
    sender_QPMap[DestAddr] = sndQPInfo;
}

void irnCore::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "q rcvd: %d\nq dropped: %d", numMapReceived, numMapDropped);
    getDisplayString().setTagArg("t", 0, buf);
}

// Process the message from lower layer.
void irnCore::processUpperpck(Packet *pck)
{
    // An IRN sender
    // transmits a new packet only if the number of packets in
    // flight (computed as the difference between current packets
    // sequence number and last acknowledged sequence number)
    // is less than this BDP cap.

    auto l3AddressReq = pck->addTagIfAbsent<L3AddressReq>();
    local_src = l3AddressReq->getSrcAddress();
    DestAddr = l3AddressReq->getDestAddress();
    if (string(pck->getFullName()).find("UDPBasicAppData") != string::npos)
    {
        if (inflight_Map.find(l3AddressReq->getDestAddress())==inflight_Map.end()){inflight_Map[l3AddressReq->getDestAddress()] = 0;}
        if (sender_QPMap.find(l3AddressReq->getDestAddress())==sender_QPMap.end()){sender_QPMap[l3AddressReq->getDestAddress()] = tsndInfo;}

        if (inflight_Map.find(l3AddressReq->getDestAddress())->second < BDP_pcks)
        {
            // if in loss recovery mode, directly insert the data into the Data map.
            if (sender_QPMap.find(l3AddressReq->getDestAddress())->second.inRecovery)
            {
                snd_insertData(l3AddressReq->getDestAddress(),pck);
            }
            // to send the current packet or send the extracted first packet in the map.
            else
            {
                if (data_Map.size() == 0)
                {
                    snd_sendData(pck,false);
                }
                else
                {
                    snd_sendData(snd_extractData(l3AddressReq->getDestAddress()),false);
                    snd_insertData(l3AddressReq->getDestAddress(),pck);
                }
            }
        }
        else
        {
            snd_insertData(l3AddressReq->getDestAddress(),pck);
        }
    }
    else
    {
        sendDown(pck);
    }
}

// Process the Packet from upper layer.
void irnCore::processLowerpck(Packet *pck)
{
    if (string(pck->getFullName()).find("UDPBasicAppData") != string::npos)
    {
        rcv_receiveData(pck);
    }
    else if (string(pck->getFullName()).find("ACK") != string::npos)
    {
        snd_receiveAck(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void irnCore::snd_sendData(Packet *pck, bool retransmit)
{
    if (!retransmit)
    {
        auto *l3addr = pck->addTagIfAbsent<L3AddressReq>();

        auto it = sender_QPMap.find(l3addr->getDestAddress());
        if (it==sender_QPMap.end())
        {
            sender_QPMap[l3addr->getDestAddress()] = tsndInfo;
        }
        snd_QPInfo snd_info = sender_QPMap.find(l3addr->getDestAddress())->second;


        EV<<"snd_sendData, the seq No. = "<<snd_info.nextSNtoSend<<endl;
        const auto& data_head = makeShared<Ipv4Header>();
        data_head->setChunkLength(b(32));
        data_head->setIdentification(snd_info.nextSNtoSend);
        data_head->setDiffServCodePoint(11);
        pck->insertAtFront(data_head);
        pck->setSchedulingPriority(1);
        pck->setTimestamp(simTime());

        snd_info.nextSNtoSend ++;
        sender_QPMap[l3addr->getDestAddress()] = snd_info;

        inflight_Map[l3addr->getDestAddress()] = inflight_Map.find(l3addr->getDestAddress())->second + 1;

        snd_copytoBDPmap(snd_info.nextSNtoSend-1,pck);
        sendDown(pck);
    }
    else
    {
        auto *l3addr = pck->addTagIfAbsent<L3AddressReq>();
        inflight_Map[l3addr->getDestAddress()] = inflight_Map.find(l3addr->getDestAddress())->second + 1;
        sendDown(pck);
    }

}

void irnCore::snd_receiveAck(Packet *pck)
{
    // getFragmentOffset() means cumulatively ack
    // Idetification means sackNo
    // dontfragment=true means nack=true

    // for the receiver:
    // Upon every out-of-order packet arrival, an IRN receiver
    // sends a NACK, which carries both the cumulative acknowledgment
    // (indicating its expected sequence number) and the
    // sequence number of the packet that triggered the NACK (as
    // a simplified form of selective acknowledgement or SACK).

    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    const auto& ArrivedAck = pck->removeAtFront<Ipv4Header>();
    int cumAck = ArrivedAck->getFragmentOffset();
    int sAck = ArrivedAck->getIdentification();
    bool NACK = ArrivedAck->getDontFragment();
    const L3Address &SrcAddr = l3addr->getSrcAddress();
    pck->insertAtFront(ArrivedAck);

    auto sndQPInfo = sender_QPMap.find(SrcAddr)->second;
    sndQPInfo.bits_to_shift = 0; // update upon every arrived ack
    if (sndQPInfo.inRecovery)
    {
        inflight_Map[SrcAddr] = inflight_Map.find(l3addr->getDestAddress())->second - 1;
    }
    else
    {
        inflight_Map[SrcAddr] = sndQPInfo.nextSNtoSend - sAck;
    }

    //update next sequence to send, if cumulative ack is higher.
    if(cumAck > sndQPInfo.nextSNtoSend)
    {
        sndQPInfo.nextSNtoSend = cumAck;
    }

    //check if any new packets are cumulatively acked and update last ack value and the bitmap.
    bool newAck = false;
    if(cumAck > sndQPInfo.lastAckedPsn)
    {
        newAck = true;
        sndQPInfo.sack_bitmap = sndQPInfo.sack_bitmap >> (cumAck - sndQPInfo.lastAckedPsn);
        sndQPInfo.lastAckedPsn = cumAck;
        sndQPInfo.retransmitSN = cumAck;
        EV<<"snd_receiveAck(), reset the RTO Timer"<<endl;
        cancelEvent(start_RTO_Timer);
        scheduleAt(simTime()+RTO_low,start_RTO_Timer);

    }


    EV<<"snd_receiveAck, the cumAck No. = "<<cumAck<<", sAck No. = "<<sAck<<", sAck bitmap = "<<sndQPInfo.sack_bitmap<<endl;
    EV<<"snd_receiveAck, the in flight packets = "<<inflight_Map.find(SrcAddr)->second<<", time stamp = "<<pck->getTimestamp()<<endl;

    //if a nack is received,
    if(NACK)
    {
        // first NACK received
        if (!sndQPInfo.inRecovery)
        {
            sndQPInfo.recoverSN = sndQPInfo.nextSNtoSend - 1;
            sndQPInfo.doRetransmit = true;
            sndQPInfo.inRecovery = true;
        }
        if (newAck)
        {
            sndQPInfo.doRetransmit = true;
        }
        EV<<"snd_retransmitbyNACK, newAck = "<<newAck<<", sAck No. = "<<sAck<<", recoverSN = "<<sndQPInfo.recoverSN<<endl;
        // update bitmap to set selective ack. for example: 0000 to 0010. 1 presents sAcked packet
        if (sAck >= sndQPInfo.lastAckedPsn)
        {
            uint64_t temp = 1;
            temp = temp << (sAck - sndQPInfo.lastAckedPsn);
            sndQPInfo.sack_bitmap = sndQPInfo.sack_bitmap | temp;
        }
        if(sndQPInfo.inRecovery)
        {
            EV<<"sndQPInfo.doRetransmit = "<<sndQPInfo.doRetransmit<<", sndQPInfo.retransmitSN = "<<sndQPInfo.retransmitSN<<endl;
            //if packet set for retransmission is selectively acked, don't retransmit it, and find new hole instead.
            if(sndQPInfo.retransmitSN >= sAck)
            {
                sndQPInfo.findNewHole = true;
                sndQPInfo.doRetransmit = false;
            }
        }
    }

    //when cumulative ack is greater than recovery sequence,
    //exit recovery and disable flags for retransmission and bitmap lookup.
    if (cumAck >= sndQPInfo.recoverSN)
    {
        sndQPInfo.inRecovery = false;
        sndQPInfo.findNewHole = false;
        sndQPInfo.doRetransmit = false;
    }

    EV<<"snd_receiveAck, findNewHole = "<<sndQPInfo.findNewHole<<endl;
    sender_QPMap[SrcAddr] = sndQPInfo;
    if (sndQPInfo.findNewHole)
    {
        snd_findNewHole(SrcAddr);
    }
    sndQPInfo = sender_QPMap.find(SrcAddr)->second;

    //if new packets have been acked,
    if (newAck)
    {
        if(sndQPInfo.inRecovery)
        {
            //partial ack: retransmit this packet only if it has not already been retransmitted.
            if((sndQPInfo.retransmitSN < sndQPInfo.lastAckedPsn) || ((sndQPInfo.retransmitSN == sndQPInfo.lastAckedPsn) && (sndQPInfo.doRetransmit)))
            {
                EV<<"1"<<endl;
                sndQPInfo.retransmitSN = sndQPInfo.lastAckedPsn;
                sndQPInfo.doRetransmit = true;
                sndQPInfo.findNewHole = false;
            }
        }
    }
    else
    {
        if(!sndQPInfo.inRecovery)
        {
            //first duplicated cumulative ack.
            //check if it is due to a valid lost packet.
            if ((sndQPInfo.lastAckedPsn < sndQPInfo.nextSNtoSend) && NACK)
            {
                sndQPInfo.retransmitSN = sndQPInfo.lastAckedPsn;
                sndQPInfo.doRetransmit = true;
                sndQPInfo.findNewHole = false;
                sndQPInfo.recoverSN = sndQPInfo.nextSNtoSend - 1;
                sndQPInfo.inRecovery = true;
            }
        }
    }

    if (sndQPInfo.sack_bitmap == 0 && sndQPInfo.inRecovery)
    {
        sndQPInfo.doRetransmit = true;
        sndQPInfo.retransmitSN = sndQPInfo.lastAckedPsn + 1;
        sndQPInfo.findNewHole = false;
        EV<< "snd_receiveAck, sack bitmap = 0, but still in fast recovery mode, some pcks have been discarded, to retranst the pck "<< sndQPInfo.retransmitSN <<endl;
    }

    if (sndQPInfo.doRetransmit)
    {
        if(BDP_Cap.find(sndQPInfo.retransmitSN) != BDP_Cap.end())
        {
            EV<<"snd_receiveAck, retransmit pck, the sAck = "<< sndQPInfo.retransmitSN <<endl;
            snd_sendData(BDP_Cap.find(sndQPInfo.retransmitSN)->second,true);
            snd_copytoBDPmap(sndQPInfo.retransmitSN,BDP_Cap.find(sndQPInfo.retransmitSN)->second);
            sndQPInfo.doRetransmit = false;
            sndQPInfo.findNewHole = false;
            //throw cRuntimeError(" Packet has been retransmitted.");
        }
    }
    sender_QPMap[SrcAddr] = sndQPInfo;
    delete pck;
}

void irnCore::snd_findNewHole(L3Address addr)
{
    auto sndQPInfo = sender_QPMap.find(addr)->second;
    int bits_to_shift = 0;
    uint64_t part = sndQPInfo.sack_bitmap;

    if((part & 0xffffffff) == 0xffffffff) {
        bits_to_shift += 32;
        part = part >> 32;
    }
    if((part & 0xffff) == 0xffff) {
        bits_to_shift += 16;
        part = part >> 16;
    }
    if((part & 0xff) == 0xff) {
        bits_to_shift += 8;
        part = part >> 8;
    }
    if((part & 0xf) == 0xf) {
        bits_to_shift += 4;
        part = part >> 4;
    }
    if((part & 3) == 3) {
        bits_to_shift += 2;
        part = part >> 2;
    }
    if((part & 1) == 1){
        bits_to_shift += 1;
        part = part >> 1;
    }
    if((part & 1) == 1){
        bits_to_shift += 1;
    }

    sndQPInfo.bits_to_shift = bits_to_shift;
    sndQPInfo.retransmitSN += bits_to_shift;
    if (bits_to_shift>0)
    {
    sndQPInfo.doRetransmit = true;
    }
    sender_QPMap[addr] = sndQPInfo;

    EV<<" snd_findNewHole, the sack_bitmap = "<< sndQPInfo.sack_bitmap<<", bits to shift = "<< bits_to_shift <<", new retransmitSN = "<<sndQPInfo.retransmitSN<<endl;
}

void irnCore::snd_decidebyRTOlow(Packet *pck)
{

}

void irnCore::snd_decidebyRTOhigh(Packet *pck)
{

}

Packet *irnCore::snd_findData(L3Address addr)
{
    auto it = data_Map.begin();
    for(; it != data_Map.end(); ++ it)
    {
        if (addr == it->first)
        {
            Packet *pck = it->second;
            return pck;
        }
    }
    EV << "Cannot find associated data packet."<<endl;
    return nullptr;
}

Packet *irnCore::snd_extractData(L3Address addr)
{
    auto it = data_Map.begin();
    for(; it != data_Map.end(); ++ it)
    {
        if (addr == it->first)
        {
            Packet *pck = it->second;
            data_Map.erase(it);
            return pck;
        }
    }
    EV << "No associated data packet to be extract in the Map."<<endl;
    return nullptr;
}

Packet *irnCore::snd_insertData(L3Address addr,Packet *pck)
{
    if (int(data_Map.size()) >= cacheCapacity)
    {
        //EV << "Map full, dropping packet.\n";
        delete pck;
        return nullptr;
    }
    else
    {
        data_Map.insert(pair<L3Address,Packet*>(addr,pck));
        emit(queueLengthSignal, int(data_Map.size()));
        //EV << "Data packet has been inserted into the map, map size = "<<data_Map.size()<<endl;
        return nullptr;
    }
}

Packet *irnCore::snd_copytoBDPmap(int seq,Packet *pck)
{
    if (BDP_Cap.size() < Capsize)
    {
        EV<<"snd_copytoBDPmap, BDP_Cap is not full, directly copy the current pck into the cap."<<endl;
        BDP_Cap[seq] = pck->dup();
    }
    else
    {
        EV<<"snd_copytoBDPmap, BDP_Cap is full, discard the first pck "<< BDP_Cap.begin()->first <<" and copy the current pck into the cap."<<endl;
        BDP_Cap.erase(BDP_Cap.begin());
        BDP_Cap[seq] = pck->dup();
    }
    return nullptr;
}

void irnCore::rcv_sendAck(L3Address destaddr, int sackNo, int cumAck, bool ackSyndrome, simtime_t pckTS)
{
    // setFragmentOffset means cumulatively ack
    // Idetification means sackNo
    // dontfragment=true means nack=true

    // for the receiver:
    // Upon every out-of-order packet arrival, an IRN receiver
    // sends a NACK, which carries both the cumulative acknowledgment
    // (indicating its expected sequence number) and the
    // sequence number of the packet that triggered the NACK (as
    // a simplified form of selective acknowledgement or SACK).

    EV<<"rcv_sendAck, the sack No. = "<<sackNo<<", cumAck No. = "<<cumAck<<endl;
    Packet *ack = new Packet("ACK");
    const auto& content = makeShared<Ipv4Header>();

    content->setChunkLength(b(32));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
    content->setIdentification(sackNo);
    content->setFragmentOffset(cumAck);
    content->setDontFragment(ackSyndrome);

    switch (crcMode) {
        case CRC_DECLARED_CORRECT:
            // if the CRC mode is declared to be correct, then set the CRC to an easily recognizable value
            content->setCrc(0xC00D);
            break;
        case CRC_DECLARED_INCORRECT:
            // if the CRC mode is declared to be incorrect, then set the CRC to an easily recognizable value
            content->setCrc(0xBAAD);
            break;
        case CRC_COMPUTED: {
            content->setCrc(0);
            // crc will be calculated in fragmentAndSend()
            break;
        }
        default:
            throw cRuntimeError("Unknown CRC mode");
    }
    ack->insertAtFront(content);

    ack->setTimestamp(pckTS);
    ack->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    ack->addTagIfAbsent<L3AddressReq>()->setSrcAddress(local_src);
    ack->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    ack->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    sendDown(ack);
}

void irnCore::rcv_receiveData(Packet *pck)
{
    // If receive a UDP packet, determine which flow the packet belongs to.

    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    const auto& data_head = pck->removeAtFront<Ipv4Header>();
    bool NACK = false;
    auto it = receiver_QPMap.find(l3addr->getSrcAddress());
    if (it==receiver_QPMap.end())
    {
        receiver_QPMap[l3addr->getSrcAddress()] = trcvInfo;
    }

    rcv_QPInfo rcvInfo = receiver_QPMap.find(l3addr->getSrcAddress())->second;

    EV<<"rcv_receiveData, the sack No. = "<<data_head->getIdentification()<<", expected (cumAck) No. = "<<rcvInfo.expectedSeq<<endl;

    //if the packet corresponds to expected sequence number,
    //advance the bitmaps and increase expected sequence, message sequence (MSN),
    //and number of CQEs completed accordingly.
    if(data_head->getIdentification() == rcvInfo.expectedSeq) {
        rcvInfo.expectedSeq++;
        uint64_t temp = rcvInfo.ooo_bitmap;
        // the greater sack has arrived.
        if(temp != 0) {
            rcvInfo.ooo_bitmap = rcvInfo.ooo_bitmap >> 1;
            //rcvInfo.expectedSeq++;

            short bits_to_shift = 0;
            uint64_t part = rcvInfo.ooo_bitmap;

            if((part & 0xffffffff) == 0xffffffff) {
                bits_to_shift += 32;
                part = part >> 32;
            }
            if((part & 0xffff) == 0xffff) {
                bits_to_shift += 16;
                part = part >> 16;
            }
            if((part & 0xff) == 0xff) {
                bits_to_shift += 8;
                part = part >> 8;
            }
            if((part & 0xf) == 0xf) {
                bits_to_shift += 4;
                part = part >> 4;
            }
            if((part & 3) == 3) {
                bits_to_shift += 2;
                part = part >> 2;
            }
            if((part & 1) == 1){
                bits_to_shift += 1;
                part = part >> 1;
            }
            if((part & 1) == 1){
                bits_to_shift += 1;
            }
            rcvInfo.ooo_bitmap = rcvInfo.ooo_bitmap >> bits_to_shift;
            rcvInfo.expectedSeq += bits_to_shift;
            EV<<"bits_to_shift"<<bits_to_shift<<", expectedSeq = "<<rcvInfo.expectedSeq<<endl;
        }
        //if(rcvInfo.ooo_bitmap != 0)
        if(temp != 0)
        {
            NACK = true;
        }
        rcv_sendAck(l3addr->getSrcAddress(),data_head->getIdentification(),rcvInfo.expectedSeq-1,NACK,pck->getTimestamp());
        EV<< "rcv_receiveData, the ooo_bitmap = "<<rcvInfo.ooo_bitmap<<", NACK = "<<NACK<<endl;
        receiver_QPMap[l3addr->getSrcAddress()] = rcvInfo;
        sendUp(pck);
    //if arrived packet's sequence number is greater than expected, prepare a NACK and mark bitmap.
    }
    else if(data_head->getIdentification() > rcvInfo.expectedSeq)
    {
        NACK = true;
        rcv_sendAck(l3addr->getSrcAddress(),data_head->getIdentification(),rcvInfo.expectedSeq,NACK,pck->getTimestamp());
        uint64_t temp = 1;
        temp = temp << (data_head->getIdentification() - rcvInfo.expectedSeq);
        rcvInfo.ooo_bitmap = rcvInfo.ooo_bitmap | temp;
        EV<< "rcv_receiveData, the ooo_bitmap = "<<rcvInfo.ooo_bitmap<<", NACK = "<<NACK<<endl;
        receiver_QPMap[l3addr->getSrcAddress()] = rcvInfo;
        sendUp(pck);
    }
    //if arrived packet's sequence number is smaller than expected, drop the packet.
    else
    {
        rcv_sendAck(l3addr->getSrcAddress(),data_head->getIdentification(),rcvInfo.expectedSeq-1,NACK,pck->getTimestamp());
        EV<< "rcv_receiveData, arrived packet's sequence number is smaller than expected, drop the packet."<<endl;
        delete pck;
    }

}

void irnCore::sendDown(Packet *pck)
{
    EV << "sendDown, data map size = " << data_Map.size() << " !"<<endl;
    send(pck, lowOutGate);
}

void irnCore::sendUp(Packet *pck)
{
    EV << "sendUp!"<< endl;
    send(pck, upOutGate);
}
} // namespace inet


