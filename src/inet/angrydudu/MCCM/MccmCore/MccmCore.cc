// Copyright (C) 2018 Angrydudu

#include "MccmCore.h"

#include "inet/common/INETDefs.h"


namespace inet {

Define_Module(MccmCore);

simsignal_t MccmCore::queueLengthSignal = registerSignal("queueLength");
simsignal_t MccmCore::ooodegreeSignal = registerSignal("oooDegree");

void MccmCore::initialize()
{
    //statistics
    max_ooo = 0;

    OoO_Vector.setName("ooo degree");
    emit(queueLengthSignal, int(data_Map.size()));
    emit(ooodegreeSignal, int(receiver_orderMap.size()));
    outGate = gate("lowerOut");
    inGate = gate("lowerIn");
    upGate = gate("upperOut");
    downGate = gate("upperIn");
    // configuration
    multipath = par("enableMP");
    sumpaths = par("sumpaths");
    mp_algorithm = par("mp_algorithm");
    activate = par("activate");
    linkspeed = par("linkspeed");
    bits_timeout = par("bits_timeout");
    useECN = par("useECN");
    useRe_order = par("ReOrder");
    rate_control = par("ratecontrol");
    randomspreading = par("randomspreading");
    sum_tenants = par("TenantsNumber");

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
    receiver_orderMap.clear();
    receiver_seqCache.clear();

    EV<< activate<<"  "<<linkspeed<<endl;

    temp_info = new Packet();
    stopcdt_info = new Packet();
    sendcredit = new cMessage("sendcred", SENDCRED);
    stopcredit = new cMessage("stopcred", STOPCRED);
    // state

    frameCapacity = 10000;
    // credit state
    CreState = INITIAL_STATE;

    credit_size = 76*8;
    // The parameters of feedback control
    alpha = 0.5;
    if (useECN)
    {
        targetratio = 0.125;
    }
    else
    {
        targetratio = 0.125;
    }
    wmax = 0.5;
    wmin = 0.01;

    if (multipath)
    {
        maxrate = 0.05*linkspeed; // The max link capacity for transmitting credits. 0.41%
        currate = maxrate;
    }
    else
    {
        maxrate = 0.05*linkspeed; // The max link capacity for transmitting credits. 0.41%
        currate = maxrate/16;
    }
    if (!rate_control)
    {
        currate = maxrate;
    }
    max_lossratio = 0.9;
    // Initialize the credit sequence number of the receiver

    // ecnbased control
    gama = 0.0625;

    delta_cdt_time = credit_size/currate;
    last_cdt_time = 0;


    // Used for judging the end of flow.
    this->max_idletime = bits_timeout/linkspeed;
    // this->max_idletime = 0.002;
    treceiver_flowinfo = {simTime(),0,0,1,0,0,0,0.125,0,0,currate,currate,currate,false};
    tsender_flowinfo = {simTime(),0,false};

        // statistics
    numMapReceived = 0;
    numMapDropped = 0;
    WATCH(numMapReceived);  // By using WATCH() function, we can see the value in the simulator.
    WATCH(numMapDropped);
}

void MccmCore::handleMessage(cMessage *msg)
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

void MccmCore::handleSelfMessage(cMessage *pck)
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

void MccmCore::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "q rcvd: %d\nq dropped: %d", numMapReceived, numMapDropped);
    getDisplayString().setTagArg("t", 0, buf);
}

