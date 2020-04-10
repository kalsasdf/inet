// Copyright (C) 2018 Angrydudu

#include "xpassCore.h"
#include "inet/common/INETDefs.h"


namespace inet {

Define_Module(xpassCore);

simsignal_t xpassCore::queueLengthSignal = registerSignal("queueLength");

void xpassCore::initialize()
{
    //statistics
    emit(queueLengthSignal, int(data_Map.size()));
    outGate = gate("lowerOut");
    inGate = gate("lowerIn");
    upGate = gate("upperOut");
    downGate = gate("upperIn");
    // configuration
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

    EV<< activate<<"  "<<linkspeed<<endl;

    if (activate)
    {
        sender_flows = 0;
        receiver_flows = 0;
        temp_info = new Packet();
        stopcdt_info = new Packet();
        sendcredit = new cMessage("sendcred", SENDCRED);
        stopcredit = new cMessage("stopcred", STOPCRED);

        EV_INFO <<"0000 linkspeed = "<< linkspeed<<endl;
        // state
        registedCredits = 0;
        WATCH(registedCredits);

        // credit state
        CreState = INITIAL_STATE;

        credit_size = 64;
        // The parameters of feedback control
        targetratio = 0.2;
        previousincrease = false;
        wmax = 0.5;
        wmin = 0.01;
        w = 1/16;
        maxrate = 0.04*linkspeed; // The max link capacity for transmitting credits.
        currate = maxrate/32;
        max_lossratio = 0.5;
        // Initialize the credit sequence number of the receiver

        delta_cdt_time = credit_size/currate;
        last_cdt_time = 0;

        // Used for judging the end of flow.
        this->max_idletime = bits_timeout/linkspeed;

        treceiver_flowinfo = {simTime(),0,0,1,1,1,0,currate};
        tsender_flowinfo = {simTime()};

        // statistics
        numMapReceived = 0;
        numMapDropped = 0;
        WATCH(numMapReceived);  // By using WATCH() function, we can see the value in the simulator.
        WATCH(numMapDropped);
    }
}

void xpassCore::handleMessage(cMessage *msg)
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

void xpassCore::handleSelfMessage(cMessage *pck)
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

void xpassCore::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "q rcvd: %d\nq dropped: %d", numMapReceived, numMapDropped);
    getDisplayString().setTagArg("t", 0, buf);
}

// Process the message from lower layer.
void xpassCore::processUpperpck(Packet *pck)
{
    if(!strcmp(pck->getFullName(),"SYN"))
    {
        sender_flows++;
        auto l3AddressReq = pck->addTagIfAbsent<L3AddressReq>();
        sender_flowinfo sndflow = tsender_flowinfo;
        sndflow.cretime = simTime();
        sender_flowMap[l3AddressReq->getDestAddress()] = sndflow;
        EV << "0000 send SYN down, sender flow map size = "<< sender_flowMap.size()<<endl;
        sndsrc = l3AddressReq->getSrcAddress();
        L3Address snddest = l3AddressReq->getDestAddress();

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
            EV<< "00000000 pck requested > 0, = "<< registedCredits << " ! 00000000"<< endl;
            Packet *credit = extractCredit(tdest);
            const auto& data_head = makeShared<Ipv4Header>();
            const auto& credit_head = credit->peekAtFront<Ipv4Header>();
            data_head->setChunkLength(b(32));
            data_head->setIdentification(credit_head->getIdentification());
            pck->insertAtFront(data_head);
            pck->setTimestamp(simTime());
            pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x0A);

            delete credit;
            sendDown(pck);
            registedCredits--;
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
        pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x12);
        sendDown(pck);
    }
    else if(!strcmp(pck->getFullName(),"RST")||!strcmp(pck->getFullName(),"FIN"))
    {
        pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x12);
        sendDown(pck);
    }
    else
    {
        sendDown(pck);
    }
}

// Process the Packet from upper layer.
void xpassCore::processLowerpck(Packet *pck)
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
        get_singleRTT(pck);
        receive_data(pck);
    }
    else
    {
        sendUp(pck);
    }
}

Packet *xpassCore::findData(L3Address addr)
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

Packet *xpassCore::extractData(L3Address addr)
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

Packet *xpassCore::insertData(L3Address addr,Packet *pck)
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

Packet *xpassCore::extractCredit(L3Address addr)
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

Packet *xpassCore::findCredit(L3Address addr)
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

Packet *xpassCore::insertCredit(L3Address addr,Packet *pck)
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

