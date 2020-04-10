// Copyright (C) 2018 Angrydudu

#include "AngryCredits.h"
#include "inet/common/INETDefs.h"


namespace inet {

Define_Module(AngryCredits);

simsignal_t AngryCredits::queueLengthSignal = registerSignal("queueLength");

void AngryCredits::initialize()
{
    //statistics
    emit(queueLengthSignal, int(data_Map.size()));
    outGate = gate("lowerOut");
    inGate = gate("lowerIn");
    upGate = gate("upperOut");
    downGate = gate("upperIn");
    // configuration
    multipath = par("enableMP");
    mp_algorithm = par("mp_algorithm");
    activate = par("activate");
    frameCapacity = par("frameCapacity");
    linkspeed = par("linkspeed");
    bits_timeout = par("bits_timeout");

    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

    const char *crcModeString = par("crcMode");
    if (!strcmp(crcModeString, "declared"))
        crcMode = CRC_DECLARED_CORRECT;
    else if (!strcmp(crcModeString, "computed"))
        crcMode = CRC_COMPUTED;
    else
        throw cRuntimeError("Unknown crc mode: '%s'", crcModeString);

    registerService(Protocol::tcp, gate("upperIn"), gate("lowerIn"));
    registerProtocol(Protocol::tcp, gate("lowerOut"), gate("upperOut"));

    delayMap.clear();
    data_Map.clear();
    credit_Map.clear();
    receiver_flowMap.clear();
    receiver_mpMap.clear();
    sender_flowMap.clear();

    EV<< activate<<"  "<<linkspeed<<endl;

    temp_info = new Packet();
    stopcdt_info = new Packet();
    sendcredit = new cMessage("sendcred", SENDCRED);
    stopcredit = new cMessage("stopcred", STOPCRED);
    // state

    // credit state
    CreState = INITIAL_STATE;

    credit_size = 64*8;
    // The parameters of feedback control
    alpha = 0.5;
    targetratio = 0.125;
    previousincrease = false;
    wmax = 0.5;
    wmin = 0.01;
    w = 0.5;
    maxrate = 0.04*linkspeed; // The max link capacity for transmitting credits.
    if (multipath)
    {
        currate = maxrate;
    }
    else
    {
        currate = maxrate/4;
    }
    max_lossratio = 0.8;
    // Initialize the credit sequence number of the receiver

    delta_cdt_time = credit_size/currate;
    last_cdt_time = 0;


    // Used for judging the end of flow.
    this->max_idletime = bits_timeout/linkspeed;
    // this->max_idletime = 0.002;

    treceiver_flowinfo = {simTime(),0,0,1,0,0,0,currate,currate};
    tsender_flowinfo = {simTime()};

    if (activate)
    {
        // statistics
        numMapReceived = 0;
        numMapDropped = 0;
        WATCH(numMapReceived);  // By using WATCH() function, we can see the value in the simulator.
        WATCH(numMapDropped);
    }
}

void AngryCredits::handleMessage(cMessage *msg)
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
        if (msg->arrivedOn("upperIn"))
        {
            sendDown(check_and_cast<Packet*>(msg));
        }
        else if (msg->arrivedOn("lowerIn"))
        {
            sendUp(check_and_cast<Packet*>(msg));
        }
    }
}

void AngryCredits::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << pck << " received\n";

    switch (pck->getKind()) {
        case SENDCRED:
            self_send_credit();
            break;
        case STOPCRED:
            self_stop_credit();
            break;
    }
}

void AngryCredits::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "q rcvd: %d\nq dropped: %d", numMapReceived, numMapDropped);
    getDisplayString().setTagArg("t", 0, buf);
}

// Process the message from lower layer.
void AngryCredits::processUpperpck(Packet *pck)
{
    if(!strcmp(pck->getFullName(),"SYN"))
    {
        auto l3AddressReq = pck->addTagIfAbsent<L3AddressReq>();
        sender_flowinfo sndflow = tsender_flowinfo;
        sndflow.cretime = simTime();
        sender_flowMap[l3AddressReq->getDestAddress()] = sndflow;
        EV << "0000 send SYN down, sender flow map size = "<< sender_flowMap.size()<<endl;
        sndsrc = l3AddressReq->getSrcAddress();
        L3Address snddest = l3AddressReq->getDestAddress();

        pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x12);
        sendDown(pck); // send down the SYN.
        send_credreq(snddest);
    }
    else if (string(pck->getFullName()).find("tcpseg") != string::npos)
    {
        // determine the destination address of the pck
        auto *l3addr = pck->addTagIfAbsent<L3AddressReq>();
        tdest = l3addr->getDestAddress();
        auto it = sender_flowMap.find(l3addr->getDestAddress());

        sender_flowinfo info = it->second;
        info.cretime = simTime();
        sender_flowMap[tdest] = info;

        // find the flow info in the sender_flowMap map, according to the destination addr.
        if ( it == sender_flowMap.end())
        {
            send_credreq(l3addr->getDestAddress());
        }

        if (findCredit(tdest) != nullptr)
        {
            Packet *credit = extractCredit(tdest);
            const auto& data_head = makeShared<Ipv4Header>();
            const auto& credit_head = credit->peekAtFront<Ipv4Header>();
            data_head->setChunkLength(b(32));
            data_head->setIdentification(credit_head->getIdentification());
            data_head->setDiffServCodePoint(11);
            pck->insertAtFront(data_head);
            pck->setSchedulingPriority(credit->getSchedulingPriority());
            pck->setTimestamp(simTime());
            //pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x0A);
            delete credit;
            sendDown(pck);
        }
        else
        {
            pck->setArrivalTime(simTime());
            cMessage *droppedpck = insertData(tdest,pck);
            if (droppedpck)
            {
                numMapDropped++;
                delete droppedpck;
            }
        }
    }
    else if(!strcmp(pck->getFullName(),"TcpAck")||string(pck->getFullName()).find("ACK") != string::npos)
    {
        //pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x12);
        sendDown(pck);
    }
    else if(!strcmp(pck->getFullName(),"RST")||!strcmp(pck->getFullName(),"FIN"))
    {
        //pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x12);
        sendDown(pck);
    }
    else
    {
        sendDown(pck);
    }
}