// Process the message from lower layer.
void MccmCore::processUpperpck(Packet *pck)
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
        if (info.stop_sent)
        {
            info.stop_sent = false;
            sender_flowMap[tdest] = info;
            send_credreq(tdest);
        }

        if (findCredit(tdest) != nullptr)
        {
            Packet *credit = extractCredit(tdest);

            sender_flowinfo snd_info = sender_flowMap.find(tdest)->second;
            long int seqno = snd_info.seq_No + 1;
            snd_info.seq_No = seqno;
            sender_flowMap[tdest] = snd_info;

            EcnInd *ecnInd = credit->removeTagIfPresent<EcnInd>();
            const auto& data_head = makeShared<Ipv4Header>();
            const auto& credit_head = credit->removeAtFront<Ipv4Header>();
            data_head->setChunkLength(b(32));
            if (useECN)
            {
                data_head->setIdentification(seqno);
            }
            else
            {
                data_head->setIdentification(credit_head->getIdentification());
            }
            data_head->setDiffServCodePoint(11);
            data_head->setExplicitCongestionNotification(ecnInd->getExplicitCongestionNotification());
            pck->insertAtFront(data_head);
            pck->setSchedulingPriority(credit->getSchedulingPriority());
            pck->setTimestamp(simTime());
            //pck->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x0A);
            if (credit_head->getFragmentOffset()-1 <= 0)
            {
                delete credit;
            }
            else
            {
                const auto& newhead = makeShared<Ipv4Header>();
                newhead->setFragmentOffset(credit_head->getFragmentOffset() - 1);
                newhead->setIdentification(credit_head->getIdentification());
                newhead->enableImplicitChunkSerialization = true;
                //pck->removeAtFront<Ipv4Header>();
                credit->insertAtFront(newhead);
                credit->addTagIfAbsent<EcnInd>()->setExplicitCongestionNotification(ecnInd->getExplicitCongestionNotification());
                EV<<"processupper_pck(), credit->getFragmentOffset() = "<< credit->peekAtFront<Ipv4Header>()->getFragmentOffset() <<endl;
                insertCredit(l3addr->getDestAddress(),credit);
            }
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
        if (randomspreading)
            pck->setSchedulingPriority(rand()%9);
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
void MccmCore::processLowerpck(Packet *pck)
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

Packet *MccmCore::findData(L3Address addr)
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

Packet *MccmCore::extractData(L3Address addr)
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

Packet *MccmCore::insertData(L3Address addr,Packet *pck)
{

    if (frameCapacity && int(data_Map.size()) >= frameCapacity)
    {
        EV << "Map full, dropping packet.\n";
        return pck;
    }
    else
    {
        data_Map.insert(pair<L3Address,Packet*>(addr,pck));
        emit(queueLengthSignal, int(data_Map.size()));
        EV << "0000 data packet has been inserted into the map, map size = "<<data_Map.size()<<endl;
        return nullptr;
    }
}

Packet *MccmCore::extractCredit(L3Address addr)
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

Packet *MccmCore::findCredit(L3Address addr)
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

Packet *MccmCore::insertCredit(L3Address addr,Packet *pck)
{
if (useECN)
{
    if (int(credit_Map.size()) >= 10)
    {
        auto it = credit_Map.begin();
        credit_Map.erase(it);
        credit_Map.insert(pair<L3Address,Packet*>(addr,pck));
        EV << "insertCredit(), delete the oldest added credit"<< endl;
        return nullptr;
    }
    else
    {
        credit_Map.insert(pair<L3Address,Packet*>(addr,pck));
        return nullptr;
    }
}
else
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
}

Packet *MccmCore::newflowinfo(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressReq>();
    Packet *info = new Packet;
    info->addTagIfAbsent<L3AddressReq>()->setSrcAddress(l3addr->getSrcAddress());
    info->addTagIfAbsent<L3AddressReq>()->setDestAddress(l3addr->getDestAddress());

    return info;
}

int MccmCore::deleteFlowCredits(L3Address addr)
{
    int sum_deleted;
    sum_deleted = credit_Map.count(addr);
    credit_Map.erase(addr);
    return sum_deleted;
}

void MccmCore::sendDown(Packet *pck)
{
    EV << "00000000 Oh send down, data map size = " << data_Map.size() << " ! 00000000"<<endl;
    send(pck, outGate);
}

void MccmCore::sendUp(Packet *pck)
{
    EV<<"MccmCore, oh sendup!"<<endl;
    send(pck, upGate);
    //EV<<"MccmCore, oh sendup!"<<endl;
    //send(pck, upGate);
}

void MccmCore::send_credreq(L3Address destaddr)
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
void MccmCore::send_credit(L3Address destaddr,unsigned int pathid, int seq)
{
    Packet *credit = new Packet("credit");

    const auto& content = makeShared<Ipv4Header>();

    srand((unsigned)(SIMTIME_DBL(simTime())*10000000000));
    //srand((int)time(0));
    //srand((unsigned)seq);
    int jitter_bytes = rand()%6;

    EV<<"send_credit, the jitter bytes = "<<jitter_bytes<<endl;
    //jitter_bytes = 4;

    content->setChunkLength(b(208+jitter_bytes*8));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
    content->setIdentification(seq);
    content->setFragmentOffset(sum_tenants); // the value of data packets per credit


    //content->setDiffServCodePoint(rand()%256);
    //content->setDiffServCodePoint(pathid);
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

    //srand((unsigned)(SIMTIME_DBL(simTime())*1000000));
    //srand((unsigned)(SIMTIME_DBL(simTime())*10000000000));
    int rand_pathid = rand()%252;
    if (randomspreading)
    {
        credit->setSchedulingPriority(rand_pathid);
    }
    else
    {
        credit->setSchedulingPriority(pathid);
    }
    //credit->setSchedulingPriority(pathid);
    credit->setTimestamp(simTime());
    credit->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    credit->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcvsrc);
    credit->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x2E);
    credit->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    credit->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::tcp);

    EV << "00000000 send credit, DestAddr = "<<destaddr.toIpv4()<<", dscp value = "<<credit->addTagIfAbsent<DscpReq>()->getDifferentiatedServicesCodePoint()<<", random path = "<<rand_pathid%sumpaths<<endl;
    sendDown(credit); // send down the credit

    orderpackets(destaddr);
}