Packet *xpassCore::newflowinfo(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressReq>();
    Packet *info = new Packet;
    info->addTagIfAbsent<L3AddressReq>()->setSrcAddress(l3addr->getSrcAddress());
    info->addTagIfAbsent<L3AddressReq>()->setDestAddress(l3addr->getDestAddress());

    return info;
}

int xpassCore::deleteFlowCredits(L3Address addr)
{
    int sum_deleted;
    sum_deleted = credit_Map.count(addr);
    credit_Map.erase(addr);
    return sum_deleted;
}

void xpassCore::sendDown(Packet *pck)
{
    EV << "00000000 Oh send down, data map size = " << data_Map.size() << " ! 00000000"<<endl;
    send(pck, outGate);
}

void xpassCore::sendUp(Packet *pck)
{
    EV << "oh sendup!"<< endl;
    send(pck, upGate);
}

void xpassCore::send_credreq(L3Address destaddr)
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
void xpassCore::send_credit(L3Address destaddr,int seq)
{
    Packet *credit = new Packet("credit");

    const auto& content = makeShared<Ipv4Header>();
    content->setChunkLength(b(32));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
    content->setIdentification(seq);
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
void xpassCore::send_stop(L3Address addr)
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
void xpassCore::receive_credreq(Packet *pck)
{
    receiver_flows++;    // This number is dynamically changing with the number of flows;

    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    rcvsrc = l3addr->getDestAddress();

    receiver_flowinfo rcvflow = treceiver_flowinfo;
    rcvflow.nowRTT = 2*(simTime() - pck->getTimestamp());
    rcvflow.last_Fbtime = simTime();
    rcvflow.pck_in_rtt = 1;
    temp_info = newflowinfo(pck);

    EV <<"0000 current rate = "<< currate<<endl;
    receiver_flowMap[l3addr->getSrcAddress()] = rcvflow;

    EV <<"0000 receiver_flow = "<< receiver_flows<<endl;
    // Only the first flow should do the following steps.
    if (receiver_flows == 1)
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
        for(; d1_it != delayMap.end(); ++ d1_it)
        {
            delayMap[d1_it->first] = d1_it->second - simTime() + last_cdt_time;
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
void xpassCore::receive_credit(Packet *pck)
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
        sender_flows--;
        send_stop(stopaddr);
        EV << "0000 sender flow map size = "<< sender_flowMap.size()<<endl;
    }
    // Send data packet according to the credit
    Packet *data = extractData(l3addr->getSrcAddress());
    if (data == nullptr) {
        insertCredit(l3addr->getSrcAddress(),pck);
        registedCredits++;
        EV << "00000000 packets allowed to be sent = "<< registedCredits << endl;
    }
    else {
        const auto& data_head = makeShared<Ipv4Header>();
        const auto& credit_head = pck->peekAtFront<Ipv4Header>();
        data_head->setChunkLength(b(32));
        data_head->setIdentification(credit_head->getIdentification());
        data->insertAtFront(data_head);
        data->setTimestamp(simTime());
        data->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x0A);

        delete pck;
        sendDown(data);
    }
}

// Receive the stop credit Packet from sender, and stop sending credit.
void xpassCore::receive_stopcred(Packet *pck)
{
    EV<<"0000 entering receivestop_cdt"<<endl;
    stop_addr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    scheduleAt(simTime(),stopcredit); //schedule selfmessage
}

// Receive the TCP segment, determine the credit loss ratio and enter the feedback control.
void xpassCore::receive_data(Packet *pck)
{
    // If receive a TCP segment, determine which flow the segment belongs to.
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    EV <<"00000000 The src address of the segment = "<< l3addr->getSrcAddress().toIpv4()<<" ! 00000000";

    // If the segment is not recognizable, directly pass it.
    auto it = receiver_flowMap.find(l3addr->getSrcAddress());
    const auto& data_head = pck->removeAtFront<Ipv4Header>();

    if (it!=receiver_flowMap.end() && data_head->getIdentification() < it->second.creditseq)
    if (it!=receiver_flowMap.end())
    {
        receiver_flowinfo tinfo = it->second;
        tinfo.nowseq = data_head->getIdentification();
        // Calculate the credit loss ratio, and enter the feedback control.
        if ((simTime()-tinfo.last_Fbtime) < tinfo.nowRTT)
        {
            tinfo.pck_in_rtt++;
            receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
        }
        else
        {
            tinfo.sumlost = tinfo.nowseq - tinfo.lastseq + 1 - tinfo.pck_in_rtt;
            if(tinfo.sumlost >= 0)
            {
                EV <<" 00000000 Enter feedback control of No." << it->first << " flow 00000000"<<" 00000000 last seq = " << tinfo.lastseq << ", now seq = "<< tinfo.nowseq <<endl;
                tinfo.newspeed = feedback_control(tinfo.newspeed, tinfo.sumlost,tinfo.pck_in_rtt+tinfo.sumlost);
                tinfo.nowRTT = 2*(simTime() - pck->getTimestamp());
                tinfo.last_Fbtime = simTime();
            }
            tinfo.sumlost = 0;
            tinfo.lastseq = tinfo.nowseq;
            receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
        }
    }
    sendUp(pck);
}