// Process the Packet from upper layer.
void AngryCredits::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"credit_req"))
    {
        receive_credreq(pck);
    }
    else if (!strcmp(pck->getFullName(),"credit"))
    {
        receive_credit(pck);
    }
    else if (!strcmp(pck->getFullName(),"credit_stop"))
    {
        receive_stopcred(pck);
    }
    else if (string(pck->getFullName()).find("tcpseg") != string::npos)
    {
        receive_data(pck);
    }
    else
    {
        sendUp(pck);
    }
}

Packet *AngryCredits::findData(L3Address addr)
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
    EV << "No credit associated to this data packet."<<endl;
    return nullptr;
}

Packet *AngryCredits::extractData(L3Address addr)
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
    EV << "No data packet associated to this credit."<<endl;
    return nullptr;
}

Packet *AngryCredits::insertData(L3Address addr,Packet *pck)
{

    if (frameCapacity && int(data_Map.size()) >= frameCapacity)
    {
        EV << "Map full, dropping packet.\n";
        return pck;
    }
    else
    {
        data_Map.insert(pair<L3Address,Packet*>(addr,pck));
        //data_Map[addr] = pck;
        emit(queueLengthSignal, int(data_Map.size()));
        EV << "0000 data packet has been inserted into the map, map size = "<<data_Map.size()<<endl;
        return nullptr;
    }
}

Packet *AngryCredits::extractCredit(L3Address addr)
{
    auto it = credit_Map.begin();
    for(; it != credit_Map.end(); ++ it)
    {
        if (addr == it->first)
        {
            Packet *pck = it->second;
            credit_Map.erase(it);
            return pck;
        }
    }
    EV << "No credit associated to this data packet."<<endl;
    return nullptr;
}

Packet *AngryCredits::findCredit(L3Address addr)
{
    auto it = credit_Map.begin();
    for(; it != credit_Map.end(); ++ it)
    {
        if (addr == it->first)
        {
            Packet *pck = it->second;
            return pck;
        }
    }
    EV << "No credit associated to this data packet."<<endl;
    return nullptr;
}

Packet *AngryCredits::insertCredit(L3Address addr,Packet *pck)
{

    if (frameCapacity && int(credit_Map.size()) >= frameCapacity)
    {
        EV << "Map full, dropping credit.\n";
        return pck;
    }
    else
    {
        credit_Map.insert(pair<L3Address,Packet*>(addr,pck));
        //data_Map[addr] = pck;
        emit(queueLengthSignal, int(credit_Map.size()));
        EV << "0000 creidt packet has been inserted into the map, map size = "<<credit_Map.size()<<endl;
        return nullptr;
    }
}

Packet *AngryCredits::newflowinfo(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressReq>();
    Packet *info = new Packet;
    info->addTagIfAbsent<L3AddressReq>()->setSrcAddress(l3addr->getSrcAddress());
    info->addTagIfAbsent<L3AddressReq>()->setDestAddress(l3addr->getDestAddress());

    return info;
}

int AngryCredits::deleteFlowCredits(L3Address addr)
{
    int sum_deleted;
    sum_deleted = credit_Map.count(addr);
    credit_Map.erase(addr);
    return sum_deleted;
}

void AngryCredits::sendDown(Packet *pck)
{
    EV << "00000000 Oh send down, data map size = " << data_Map.size() << " ! 00000000"<<endl;
    send(pck, outGate);
}

void AngryCredits::sendUp(Packet *pck)
{
    EV << "oh sendup!"<< endl;
    send(pck, upGate);
}

void AngryCredits::send_credreq(L3Address destaddr)
{
    Packet *cred_req = new Packet("credit_req");
    cred_req->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    cred_req->addTagIfAbsent<L3AddressReq>()->setSrcAddress(sndsrc);
    cred_req->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x1A);
    cred_req->setTimestamp(simTime());
    cred_req->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_req->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::tcp);

    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(b(32));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
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
    cred_req->insertAtFront(content);
    EV << "00000000 send credit_req, DestAddr = "<<destaddr.toIpv4()<<" !00000000"<<endl;
    sendDown(cred_req); // send down the credit_request.
}

// Generate and send credit to the destination.
void AngryCredits::send_credit(L3Address destaddr,unsigned int pathid, int seq)
{
    Packet *credit = new Packet("credit");

    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(b(32));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
    content->setIdentification(seq);
    //content->setSrcAddress((Ipv4Address)pathid);
    content->setDiffServCodePoint(pathid);
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
    credit->insertAtFront(content);

    credit->setSchedulingPriority(pathid);
    credit->setTimestamp(simTime());
    credit->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    credit->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcvsrc);
    credit->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x2E);
    credit->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    credit->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::tcp);

    EV << "00000000 send credit, DestAddr = "<<destaddr.toIpv4()<<", dscp value = "<<credit->addTagIfAbsent<DscpReq>()->getDifferentiatedServicesCodePoint()<<endl;
    sendDown(credit); // send down the credit
}

// Generate and send stop credit Packet to the destination.
void AngryCredits::send_stop(L3Address addr)
{
    Packet *cred_stop = new Packet("credit_stop");
    cred_stop->addTagIfAbsent<L3AddressReq>()->setDestAddress(addr);
    cred_stop->addTagIfAbsent<L3AddressReq>()->setSrcAddress(sndsrc);
    cred_stop->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x1A);
    cred_stop->setTimestamp(simTime());
    cred_stop->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_stop->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::tcp);

    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(b(32));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
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
    cred_stop->insertAtFront(content);
    EV << "00000000 send credit_stop, DestAddr = "<<addr.toIpv4()<<" !00000000"<<endl;
    sendDown(cred_stop); // send down the credit_stop.
}