// Generate and send stop credit Packet to the destination.
void MccmCore::send_stop(L3Address addr)
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
void MccmCore::receive_credreq(Packet *pck)
{
    //currate = maxrate/(receiver_flowMap.size()+1);   // Change the current max credit rate of each flows.
    if (multipath)
    {
        currate = maxrate;
    }


    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    rcvsrc = l3addr->getDestAddress();

    // initialize the seqCache.
    if (useRe_order)
    {
        auto seqit = receiver_seqCache.find(l3addr->getSrcAddress());
        if(seqit == receiver_seqCache.end())
        {
            receiver_seqCache[l3addr->getSrcAddress()] = 0;
        }
    }

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
    rcvmp.speed[0] = rcvflow.newspeed;
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
void MccmCore::receive_credit(Packet *pck)
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
        if((simTime()-info.cretime)>= max_idletime && findData(it->first) == nullptr)
        {
            stopaddr = it->first;
            stop = true;
        }
    }
    if(stop)
    {
        EV << "0000 lets delete flow: "<<sender_flowMap.find(stopaddr)->first<<", the sender seq = "<<sender_flowMap.find(stopaddr)->second.seq_No<<endl;
        sender_flowinfo sndinfo = sender_flowMap.find(stopaddr)->second;
        if (!sndinfo.stop_sent)
        {
            send_stop(stopaddr);
        }
        sndinfo.stop_sent = true;
        sender_flowMap[stopaddr] = sndinfo;
    }
    else
    {
        int tempi = pck->peekAtFront<Ipv4Header>()->getFragmentOffset();
        EV<<"receive_credit(), initial credit->getFragmentOffset() = "<< tempi <<endl;
        for (int i=0;i<tempi;i++)
        {
            // Send data packet according to the credit
            Packet *data = extractData(l3addr->getSrcAddress());
            if (data == nullptr) {
                if (sender_flowMap.find(l3addr->getSrcAddress()) != sender_flowMap.end())
                {
                    insertCredit(l3addr->getSrcAddress(),pck); // insert the credit, break the 'for';
                }
                else
                    delete pck;
                EV << "00000000 packets allowed to be sent = "<< credit_Map.size() << endl;
                break;
            }
            else {
                sender_flowinfo snd_info = sender_flowMap.find(l3addr->getSrcAddress())->second;
                long int seqno = snd_info.seq_No + 1;
                snd_info.seq_No = seqno;
                sender_flowMap[l3addr->getSrcAddress()] = snd_info;
                EcnInd *ecnInd = pck->removeTagIfPresent<EcnInd>();
                const auto& data_head = makeShared<Ipv4Header>();
                data_head->setChunkLength(b(32));
                const auto& credit_head = pck->removeAtFront<Ipv4Header>();
                if (useECN)
                {
                    EV<<" use re-ordering seq, the seq of this packet = "<< seqno <<endl;
                    data_head->setIdentification(seqno);
                }
                else
                {
                    data_head->setIdentification(credit_head->getIdentification());
                }
                data_head->setDiffServCodePoint(11); // determine the data packets is scheduled by MccmCore
                data_head->setExplicitCongestionNotification(ecnInd->getExplicitCongestionNotification());
                data->insertAtFront(data_head);
                data->setTimestamp(simTime());
                data->setSchedulingPriority(pck->getSchedulingPriority());

                const auto& newhead = makeShared<Ipv4Header>();
                newhead->setFragmentOffset(credit_head->getFragmentOffset() - 1);
                newhead->setIdentification(credit_head->getIdentification());
                newhead->enableImplicitChunkSerialization = true;
                //pck->removeAtFront<Ipv4Header>();
                pck->insertAtFront(newhead);
                pck->addTagIfAbsent<EcnInd>()->setExplicitCongestionNotification(ecnInd->getExplicitCongestionNotification());

                EV<<"receive_credit(), credit->getFragmentOffset() = "<< pck->peekAtFront<Ipv4Header>()->getFragmentOffset() <<endl;
                //data->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x0A);
                sendDown(data);
                if (pck->peekAtFront<Ipv4Header>()->getFragmentOffset() <= 0)
                {
                    delete pck;
                    break;
                }
            }
        }
    }
}

// Receive the stop credit Packet from sender, and stop sending credit.
void MccmCore::receive_stopcred(Packet *pck)
{
    EV<<"0000 entering receivestop_cdt"<<endl;
    stop_addr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    scheduleAt(simTime(),stopcredit); //schedule selfmessage
}