// Handle the self message: send credit.
void xpassCore::self_send_credit()
{
    if (CreState != STOP_CREDIT_STATE)
    {
        delta_cdt_time = simTime() - last_cdt_time;

        EV <<"0000 time gap between sending credits = "<< delta_cdt_time << endl;
        schedule_next_credit(delta_cdt_time);
    }
}

// Handle the self message stop credit.
void xpassCore::self_stop_credit()
{
    EV<<"0000 entering self_stop_credit"<<endl;

    receiver_flowMap.erase(stop_addr);
    delayMap.erase(stop_addr);
    registedCredits = registedCredits - deleteFlowCredits(stop_addr);
    receiver_flows--;

    EV << "00000000 receiver_flows = "<<receiver_flows<<" ! 00000000"<<endl;
    if (receiver_flows == 0)
    {
        CreState = STOP_CREDIT_STATE;
    }

    // re-schedule the credit_send.
    auto d1_it = delayMap.begin();
    for(; d1_it != delayMap.end(); ++ d1_it)
    {
        delayMap[d1_it->first] = d1_it->second - simTime() + last_cdt_time;
    }
    cancelEvent(sendcredit); // The previous sendcredit pck scheduling for other flows.
    find_nextaddr();
    last_cdt_time = simTime();
    scheduleAt(simTime() + next_cdt_delay,sendcredit);
}

// Do credit feedback control.
double xpassCore::feedback_control(double speed, int sumlost, int sumcredits)
{
    double lossratio;
    lossratio = double(sumlost)/double(sumcredits);
    EV << "00000000 sumlost = "<< sumlost << ", sumcredits = "<< sumcredits<<" ! 00000000";
    EV << "00000000 loss ratio = "<< lossratio << " ! 00000000";
    if(lossratio<=targetratio)
    {
        if(previousincrease)
        {
            w = (w+wmax)/2;
            speed = (1-w)*speed + w*maxrate*(1+targetratio);
        }
        previousincrease = true;
    }
    else if(lossratio<=max_lossratio)
    {
        speed = speed*(1-lossratio)*(1+targetratio);
        if (w/2>wmin)
            w = w/2;
        else
            w = wmin;
    }
    else
    {
        speed = speed*(1-max_lossratio)*(1+targetratio);
        if (w/2>wmin)
            w = w/2;
        else
            w = wmin;
    }
    return speed;
}

void xpassCore::schedule_next_credit(simtime_t delta_t)
{
    //simtime_t min_t= 100000;

    auto f_it = receiver_flowMap.find(next_creditaddr);
    receiver_flowinfo f_info = f_it->second;
    send_credit(next_creditaddr,f_info.creditseq);

    // send the current credit before schedule the next credit.
////
    last_cdt_time = simTime();

    f_info.creditseq = f_info.creditseq + 1;
    receiver_flowMap[next_creditaddr] = f_info;

    EV<<"0000 the delay map size = "<<delayMap.size()<<endl;

    delayMap[next_creditaddr] = delta_t + credit_size/receiver_flowMap.find(next_creditaddr)->second.newspeed;

    auto d_it = delayMap.begin();
    for(; d_it != delayMap.end(); ++ d_it)
    {
        delayMap[d_it->first] = d_it->second - delta_t;
    }

    find_nextaddr();

    EV <<" 00000000 The next delay = "<<next_cdt_delay<<", the next scheduled packet "<<next_creditaddr<< ", seq = "<<receiver_flowMap.find(next_creditaddr)->second.creditseq<<endl;
    delta_cdt_time = next_cdt_delay;
    scheduleAt(simTime()+next_cdt_delay,sendcredit);
}

void xpassCore::find_nextaddr()
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

simtime_t xpassCore::get_singleRTT(Packet *pck)
{
    return simTime() - pck->getTimestamp();
}

} // namespace inet