// Receive the SYN packet from the sender, and begin sending credit.
void AngryCredits::receive_credreq(Packet *pck)
{
    currate = maxrate/(receiver_flowMap.size()+1);   // Change the current max credit rate of each flows.
    //currate = maxrate;

    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    rcvsrc = l3addr->getDestAddress();

    receiver_flowinfo rcvflow = treceiver_flowinfo;
    rcvflow.nowRTT = 2*(simTime() - pck->getTimestamp());
    rcvflow.last_Fbtime = simTime();
    rcvflow.pck_in_rtt = 0;
    rcvflow.newspeed = currate;
    rcvflow.max_speedsum = currate;
    temp_info = newflowinfo(pck);

    receiver_mpinfo rcvmp = treceiver_mpinfo;
    rcvmp.virtualpaths = 1;
    rcvmp.path_id[0] = rand()%256;
    rcvmp.speed[0] = currate;
    rcvmp.next_time[0] = 0;
    EV <<"0000 current rate = "<< currate<<endl;
    receiver_flowMap[l3addr->getSrcAddress()] = rcvflow;
    receiver_mpMap[l3addr->getSrcAddress()] = rcvmp;

    EV <<"0000 receiver_flow = "<< receiver_flowMap.size()<<endl;
    // Only the first flow should do the following steps.
    if (receiver_flowMap.size() == 1)
    {
        cancelEvent(sendcredit);
        delayMap[l3addr->getSrcAddress()] = credit_size/currate;
        next_creditaddr = l3addr->getSrcAddress();
        EV <<"0000 the next credit destination address = "<<next_creditaddr.toIpv4()<<endl;
        last_cdt_time = simTime();
        CreState = SEND_CREDIT_STATE;
        scheduleAt(simTime(),sendcredit); //schedule selfMessage
    }
    else
    {
        auto d1_it = delayMap.begin();
        for(; d1_it != delayMap.end()&&d1_it->first != l3addr->getSrcAddress(); ++ d1_it)
        {
            // minus delta time at first, and then expand the remaining time.
            delayMap[d1_it->first] = d1_it->second - simTime() + last_cdt_time;
            delayMap[d1_it->first] = d1_it->second * receiver_flowMap.size()/(receiver_flowMap.size()-1); // reset the schedule time of flows, for no congestion at the host nic.
            receiver_mpinfo tmpinfo = receiver_mpMap.find(d1_it->first)->second;
            receiver_flowinfo finfo = receiver_flowMap.find(d1_it->first)->second;
            finfo.newspeed = finfo.newspeed*(receiver_flowMap.size()-1)/receiver_flowMap.size(); // here we change all the flows' speed, including the new one.
            finfo.max_speedsum = finfo.max_speedsum*(receiver_flowMap.size()-1)/receiver_flowMap.size();
            receiver_flowMap[d1_it->first] = finfo;
            for (int i=0; i<tmpinfo.virtualpaths; i++)
            {
                tmpinfo.next_time[i] = tmpinfo.next_time[i] - simTime() + last_cdt_time;
                tmpinfo.next_time[i] = tmpinfo.next_time[i] * receiver_flowMap.size()/(receiver_flowMap.size()-1); // reset the schedule time of virtual paths.
                tmpinfo.speed[i] = tmpinfo.speed[i] * (receiver_flowMap.size()-1)/receiver_flowMap.size(); // here we change all the speed on the virtual paths.
                receiver_mpMap[d1_it->first] = tmpinfo;
                EV<<"444444 Im in, the newly next time of "<<d1_it->first<<"__"<<i<<" = "<<receiver_mpMap.find(d1_it->first)->second.next_time[i]<<endl;
            }
        }
        cancelEvent(sendcredit); // The previous sendcredit pck scheduling for other flows.
        delayMap[l3addr->getSrcAddress()] = credit_size/currate;
        next_creditaddr = l3addr->getSrcAddress();
        last_cdt_time = simTime();
        scheduleAt(simTime(),sendcredit);
    }
    EV<<"0000 The size of delayMap = "<< delayMap.size()<<endl;
}

// Receive the credit, and send corresponding TCP segment.
void AngryCredits::receive_credit(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    bool stop = false;
    L3Address stopaddr;
    // Check whether the flow has been dried and send credit stop Packet
    auto it = sender_flowMap.begin();
    for(; it != sender_flowMap.end(); ++it)
    {
        EV << "0000 dest addr = "<< it->first <<endl;
        sender_flowinfo info = it->second;
        if((simTime()-info.cretime)>= max_idletime)
        {
            if (findData(it->first) == nullptr)
            {
                stopaddr = it->first;
                stop = true;
            }
        }
    }
    if(stop)
    {
        EV << "0000 lets delete flow: "<<sender_flowMap.find(stopaddr)->first<<", "<<sender_flowMap.find(stopaddr)->second.cretime<<endl;
        sender_flowMap.erase(stopaddr);
        send_stop(stopaddr);
        EV << "0000 sender flow map size = "<< sender_flowMap.size()<<endl;
    }
    // Send data packet according to the credit
    Packet *data = extractData(l3addr->getSrcAddress());
    if (data == nullptr) {
        insertCredit(l3addr->getSrcAddress(),pck);
        EV << "00000000 packets allowed to be sent = "<< credit_Map.size() << endl;
    }
    else {
        const auto& data_head = makeShared<Ipv4Header>();
        const auto& credit_head = pck->peekAtFront<Ipv4Header>();
        data_head->setChunkLength(b(32));
        data_head->setIdentification(credit_head->getIdentification());
        data_head->setDiffServCodePoint(11); // determine the data packets is scheduled by angrycredits
        data->insertAtFront(data_head);
        data->setTimestamp(simTime());
        data->setSchedulingPriority(pck->getSchedulingPriority());
        //data->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x0A);
        delete pck;
        sendDown(data);
    }
}

// Receive the stop credit Packet from sender, and stop sending credit.
void AngryCredits::receive_stopcred(Packet *pck)
{
    EV<<"0000 entering receivestop_cdt"<<endl;
    stop_addr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    scheduleAt(simTime(),stopcredit); //schedule selfmessage
}