// Receive the TCP segment, determine the credit loss ratio and enter the feedback control.
void MccmCore::receive_data(Packet *pck)
{
    // If receive a TCP segment, determine which flow the segment belongs to.
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    EV <<"00000000 The src address of the segment = "<< l3addr->getSrcAddress().toIpv4()<<" ! 00000000"<<endl;

    // If the segment is not recognizable, directly pass it.
    auto it = receiver_flowMap.find(l3addr->getSrcAddress());

    const auto& data_head = pck->removeAtFront<Ipv4Header>();

    if (data_head->getDiffServCodePoint() == 11)// the packet is controlled by angry credits
    {
    if (it!=receiver_flowMap.end())
    {
        if (useECN) // use ecn for rate control
        {
            receiver_flowinfo tinfo = it->second;
            EV<<"receive_data(), now the RTT = "<<tinfo.nowRTT<<", the time gap from last ECN-based rate control = "<< simTime()-tinfo.last_Fbtime <<endl;
            // Calculate the credit loss ratio, and enter the feedback control.
            if ((simTime()-tinfo.last_Fbtime) < 1*tinfo.nowRTT)
            {
                tinfo.pck_in_rtt++;
                if (data_head->getExplicitCongestionNotification() == 3||pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification() == 3)
                {
                    tinfo.ecn_in_rtt++;
                }
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
            }
            else
            {
                tinfo.pck_in_rtt++;
                if (data_head->getExplicitCongestionNotification() == 3)
                {
                    tinfo.ecn_in_rtt++;
                }
                EV<<"receive_data(), ecn_in_rtt = "<<tinfo.ecn_in_rtt<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
                if (!multipath)
                {
                    double oldspeed = tinfo.newspeed;
                    EV<<"receive_data(), before ecn control, speed = "<<oldspeed<<endl;
                    tinfo = ecnbased_control(tinfo,false);
                    EV<<"receive_data(), after ecn control, speed = "<<tinfo.newspeed<<endl;
                    receiver_mpinfo mpinfo = receiver_mpMap.find(it->first)->second;
                    for (int i=0;i<mpinfo.virtualpaths;i++)
                    {
                        mpinfo.speed[i] = mpinfo.speed[i]*tinfo.newspeed/oldspeed;
                    }
                    receiver_mpMap[it->first] = mpinfo;
                    receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                    EV <<"receive_data() no multipath, new speed in mpMap = "<< receiver_mpMap.find(it->first)->second.speed[0] <<endl;
                }
                else
                {
                    if (mp_algorithm == 0)
                    {
                        EV <<"receive_data() Al-0, Enter ecn-based control of No." << it->first << " flow."<<endl;
                        double oldspeed = tinfo.newspeed;
                        EV <<"receive_data() Al-0, new speed in flow map = "<< tinfo.newspeed <<", maxrate = "<<maxrate<<endl;
                        //int virtualpath = receiver_mpMap.find(it->first)->second.virtualpaths;
                        if(double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt) > targetratio*2)
                        {
                            tinfo = ecnbased_control(tinfo,false);
                            multipath_control(it->first,tinfo.newspeed,oldspeed,maxrate);
                            tinfo.newspeed = maxrate;
                        }
                    }
                    if (mp_algorithm == 1)
                    {
                        EV <<"receive_data() Al-1, Enter ecn-based control of No." << it->first << " flow, receive_data(), ecn_in_rtt = "<<tinfo.ecn_in_rtt<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
                        double oldspeed = tinfo.newspeed;
                        receiver_mpinfo tmpinfo = receiver_mpMap.find(it->first)->second;
                        double old_sumrate = tinfo.max_speedsum;
                        double temp_sum = 0; // to record the multipath map
                        // new added
                        if (double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt) > targetratio)
                        {
                            tinfo = ecnbased_control(tinfo,false);
                            EV <<"receive_data() Al-1, , old speed in flow map = "<< oldspeed <<", newspeed = "<<tinfo.newspeed<<endl;
                            multipath_control(it->first,tinfo.newspeed,oldspeed,old_sumrate);
                            tmpinfo = receiver_mpMap.find(it->first)->second;
                            tinfo = ecnbased_control(tinfo,true);
                            EV<<"receive_data() Al-1, changing max_speedsum, old one = "<<old_sumrate<<", new one = "<<tinfo.max_speedsum<<endl;
                            for (int i=0;i<tmpinfo.virtualpaths;i++)
                            {
                                tmpinfo.speed[i] = tmpinfo.speed[i] * tinfo.max_speedsum/old_sumrate;
                                tmpinfo.next_time[i] = (tmpinfo.next_time[i] - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                                temp_sum = tmpinfo.speed[i] + temp_sum;
                            }
                            EV<<"after ecn and multipath control, sum speed = "<< temp_sum<<endl;
                            delayMap[it->first] = (delayMap.find(it->first)->second - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                        }
                        else
                        {
                            tinfo = ecnbased_control(tinfo,true);
                            if (tinfo.max_speedsum - old_sumrate > tinfo.max_speedsum/(100*sumpaths))
                            {
                                for (int i=0;i<tmpinfo.virtualpaths;i++)
                                {
                                    tmpinfo.speed[i] = tmpinfo.speed[i]*tinfo.max_speedsum/old_sumrate;
                                    tmpinfo.next_time[i] = (tmpinfo.next_time[i] - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                                    temp_sum = tmpinfo.speed[i] + temp_sum;
                                }
                                EV<<"after ecn and multipath control, sum speed = "<< temp_sum<<endl;
                                delayMap[it->first] = (delayMap.find(it->first)->second - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                            }
                            else
                            {
                                tinfo.max_speedsum = old_sumrate;
                                for (int i=0;i<tmpinfo.virtualpaths;i++)
                                {
                                    tmpinfo.speed[i] = tmpinfo.speed[i];
                                    tmpinfo.next_time[i] = (tmpinfo.next_time[i] - (simTime() - last_cdt_time));
                                    temp_sum = tmpinfo.speed[i] + temp_sum;
                                }
                                EV<<"after ecn and multipath control, sum speed = "<< temp_sum<<endl;
                                delayMap[it->first] = (delayMap.find(it->first)->second - (simTime() - last_cdt_time));
                            }
                        }
                        tinfo.newspeed = tinfo.max_speedsum;
                        receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                        receiver_mpMap[it->first] = tmpinfo;
                        cancelEvent(sendcredit);
                        find_nextaddr();
                        last_cdt_time = simTime();
                        delta_cdt_time = next_cdt_delay;
                        scheduleAt(simTime()+next_cdt_delay,sendcredit);
                    }
                }
                tinfo.nowRTT = times_of_RTT*(simTime() - pck->getTimestamp());
                tinfo.last_Fbtime = simTime();
                tinfo.sumlost = 0;
                tinfo.lastseq = tinfo.nowseq;
                tinfo.pck_in_rtt = 0;
                tinfo.ecn_in_rtt = 0;
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
            }
            if (useRe_order)
            {
                /////////////////////////////////// ordering packets ////////////////////////////////
                auto rcvit = receiver_seqCache.find(l3addr->getSrcAddress());
                if (rcvit->second + 1 == (data_head->getIdentification()))
                {
                    receiver_seqCache[l3addr->getSrcAddress()] = data_head->getIdentification();
                    EV<<"ordering packets directly, now the seq number = "<<rcvit->second<<", The arrived packetId = "<< data_head->getIdentification() <<endl;
                    sendUp(pck);
                    orderpackets(l3addr->getSrcAddress());
                    orderpackets(l3addr->getSrcAddress());
                }
                else if (rcvit->second >= data_head->getIdentification())
                {
                    receiver_seqCache[l3addr->getSrcAddress()] = data_head->getIdentification();
                    EV<<"ordering packets for already rcved seq, now the seq number = "<<rcvit->second<<", The arrived packetId = "<< data_head->getIdentification() <<endl;
                    sendUp(pck);
                }
                else
                {
                    pck->insertAtFront(data_head);
                    receiver_orderMap.insert(pair<L3Address,Packet*>(l3addr->getSrcAddress(),pck));
                    EV << "The arrived packetId = "<< data_head->getIdentification() <<", now seq should be = "<< rcvit->second + 1 <<", insert into the ordering map, size = "<<receiver_orderMap.size()<<endl;
                    orderpackets(l3addr->getSrcAddress());
                    orderpackets(l3addr->getSrcAddress());
                }

                OoO_Vector.recordWithTimestamp(simTime(),int(receiver_orderMap.size()));
                if (receiver_orderMap.size()>max_ooo)
                {
                    max_ooo = receiver_orderMap.size();
                }
            }
            else
            {
                sendUp(pck);
            }
        }
        else //// use lossratio for feedback control
        {
            receiver_flowinfo tinfo = it->second;
            tinfo.nowseq = data_head->getIdentification();
            EV<<"receive_data(), now the one-side RTT = "<<tinfo.nowRTT<<", the time gap from last feedback control = "<< simTime()-tinfo.last_Fbtime <<endl;
            // Calculate the credit loss ratio, and enter the feedback control.
            if ((simTime()-tinfo.last_Fbtime) < 1*tinfo.nowRTT)
            {
                tinfo.pck_in_rtt++;
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
            }
            else
            {
                tinfo.pck_in_rtt++;
                tinfo.sumlost = tinfo.nowseq - tinfo.lastseq - tinfo.pck_in_rtt;
                double lossratio = double(tinfo.sumlost)/double(tinfo.nowseq - tinfo.lastseq);
                EV<<"receive_data(), sumlost = "<<tinfo.sumlost<<", nowseq = "<<tinfo.nowseq<<", lastseq = "<<tinfo.lastseq<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
                if(tinfo.sumlost >= 0) // else: error may occur
                {
                    if (!multipath)
                    {
                        double oldspeed = tinfo.newspeed;
                        tinfo = feedback_control(tinfo,false);
                        receiver_mpinfo mpinfo = receiver_mpMap.find(it->first)->second;
                        for (int i=0;i<mpinfo.virtualpaths;i++)
                        {
                            mpinfo.speed[i] = mpinfo.speed[i]*tinfo.newspeed/oldspeed;
                        }
                        receiver_mpMap[it->first] = mpinfo;
                        EV <<"receive_data() no multipath, new speed in mpMap = "<< receiver_mpMap.find(it->first)->second.speed[0] <<endl;
                        receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                        cancelEvent(sendcredit);
                        find_nextaddr();
                        last_cdt_time = simTime();
                        delta_cdt_time = next_cdt_delay;
                        scheduleAt(simTime()+next_cdt_delay,sendcredit);
                    }
                    else
                    {
                        if (mp_algorithm == 0)
                        {
                            EV <<"receive_data() Al-0, Enter feedback control of No." << it->first << " flow, "<<" last seq = " << tinfo.lastseq << ", now seq = "<< tinfo.nowseq <<endl;
                            EV <<"receive_data() Al-0, received data packets = "<< tinfo.pck_in_rtt <<", old speed in flow map = "<< tinfo.newspeed <<endl;
                            double oldspeed = tinfo.newspeed;
                            EV <<"receive_data() Al-0, new speed in flow map = "<< tinfo.newspeed <<", maxrate = "<<maxrate<<endl;
                            //int virtualpath = receiver_mpMap.find(it->first)->second.virtualpaths;
                            if(lossratio > targetratio) // example: 3/4 for 3 paths
                            {
                                tinfo = feedback_control(tinfo,false);
                                multipath_control(it->first,tinfo.newspeed,oldspeed,maxrate);
                                tinfo.newspeed = maxrate;
                            }
                        }
                        if (mp_algorithm == 1)
                        {
                            EV <<"receive_data() Al-1, Enter feedback control of No." << it->first << " flow, "<<" last seq = " << tinfo.lastseq << ", now seq = "<< tinfo.nowseq <<endl;
                            EV <<"receive_data() Al-1, received data packets = "<< tinfo.pck_in_rtt <<", old speed in flow map = "<< tinfo.newspeed <<endl;
                            double oldspeed = tinfo.newspeed;
                            receiver_mpinfo tmpinfo = receiver_mpMap.find(it->first)->second;
                            double old_sumrate = tinfo.max_speedsum;
                            double temp_sum = 0; // to record the multipath map
                            // new added
                            if (lossratio > targetratio)
                            {
                                tinfo = feedback_control(tinfo,false);
                                multipath_control(it->first,tinfo.newspeed,oldspeed,tinfo.max_speedsum);
                                tmpinfo = receiver_mpMap.find(it->first)->second;
                                tinfo = feedback_control(tinfo,true);
                                for (int i=0;i<tmpinfo.virtualpaths;i++)
                                {
                                    tmpinfo.speed[i] = tmpinfo.speed[i] * tinfo.max_speedsum/old_sumrate;
                                    tmpinfo.next_time[i] = (tmpinfo.next_time[i] - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                                    temp_sum = tmpinfo.speed[i] + temp_sum;
                                }
                                EV<<"after ecn and multipath control, sum speed = "<< temp_sum<<endl;
                                delayMap[it->first] = (delayMap.find(it->first)->second - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                            }
                            else
                            {
                                tinfo = feedback_control(tinfo,true);
                                if (tinfo.max_speedsum - old_sumrate > tinfo.max_speedsum/(10*sumpaths))
                                {
                                    for (int i=0;i<tmpinfo.virtualpaths;i++)
                                    {
                                        tmpinfo.speed[i] = tmpinfo.speed[i]*tinfo.max_speedsum/old_sumrate;
                                        tmpinfo.next_time[i] = (tmpinfo.next_time[i] - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                                        temp_sum = tmpinfo.speed[i] + temp_sum;
                                    }
                                    EV<<"after ecn and multipath control, sum speed = "<< temp_sum<<endl;
                                    delayMap[it->first] = (delayMap.find(it->first)->second - (simTime() - last_cdt_time))*old_sumrate/tinfo.max_speedsum;
                                }
                                else
                                {
                                    tinfo.max_speedsum = old_sumrate;
                                    for (int i=0;i<tmpinfo.virtualpaths;i++)
                                    {
                                        tmpinfo.speed[i] = tmpinfo.speed[i];
                                        tmpinfo.next_time[i] = (tmpinfo.next_time[i] - (simTime() - last_cdt_time));
                                        temp_sum = tmpinfo.speed[i] + temp_sum;
                                    }
                                    EV<<"after ecn and multipath control, sum speed = "<< temp_sum<<endl;
                                    delayMap[it->first] = (delayMap.find(it->first)->second - (simTime() - last_cdt_time));
                                }
                            }
                            tinfo.newspeed = tinfo.max_speedsum;
                            receiver_mpMap[it->first] = tmpinfo;
                            cancelEvent(sendcredit);
                            find_nextaddr();
                            last_cdt_time = simTime();
                            delta_cdt_time = next_cdt_delay;
                            scheduleAt(simTime()+next_cdt_delay,sendcredit);
                            EV<<"receive_data(), after feedback control, lastseq = "<<tinfo.lastseq<<endl;
                        }
                    }
                }
                else
                {
                    EV<<"receive_data(), sumlost < 0, some packets are in flight."<< endl;
                }
                tinfo.nowRTT = times_of_RTT*(simTime() - pck->getTimestamp());
                tinfo.last_Fbtime = simTime();
                tinfo.sumlost = 0;
                tinfo.lastseq = tinfo.nowseq;
                tinfo.pck_in_rtt = 0;
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                }
            sendUp(pck);
            }
        }
    else
    {
        if(useRe_order)
        {
            auto rcvit = receiver_seqCache.find(l3addr->getSrcAddress());
            EV << "The arrived packetId = "<< data_head->getIdentification() <<", now seq should be = "<< rcvit->second + 1 << endl;
            pck->insertAtFront(data_head);
            receiver_orderMap.insert(pair<L3Address,Packet*>(l3addr->getSrcAddress(),pck));
            EV<<", insert into the ordering map, size = "<<receiver_orderMap.size()<<endl;
            orderpackets(l3addr->getSrcAddress());
            orderpackets(l3addr->getSrcAddress());
            OoO_Vector.recordWithTimestamp(simTime(),int(receiver_orderMap.size()));
            if (receiver_orderMap.size()>max_ooo)
            {
                max_ooo = receiver_orderMap.size();
            }
        }
        else
        {
            sendUp(pck);
        }
    }
    }
    else // packet dont controlled by angry credits.
    {
        pck->insertAtFront(data_head);
        sendUp(pck);
    }

    ////
}

void MccmCore::orderpackets(L3Address addr)
{
    // ordering the data packets in the ordering map
    auto it = receiver_orderMap.begin();
    for(; it != receiver_orderMap.end(); ++ it)
    {
        Packet *temp_pck = it->second;
        const auto& tdata_head = temp_pck->removeAtFront<Ipv4Header>();
        if (addr == it->first && tdata_head->getIdentification() == receiver_seqCache.find(addr)->second + 1)
        {
            receiver_seqCache[addr] = tdata_head->getIdentification();
            EV<<"ordering packets again, the now seq number = "<<receiver_seqCache.find(addr)->second<<endl;
            sendUp(temp_pck);
            receiver_orderMap.erase(it);
        }
        else
        {
            temp_pck->insertAtFront(tdata_head);
        }
    }
}

// Handle the self message: send credit.
void MccmCore::self_send_credit()
{
    if (CreState != STOP_CREDIT_STATE)
    {
        delta_cdt_time = simTime() - last_cdt_time;
        EV <<"0000 time gap between sending credits = "<< delta_cdt_time << endl;
        schedule_next_credit(delta_cdt_time);
    }
}

// Handle the self message stop credit.
void MccmCore::self_stop_credit()
{
    EV<<"self_stop_credit(), flows before deleted = "<<receiver_flowMap.size()<<endl;

    receiver_flowMap.erase(stop_addr);
    receiver_mpMap.erase(stop_addr);
    delayMap.erase(stop_addr);


    /*
    if (useRe_order)
    {
        auto it = receiver_orderMap.begin();
        for(; it != receiver_orderMap.end(); ++ it)
        {
            Packet *temp_pck = it->second;
            const auto& tdata_head = temp_pck->removeAtFront<Ipv4Header>();
            if (stop_addr == it->first)
            {
                auto rcv_it = receiver_seqCache.find(stop_addr);
                receiver_seqCache[stop_addr] = tdata_head->getIdentification();
                EV<<"ordering packets again, the now seq number = "<<rcv_it->second<<", the order map size = "<< receiver_orderMap.size() <<endl;
                sendUp(temp_pck);
            }
            else
            {
                temp_pck->insertAtFront(tdata_head);
            }
        }
        receiver_orderMap.erase(stop_addr);
    }
    */

    EV << "00000000 receiver_flows = "<<receiver_flowMap.size()<<" ! 00000000"<<endl;
    if (receiver_flowMap.size() == 0)
    {
        //currate = maxrate;
        CreState = STOP_CREDIT_STATE;
    }
    else
    {
        // re-schedule the credit_send, increase speed and decrease schedule time.
        //currate = maxrate/receiver_flowMap.size();
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

void MccmCore::schedule_next_credit(simtime_t delta_t)
{
    // for multi-path transmitting

    receiver_mpinfo mp_info = receiver_mpMap.find(next_creditaddr)->second;
    simtime_t min_t= 100000;
    int mpindex = -1;
    int mpsum = mp_info.virtualpaths;
    double time_jitter = 0;
    double sum_speed = 0;

    //int mpsum = mp_info.virtualpaths;
    EV<<"0000 the multi-paths size = "<<mpsum<<", for address"<<next_creditaddr.toIpv4()<<endl;
    for (int i=0;i<mpsum;i++)
    {
        mp_info.next_time[i] = mp_info.next_time[i] - delta_t;
        if (mp_info.next_time[i] == 0)
        {
            mp_info.next_time[i] = credit_size/mp_info.speed[i] + time_jitter; // will minus delta_t in following
            mpindex = i;
        }
        else
        {
            mp_info.next_time[i] = mp_info.next_time[i] + time_jitter; // will minus delta_t in following
        }
        if (mp_info.next_time[i] < min_t)
        {
            min_t = mp_info.next_time[i];
        }
        mp_info.next_time[i] = mp_info.next_time[i] + delta_t;
        sum_speed  = mp_info.speed[i] + sum_speed;
    }

    EV<<"schedule_nextcdt(), the current sum_speed of the flow = "<<sum_speed<<endl;
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

    double avg_next_time = 0;
    auto d_it = delayMap.begin();
    for(; d_it != delayMap.end(); ++ d_it)
    {
        delayMap[d_it->first] = d_it->second - delta_t;
        receiver_mpinfo tmpinfo = receiver_mpMap.find(d_it->first)->second;
        for (int i=0; i<tmpinfo.virtualpaths; i++)
        {
            tmpinfo.next_time[i] = tmpinfo.next_time[i] - delta_t;
            receiver_mpMap[d_it->first] = tmpinfo;
            avg_next_time = credit_size/tmpinfo.next_time[i] + avg_next_time;
        }
    }

    //avg_next_time = avg_next_time/maxrate;
    EV<<"schedule_nextcdt(), the current next size packet of the flow ="<<avg_next_time<<endl;

    find_nextaddr();

    EV <<" 00000000 The next delay = "<<next_cdt_delay<<", the next scheduled packet "<<next_creditaddr<<endl;

    delta_cdt_time = next_cdt_delay;

    scheduleAt(simTime()+next_cdt_delay,sendcredit);

}

void MccmCore::find_nextaddr()
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

simtime_t MccmCore::get_singleRTT(Packet *pck)
{
    return simTime() - pck->getTimestamp();
}

// speed: new sum speed for current paths.
// oldspeed: old sum speed for current paths.
// max_sumspeed: max sum speed for all paths.
void MccmCore::multipath_control(L3Address addr, double speed, double oldspeed, double max_sumspeed)
{
    if (rate_control)
    {
        EV<<"multipath_control(), max_sumspeed = "<< max_sumspeed<<", oldspeed = "<<oldspeed<<", speed = "<<speed<<endl;
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

    srand((unsigned)(SIMTIME_DBL(simTime())*10000000000));
    mpinfo.virtualpaths = vpaths + 1;
    mpinfo.path_id[vpaths] = rand()%252;
    mpinfo.speed[vpaths] = max_sumspeed - speed;
    mpinfo.next_time[vpaths] = credit_size/(max_sumspeed-speed);
    mint = credit_size/(max_sumspeed-speed);
    minid = vpaths;
    newpathid = vpaths;
    onew_pathid = newpathid;
    vpaths++;
    osum_paths = vpaths;
    // update the receiver_mpMap and delayMap.
    // we adjust the next_time of scheduled address directly at here.
    receiver_mpinfo t_mpinfo;
    int t_i = 0;
    double sum_speed = 0;
    for(int i=0;i<osum_paths;i++)
    {
        if (mpinfo.speed[i]<max_sumspeed/(10*sumpaths))
        {
            if (i!=onew_pathid)
            {
                t_mpinfo.speed[onew_pathid] = mpinfo.speed[onew_pathid] + mpinfo.speed[i];
                t_mpinfo.next_time[onew_pathid] = credit_size/(mpinfo.speed[onew_pathid]);
                mint = mpinfo.next_time[onew_pathid];
                oldspeed = oldspeed - mpinfo.speed[i];
                vpaths = vpaths - 1;
                if (onew_pathid>i)
                {
                    newpathid = newpathid - 1; // the new id of added vp has been minus by 1;
                }
                EV <<"multipath_control(), delete the low speed virtual path: "<<addr<<"__"<<i<<", speed = "<<mpinfo.speed[i]<<", sum of vps = "<<vpaths<<endl;
            }
            else
            {
                //oldspeed = max_sumspeed;
                oldspeed = oldspeed - mpinfo.speed[i];
                vpaths = vpaths - 1;
                EV <<"multipath_control(), delete the new virtual path: "<<addr<<"__"<<i<<", speed = "<<mpinfo.speed[i]<<", sum of vps = "<<vpaths<<endl;
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
            EV <<"multipath_control(), after adjusting, the next time of "<<addr<<"__"<<i<<" = "<<t_mpinfo.next_time[i]<<", speed = "<<t_mpinfo.speed[i]<<", multies "<<speed/oldspeed<<endl;
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
            mpinfo.path_id[vpaths] = rand()%252;
            mpinfo.speed[vpaths] = max_sumspeed - speed;
            mpinfo.next_time[vpaths] = credit_size/(max_sumspeed-speed);
            mint = credit_size/(max_sumspeed-speed);
            minid = vpaths;
            newpathid = vpaths;
            onew_pathid = newpathid;
            vpaths++;
            osum_paths = vpaths;
        }
        // update the receiver_mpMap and delayMap.
        // we adjust the next_time of scheduled address directly at here.
        receiver_mpinfo t_mpinfo;
        int t_i = 0;
        double sum_speed = 0;
        for(int i=0;i<osum_paths;i++)
        {
            if (mpinfo.speed[i]<max_sumspeed/(10*sumpaths))
            {
                if (i!=onew_pathid)
                {
                    t_mpinfo.speed[onew_pathid] = mpinfo.speed[onew_pathid] + mpinfo.speed[i];
                    t_mpinfo.next_time[onew_pathid] = credit_size/(mpinfo.speed[onew_pathid]);
                    mint = mpinfo.next_time[onew_pathid];
                    oldspeed = oldspeed - mpinfo.speed[i];
                    vpaths = vpaths - 1;
                    if (onew_pathid>i)
                    {
                        newpathid = newpathid - 1; // the new id of added vp has been minus by 1;
                    }
                    EV <<"multipath_control(), delete the low speed virtual path: "<<addr<<"__"<<i<<", speed = "<<mpinfo.speed[i]<<", sum of vps = "<<vpaths<<endl;
                }
                else
                {
                    //oldspeed = max_sumspeed;
                    oldspeed = oldspeed - mpinfo.speed[i];
                    vpaths = vpaths - 1;
                    newpathid = 256;
                    EV <<"multipath_control(), delete new added low speed virtual path: "<<addr<<"__"<<i<<", speed = "<<mpinfo.speed[i]<<", sum of vps = "<<vpaths<<endl;
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
    else
    { }
}

void MccmCore::finish()
{
    recordScalar("max ooo degree",max_ooo);
}

} // namespace inet