// Receive the TCP segment, determine the credit loss ratio and enter the feedback control.
void AngryCredits::receive_data(Packet *pck)
{
    // If receive a TCP segment, determine which flow the segment belongs to.
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    EV <<"00000000 The src address of the segment = "<< l3addr->getSrcAddress().toIpv4()<<" ! 00000000"<<endl;

    // If the segment is not recognizable, directly pass it.
    auto it = receiver_flowMap.find(l3addr->getSrcAddress());

    const auto& data_head = pck->removeAtFront<Ipv4Header>();
    if (data_head->getDiffServCodePoint() == 11)
    {
    if (it!=receiver_flowMap.end())
    {
        receiver_flowinfo tinfo = it->second;
        tinfo.nowseq = data_head->getIdentification();
        EV<<"receive_data(), now the one-side RTT = "<<tinfo.nowRTT<<", the time gap from last feedback control = "<< simTime()-tinfo.last_Fbtime <<endl;
        // Calculate the credit loss ratio, and enter the feedback control.
        if ((simTime()-tinfo.last_Fbtime) < tinfo.nowRTT)
        {
            tinfo.pck_in_rtt++;
            receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
        }
        else
        {
            tinfo.pck_in_rtt++;
            tinfo.sumlost = tinfo.nowseq - tinfo.lastseq - tinfo.pck_in_rtt;
            EV<<"receive_data(), sumlost = "<<tinfo.sumlost<<", nowseq = "<<tinfo.nowseq<<", lastseq = "<<tinfo.lastseq<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
            if(tinfo.sumlost >= 0) // else: error may occur
            {
                if (!multipath)
                {
                    double oldspeed = tinfo.newspeed;
                    tinfo.newspeed = feedback_control(oldspeed,tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                    receiver_mpinfo mpinfo = receiver_mpMap.find(it->first)->second;
                    for (int i=0;i<mpinfo.virtualpaths;i++)
                    {
                        mpinfo.speed[i] = mpinfo.speed[i]*tinfo.newspeed/oldspeed;
                    }
                    receiver_mpMap[it->first] = mpinfo;
                    EV <<"receive_data() no multipath, new speed in mpMap = "<< receiver_mpMap.find(it->first)->second.speed[0] <<endl;
                }
                else
                {
                    if (mp_algorithm == 0)
                    {
                        EV <<"receive_data() Al-0, Enter feedback control of No." << it->first << " flow, "<<" last seq = " << tinfo.lastseq << ", now seq = "<< tinfo.nowseq <<endl;
                        EV <<"receive_data() Al-0, received data packets = "<< tinfo.pck_in_rtt <<", old speed in flow map = "<< tinfo.newspeed <<endl;
                        double oldspeed = tinfo.newspeed;
                        tinfo.newspeed = feedback_control(oldspeed,tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                        EV <<"receive_data() Al-0, new speed in flow map = "<< tinfo.newspeed <<", currate = "<<currate<<endl;
                        int virtualpath = receiver_mpMap.find(it->first)->second.virtualpaths;
                        if(tinfo.newspeed < currate*(virtualpath)/(virtualpath+1)) // example: 3/4 for 3 paths
                        {
                            multipath_control(it->first,tinfo.newspeed,oldspeed,currate);
                            tinfo.newspeed = currate;
                        }
                        else //
                        {
                            receiver_mpinfo mpinfo = receiver_mpMap.find(it->first)->second;
                            for (int i=0;i<mpinfo.virtualpaths;i++)
                            {
                                mpinfo.speed[i] = mpinfo.speed[i]*tinfo.newspeed/oldspeed;
                            }
                            receiver_mpMap[it->first] = mpinfo;
                            EV <<"receive_data() Al-0, new speed in mpMap = "<< receiver_mpMap.find(it->first)->second.speed[0] <<endl;
                        }
                    }
                    if (mp_algorithm == 1)
                    {
                        EV <<"receive_data() Al-1, Enter feedback control of No." << it->first << " flow, "<<" last seq = " << tinfo.lastseq << ", now seq = "<< tinfo.nowseq <<endl;
                        EV <<"receive_data() Al-0, received data packets = "<< tinfo.pck_in_rtt <<", old speed in flow map = "<< tinfo.newspeed <<endl;
                        double oldspeed = tinfo.newspeed;
                        //tinfo.newspeed = AL1_control(tinfo.newspeed,tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                        tinfo.newspeed = feedback_control(oldspeed,tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                        receiver_mpinfo tmpinfo = receiver_mpMap.find(it->first)->second;
                        if ((tinfo.newspeed/tinfo.max_speedsum < 1-targetratio && tmpinfo.virtualpaths >= 8)||(tinfo.newspeed/tinfo.max_speedsum < 1-max_lossratio))
                        {
                            double old_sumrate = tinfo.max_speedsum;
                            tinfo.max_speedsum = feedback_control(old_sumrate,tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                            EV<<"receive_data() Al-1, changing max_speedsum, old one = "<<old_sumrate<<", new one = "<<tinfo.max_speedsum<<endl;
                            for (int i=0;i<tmpinfo.virtualpaths;i++)
                            {
                                tmpinfo.speed[i] = tmpinfo.speed[i]*tinfo.max_speedsum/old_sumrate;
                            }
                            receiver_mpMap[it->first] = tmpinfo;
                        }
                        else if(tinfo.newspeed/tinfo.max_speedsum > 1-targetratio && tmpinfo.virtualpaths >= 4)
                        {
                            double old_sumrate = tinfo.max_speedsum;
                            tinfo.max_speedsum = feedback_control(tinfo.max_speedsum,tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                            EV<<"receive_data() Al-1, changing max_speedsum, old one = "<<old_sumrate<<", new one = "<<tinfo.max_speedsum<<endl;
                            for (int i=0;i<tmpinfo.virtualpaths;i++)
                            {
                                tmpinfo.speed[i] = tmpinfo.speed[i]*tinfo.max_speedsum/old_sumrate;
                            }
                            receiver_mpMap[it->first] = tmpinfo;
                        }
                        else if(tinfo.newspeed/tinfo.max_speedsum < 1-targetratio)
                        {
                            EV <<"receive_data() Al-1, speed no changing, new speed in flow map = "<< tinfo.newspeed <<", max_speedsum = "<<tinfo.max_speedsum<<endl;
                            multipath_control(it->first,tinfo.newspeed,oldspeed,tinfo.max_speedsum);
                            tinfo.newspeed = tinfo.max_speedsum;
                        }
                    }
                }
                tinfo.nowRTT = 2*(simTime() - pck->getTimestamp());
                tinfo.last_Fbtime = simTime();
                tinfo.sumlost = 0;
                tinfo.lastseq = tinfo.nowseq;
                tinfo.pck_in_rtt = 0;
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                EV<<"receive_data(), after feedback control, lastseq = "<<tinfo.lastseq<<endl;
            }
            else
            {
                tinfo.nowRTT = 2*(simTime() - pck->getTimestamp());
                tinfo.last_Fbtime = simTime();
                tinfo.sumlost = 0;
                tinfo.lastseq = tinfo.nowseq;
                tinfo.pck_in_rtt = 0;
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                EV<<"receive_data(), sumlost < 0, some packets are in flight."<< endl;
                EV<<"receive_data(), after feedback control, lastseq = "<<tinfo.lastseq<<endl;
            }

        }
    }
    sendUp(pck);
    }
    else
    {
        pck->insertAtFront(data_head);
        sendUp(pck);
    }
}

// Handle the self message: send credit.
void AngryCredits::self_send_credit()
{
    if (CreState != STOP_CREDIT_STATE)
    {
        delta_cdt_time = simTime() - last_cdt_time;
        EV <<"0000 time gap between sending credits = "<< delta_cdt_time << endl;
        schedule_next_credit(delta_cdt_time);
    }
}

// Handle the self message stop credit.
void AngryCredits::self_stop_credit()
{
    EV<<"self_stop_credit(), flows before deleted = "<<receiver_flowMap.size()<<endl;

    receiver_flowMap.erase(stop_addr);
    receiver_mpMap.erase(stop_addr);
    delayMap.erase(stop_addr);
    deleteFlowCredits(stop_addr);

    EV << "00000000 receiver_flows = "<<receiver_flowMap.size()<<" ! 00000000"<<endl;
    if (receiver_flowMap.size() == 0)
    {
        currate = maxrate;
        CreState = STOP_CREDIT_STATE;
    }
    else
    {
        // re-schedule the credit_send, increase speed and decrease schedule time.
        currate = maxrate/receiver_flowMap.size();
        auto d1_it = delayMap.begin();
        for(; d1_it != delayMap.end(); ++ d1_it)
        {
            // minus delta time at first, and then shrink the remaining time.
            delayMap[d1_it->first] = d1_it->second - simTime() + last_cdt_time;
            delayMap[d1_it->first] = d1_it->second * receiver_flowMap.size()/(receiver_flowMap.size()+1); // reset the schedule time of flows, for no congestion at the host nic.
            receiver_mpinfo tmpinfo = receiver_mpMap.find(d1_it->first)->second;
            receiver_flowinfo finfo = receiver_flowMap.find(d1_it->first)->second;
            finfo.newspeed = finfo.newspeed*(receiver_flowMap.size()+1)/receiver_flowMap.size(); // here we change all the flows' schedule time, including the new one.
            finfo.max_speedsum = finfo.max_speedsum*(receiver_flowMap.size()+1)/receiver_flowMap.size();
            receiver_flowMap[d1_it->first] = finfo;
            for (int i=0; i<tmpinfo.virtualpaths; i++)
            {
                tmpinfo.next_time[i] = tmpinfo.next_time[i] - simTime() + last_cdt_time;
                tmpinfo.next_time[i] = tmpinfo.next_time[i] * receiver_flowMap.size()/(receiver_flowMap.size()+1); // reset the rate of virtual paths.
                tmpinfo.speed[i] = tmpinfo.speed[i] * (receiver_flowMap.size()+1)/receiver_flowMap.size(); // here we change all the speed on the virtual paths.
                receiver_mpMap[d1_it->first] = tmpinfo;
            }
        }
        cancelEvent(sendcredit); // The previous sendcredit pck scheduling for other flows.
        find_nextaddr();
        last_cdt_time = simTime();
        scheduleAt(simTime() + next_cdt_delay,sendcredit);
    }
}

// Do credit feedback control.
double AngryCredits::feedback_control(double nowrate, int sumlost, int sumcredits)
{
    double lossratio;
    lossratio = double(sumlost)/double(sumcredits);
    EV << "feedback_control(), sumlost = "<< sumlost << ", sumcredits = "<< sumcredits<<endl;
    EV << "feedback_control(), loss ratio = "<< lossratio <<endl;
    if(lossratio<=targetratio)
    {
        if(previousincrease)
        {
            w = (w+wmax)/2;
        }
        nowrate = (1-w)*nowrate + w*maxrate*(1+targetratio);
        previousincrease = true;
    }
    else if(lossratio<=max_lossratio)
    {
        previousincrease = false;
        nowrate = nowrate*(1-lossratio)*(1+targetratio);
        if (w/2>wmin)
            w = w/2;
        else
            w = wmin;
    }
    else
    {
        previousincrease = false;
        nowrate = nowrate*(1-max_lossratio)*(1+targetratio);
        if (w/2>wmin)
            w = w/2;
        else
            w = wmin;
    }
    return max(nowrate,0.005*linkspeed);
}

double AngryCredits::AL1_control(double nowrate, int sumlost, int sumcredits)
{
    EV<<"AL1_control(), loss ratio = "<< (double)sumlost/(double)sumcredits <<endl;
    return receiver_flowMap.size()*nowrate*(1-(double)sumlost/(double)sumcredits);
}

void AngryCredits::schedule_next_credit(simtime_t delta_t)
{
    // for multi-path transmitting
    receiver_mpinfo mp_info = receiver_mpMap.find(next_creditaddr)->second;
    simtime_t min_t= 100000;
    int mpindex = -1;
    int mpsum = mp_info.virtualpaths;
    //int mpsum = mp_info.virtualpaths;
    EV<<"0000 the multi-paths size = "<<mpsum<<", for address"<<next_creditaddr.toIpv4()<<endl;
    for (int i=0;i<mpsum;i++)
    {
        EV<<"aaaaa next time before = " << mp_info.next_time[i]<<endl;
        mp_info.next_time[i] = mp_info.next_time[i] - delta_t;
        EV<<"aaaaa next time after = " << mp_info.next_time[i]<<endl;
        if (mp_info.next_time[i] == 0)
        {
            EV<<"11111"<<endl;
            mp_info.next_time[i] = credit_size/mp_info.speed[i]; // will minus delta_t in following
            mpindex = i;
        }
        else
        {
            EV<<"22222 next time = "<<mp_info.next_time[i]<<"delta_t = "<<delta_t<<endl;
            mp_info.next_time[i] = mp_info.next_time[i]; // will minus delta_t in following
        }
        if (mp_info.next_time[i] < min_t)
        {
            EV<<"33333"<<endl;
            min_t = mp_info.next_time[i];
        }
        mp_info.next_time[i] = mp_info.next_time[i] + delta_t;
    }

    // send the current credit before scheduling the next credit.
    auto f_it = receiver_flowMap.find(next_creditaddr);
    receiver_flowinfo f_info = f_it->second;
    if (mpindex != -1) // mpMap has been updated during scheduling. multipath_control() has been called
    {
        send_credit(next_creditaddr, mp_info.path_id[mpindex], f_info.creditseq);
    }
    ////////////////////////////////////
    last_cdt_time = simTime();

    f_info.creditseq = f_info.creditseq + 1;
    receiver_flowMap[next_creditaddr] = f_info;
    receiver_mpMap[next_creditaddr] = mp_info;

    EV<<"0000 the delay map size = "<<delayMap.size()<<endl;

    // add delta_t before minus it, and then the delay value can be the new min_t.
    delayMap[next_creditaddr] = min_t + delta_t;

    auto d_it = delayMap.begin();
    for(; d_it != delayMap.end(); ++ d_it)
    {
        delayMap[d_it->first] = d_it->second - delta_t;
        receiver_mpinfo tmpinfo = receiver_mpMap.find(d_it->first)->second;
        for (int i=0; i<tmpinfo.virtualpaths; i++)
        {
            tmpinfo.next_time[i] = tmpinfo.next_time[i] - delta_t;
            receiver_mpMap[d_it->first] = tmpinfo;
            EV<<"444444 Im in, the newly next time of "<<d_it->first<<"__"<<i<<" = "<<receiver_mpMap.find(d_it->first)->second.next_time[i]<<endl;
        }
    }

    find_nextaddr();

    EV <<" 00000000 The next delay = "<<next_cdt_delay<<", the next scheduled packet "<<next_creditaddr<< ", seq = "<<receiver_flowMap.find(next_creditaddr)->second.creditseq<<endl;
    delta_cdt_time = next_cdt_delay;
    scheduleAt(simTime()+next_cdt_delay,sendcredit);
}

void AngryCredits::find_nextaddr()
{
    simtime_t min_t= 100000;
    auto d_it = delayMap.begin();
    for(; d_it != delayMap.end(); ++ d_it)
    {
        EV <<"0000 the value of the delay" <<delayMap.find(d_it->first)->second<<", "<<d_it->first<<endl;
        if (delayMap.find(d_it->first)->second < min_t)
        {
            min_t = d_it->second;
            next_creditaddr = d_it->first;
        }
        next_cdt_delay = min_t;
    }
}

simtime_t AngryCredits::get_singleRTT(Packet *pck)
{
    return simTime() - pck->getTimestamp();
}

void AngryCredits::multipath_control(L3Address addr, double speed, double oldspeed, double max_sumspeed)
{
    ////////// multi_path algorithm 0 ///////////
    if (mp_algorithm == 0)
    {
    int vpaths = receiver_mpMap.find(addr)->second.virtualpaths; // the current number of virtual paths.
    int osum_paths = vpaths; // the original number of virtual paths before adjusting.
    receiver_mpinfo mpinfo = receiver_mpMap.find(addr)->second;
    int newpathid = 0;
    int onew_pathid = newpathid; // the original id of the new added virtual path.
    int minid = 0;
    simtime_t mint = 10^10;

    EV<<"multipath_control(), initial vpaths = "<<vpaths<<endl;

    if (vpaths < 256)
    {
        mpinfo.virtualpaths = vpaths + 1;
        mpinfo.path_id[vpaths] = rand()%256;
        mpinfo.speed[vpaths] = max_sumspeed - speed;
        mpinfo.next_time[vpaths] = credit_size/(max_sumspeed-speed);
        mint = credit_size/(max_sumspeed-speed);
        minid = vpaths;
        newpathid = vpaths;
        onew_pathid = newpathid;
        vpaths++;
        osum_paths = vpaths;
    }
    else
    {
        int minsid = 0;
        int minspeed = 10^10;
        for(int i=0;i<256;i++)
        {
            if(mpinfo.speed[i]<minspeed)
            {
                minspeed = mpinfo.speed[i];
                minsid = i;
            }
        }

        EV<<"multipath_control(), virtual paths are more than 256, delete the slowest one :"<<minsid<<endl;
        speed = speed - minspeed;
        for (int j=0;j<minsid;j++)
        {
            mpinfo.speed[j] = mpinfo.speed[j]*(speed/max_sumspeed);
            mpinfo.next_time[j] = mpinfo.next_time[j]*(max_sumspeed/speed);
        }
        for (int j=minsid;j<vpaths;j++)
        {
            mpinfo.path_id[j] = mpinfo.path_id[j+1];
            mpinfo.speed[j] = mpinfo.speed[j+1]*(speed/max_sumspeed);
            mpinfo.next_time[j] = mpinfo.next_time[j+1]*(max_sumspeed/speed);
        }

        mpinfo.path_id[minsid] = rand()%256;
        mpinfo.speed[minsid] = max_sumspeed - speed;
        mpinfo.next_time[minsid] = credit_size/(max_sumspeed-speed);
        mint = credit_size/(max_sumspeed-speed);
        minid = minsid;
        newpathid = minsid;
    }
    // update the receiver_mpMap and delayMap.
    // we adjust the next_time of scheduled address directly at here.
    receiver_mpinfo t_mpinfo;
    int t_i = 0;
    double sum_speed = 0;
    for(int i=0;i<osum_paths;i++)
    {
        if (mpinfo.speed[i]<max_sumspeed/100)
        {
            if (i!=onew_pathid)
            {
                t_mpinfo.speed[onew_pathid] = mpinfo.speed[onew_pathid] + mpinfo.speed[i];
                t_mpinfo.next_time[onew_pathid] = credit_size/(mpinfo.speed[onew_pathid]);
                mint = mpinfo.next_time[onew_pathid];
                speed = speed - mpinfo.speed[i];
                vpaths = vpaths - 1;
                if (onew_pathid>i)
                {
                    newpathid = newpathid - 1; // the new id of added vp has been minus by 1;
                }
                EV <<"multipath_control(), delete the low speed virtual path: "<<addr<<"__"<<i<<", speed = "<<mpinfo.speed[i]<<", sum of vps = "<<vpaths<<endl;
            }
            else
            {
                speed = speed - mpinfo.speed[i];
                vpaths = vpaths - 1;
            }
        }
        else
        {
            t_mpinfo.path_id[t_i] = mpinfo.path_id[i];
            t_mpinfo.next_time[t_i] = mpinfo.next_time[i];
            t_mpinfo.speed[t_i] = mpinfo.speed[i];
            EV <<"multipath_control(), before adjust speed, the next time of "<<addr<<"__"<<t_i<<" = "<<t_mpinfo.next_time[t_i]<<", speed = "<<t_mpinfo.speed[t_i]<<endl;
            t_i++;
        }
    }

    t_mpinfo.virtualpaths = vpaths;
    assert(vpaths==t_i);

    for(int i=0;i<vpaths;i++)
    {
        if(i != newpathid)
        {
            assert(speed!=0 && oldspeed!=0);
            t_mpinfo.speed[i] = t_mpinfo.speed[i]*(speed/oldspeed);
            t_mpinfo.next_time[i] = (t_mpinfo.next_time[i] - (simTime() - last_cdt_time))*(oldspeed/speed);
            EV <<"multipath_control(), after adjusting, the next time of "<<addr<<"__"<<i<<" = "<<t_mpinfo.next_time[i]<<", speed = "<<t_mpinfo.speed[i]<<endl;
            if (t_mpinfo.next_time[i] < mint)
            {
                mint = t_mpinfo.next_time[i];
                minid = i;
            }
            sum_speed = sum_speed + t_mpinfo.speed[i];
        }
        else
        {
            sum_speed = sum_speed + t_mpinfo.speed[i];
            EV <<"multipath_control(), after adjusting, the next time of new added vp "<<addr<<"__"<<i<<" = "<<t_mpinfo.next_time[i]<<", speed = "<<t_mpinfo.speed[i]<<endl;
        }
    }
    receiver_mpMap[addr] = t_mpinfo;

    EV<<"multipath_control() completed, virtualpaths = "<<receiver_mpMap.find(addr)->second.virtualpaths<<endl;

    auto d1_it = delayMap.begin();
    for(; d1_it != delayMap.end(); ++ d1_it)
    {
        if (d1_it->first != addr)
        {
            double othersum_speed = 0;
            delayMap[d1_it->first] = d1_it->second - simTime() + last_cdt_time;
            receiver_mpinfo tmpinfo = receiver_mpMap.find(d1_it->first)->second;
            for (int i=0; i<tmpinfo.virtualpaths; i++)
            {
                othersum_speed = othersum_speed + tmpinfo.speed[i];
                tmpinfo.next_time[i] = tmpinfo.next_time[i] - simTime() + last_cdt_time;
                receiver_mpMap[d1_it->first] = tmpinfo;
                EV<<"0000 reset the next_time of "<<d1_it->first<<"__"<<i<<" = "<<receiver_mpMap.find(d1_it->first)->second.next_time[i]<<", speed = "<<receiver_mpMap.find(d1_it->first)->second.speed[i]<<", path id = "<<tmpinfo.path_id[i]<<endl;
            }
            EV<<"multipath_control(), othersum_speed of the addr "<<d1_it->first<<" is : "<<othersum_speed<<endl;
        }
        else
        {
            EV<<"multipath_control(), sum_speed of the adding vp addr "<<addr<<" is :"<<sum_speed<<endl;
        }
    }


    cancelEvent(sendcredit); // The previous sendcredit pck scheduling for other flows.
    EV <<"minid = "<<minid<<endl;
    delayMap[addr] = receiver_mpMap.find(addr)->second.next_time[minid];
    find_nextaddr();
    last_cdt_time = simTime();
    delta_cdt_time = next_cdt_delay;
    scheduleAt(simTime()+next_cdt_delay,sendcredit);
    }

    ////////// multi_path algorithm 1 ///////////
    if (mp_algorithm == 1)
    {
        int vpaths = receiver_mpMap.find(addr)->second.virtualpaths; // the current number of virtual paths.
        int osum_paths = vpaths; // the original number of virtual paths before adjusting.
        receiver_mpinfo mpinfo = receiver_mpMap.find(addr)->second;
        receiver_flowinfo flowinfo = receiver_flowMap.find(addr)->second;
        int newpathid = 0;
        int onew_pathid = newpathid; // the original id of the new added virtual path.
        int minid = 0;
        simtime_t mint = 10^10;

        EV<<"multipath_control(), initial vpaths = "<<vpaths<<endl;

        if (vpaths < 256)
        {
            mpinfo.virtualpaths = vpaths + 1;
            mpinfo.path_id[vpaths] = rand()%256;
            mpinfo.speed[vpaths] = max_sumspeed - speed;
            mpinfo.next_time[vpaths] = credit_size/(max_sumspeed-speed);
            mint = credit_size/(max_sumspeed-speed);
            minid = vpaths;
            newpathid = vpaths;
            onew_pathid = newpathid;
            vpaths++;
            osum_paths = vpaths;
        }
        else
        {
            int minsid = 0;
            int     minspeed = 10^10;
            for(int     i=0;i<256;i++)
            {
                if(mpinfo.speed[i]<minspeed)
                {
                    minspeed = mpinfo.speed[i];
                    minsid = i;
                }
            }

            EV<<"multipath_control(), virtual paths are more than 256, delete the slowest one :"<<minsid<<endl;
            speed = speed - minspeed;
            for (int j=0;j<minsid;j++)
            {
                mpinfo.speed[j] = mpinfo.speed[j]*(speed/max_sumspeed);
                mpinfo.next_time[j] = mpinfo.next_time[j]*(max_sumspeed/speed);
            }
            for (int j=minsid;j<vpaths;j++)
            {
                mpinfo.path_id[j] = mpinfo.path_id[j+1];
                mpinfo.speed[j] = mpinfo.speed[j+1]*(speed/max_sumspeed);
                mpinfo.next_time[j] = mpinfo.next_time[j+1]*(max_sumspeed/speed);
            }

            mpinfo.path_id[minsid] = rand()%256;
            mpinfo.speed[minsid] = max_sumspeed - speed;
            mpinfo.next_time[minsid] = credit_size/(max_sumspeed-speed);
            mint = credit_size/(max_sumspeed-speed);
            minid = minsid;
            newpathid = minsid;
        }
        // update the receiver_mpMap and delayMap.
        // we adjust the next_time of scheduled address directly at here.
        receiver_mpinfo t_mpinfo;
        int t_i = 0;
        double sum_speed = 0;
        for(int i=0;i<osum_paths;i++)
        {
            if (mpinfo.speed[i]<max_sumspeed/100)
            {
                if (i!=onew_pathid)
                {
                    t_mpinfo.speed[onew_pathid] = mpinfo.speed[onew_pathid] + mpinfo.speed[i];
                    t_mpinfo.next_time[onew_pathid] = credit_size/(mpinfo.speed[onew_pathid]);
                    mint = mpinfo.next_time[onew_pathid];
                    speed = speed - mpinfo.speed[i];
                    vpaths = vpaths - 1;
                    if (onew_pathid>i)
                    {
                        newpathid = newpathid - 1; // the new id of added vp has been minus by 1;
                    }
                    EV <<"multipath_control(), delete the low speed virtual path: "<<addr<<"__"<<i<<", speed = "<<mpinfo.speed[i]<<", sum of vps = "<<vpaths<<endl;
                }
                else
                {
                    speed = speed - mpinfo.speed[i];
                    vpaths = vpaths - 1;
                }
            }
            else
            {
                t_mpinfo.path_id[t_i] = mpinfo.path_id[i];
                t_mpinfo.next_time[t_i] = mpinfo.next_time[i];
                t_mpinfo.speed[t_i] = mpinfo.speed[i];
                EV <<"multipath_control(), before adjust speed, the next time of "<<addr<<"__"<<t_i<<" = "<<t_mpinfo.next_time[t_i]<<", speed = "<<t_mpinfo.speed[t_i]<<endl;
                t_i++;
            }
        }

        t_mpinfo.virtualpaths = vpaths;
        assert(vpaths==t_i);

        for(int i=0;i<vpaths;i++)
        {
            if(i != newpathid)
            {
                assert(speed!=0 && oldspeed!=0);
                t_mpinfo.speed[i] = t_mpinfo.speed[i]*(speed/oldspeed);
                t_mpinfo.next_time[i] = (t_mpinfo.next_time[i] - (simTime() - last_cdt_time))*(oldspeed/speed);
                EV <<"multipath_control(), after adjusting, the next time of "<<addr<<"__"<<i<<" = "<<t_mpinfo.next_time[i]<<", speed = "<<t_mpinfo.speed[i]<<endl;
                if (t_mpinfo.next_time[i] < mint)
                {
                    mint = t_mpinfo.next_time[i];
                    minid = i;
                }
                sum_speed = sum_speed + t_mpinfo.speed[i];
            }
            else
            {
                sum_speed = sum_speed + t_mpinfo.speed[i];
                EV <<"multipath_control(), after adjusting, the next time of new added vp "<<addr<<"__"<<i<<" = "<<t_mpinfo.next_time[i]<<", speed = "<<t_mpinfo.speed[i]<<endl;
            }
        }
        receiver_mpMap[addr] = t_mpinfo;

        EV<<"multipath_control() completed, virtualpaths = "<<receiver_mpMap.find(addr)->second.virtualpaths<<endl;

        auto d1_it = delayMap.begin();
        for(; d1_it != delayMap.end(); ++ d1_it)
        {
            if (d1_it->first != addr)
            {
                double othersum_speed = 0;
                delayMap[d1_it->first] = d1_it->second - simTime() + last_cdt_time;
                receiver_mpinfo tmpinfo = receiver_mpMap.find(d1_it->first)->second;
                for (int i=0; i<tmpinfo.virtualpaths; i++)
                {
                    othersum_speed = othersum_speed + tmpinfo.speed[i];
                    tmpinfo.next_time[i] = tmpinfo.next_time[i] - simTime() + last_cdt_time;
                    receiver_mpMap[d1_it->first] = tmpinfo;
                    EV<<"0000 reset the next_time of "<<d1_it->first<<"__"<<i<<" = "<<receiver_mpMap.find(d1_it->first)->second.next_time[i]<<", speed = "<<receiver_mpMap.find(d1_it->first)->second.speed[i]<<", path id = "<<tmpinfo.path_id[i]<<endl;
                }
                EV<<"multipath_control(), othersum_speed of the addr "<<d1_it->first<<" is : "<<othersum_speed<<endl;
            }
            else
            {
                EV<<"multipath_control(), sum_speed of the adding vp addr "<<addr<<" is :"<<sum_speed<<endl;
            }
        }
        cancelEvent(sendcredit); // The previous sendcredit pck scheduling for other flows.
        EV <<"minid = "<<minid<<endl;
        delayMap[addr] = receiver_mpMap.find(addr)->second.next_time[minid];
        find_nextaddr();
        last_cdt_time = simTime();
        delta_cdt_time = next_cdt_delay;
        scheduleAt(simTime()+next_cdt_delay,sendcredit);
    }
}
} // namespace inet


