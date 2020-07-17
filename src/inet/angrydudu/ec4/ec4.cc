// Copyright (C) 2018 Angrydudu

#include "../ec4/ec4.h"

#include "inet/common/INETDefs.h"


namespace inet {

Define_Module(ec4);

simsignal_t ec4::queueLengthSignal = registerSignal("queueLength");
simsignal_t ec4::ooodegreeSignal = registerSignal("oooDegree");

void ec4::initialize()
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
    frameCapacity = par("frameCapacity");
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
    useTokens = par("useTokens");
    percdtTokens = par("percdtTokens");
    AlphaTimer_th = par("AlphaTimer_th");
    RateTimer_th = par("RateTimer_th");
    frSteps_th = par("frSteps_th");
    ByteCounter_th = par("ByteCounter_th");
    Rai = par("Rai");
    Rhai = par("Rhai");
    gamma = par("gamma");
    linkspeed=linkspeed*1000000000;

    Rai *= 1e6;
    Rhai *= 1e6;
    AlphaTimer_th *= 1e-6;
    RateTimer_th *= 1e-6;
    max_pck_size = 1468;

    sendcredit=new TimerMsg("sendcredit");
    sendcredit->setKind(SENDCRED);
    stopcredit=new TimerMsg("stopcredit");
    stopcredit->setKind(STOPCRED);

    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

    const char *crcModeString = par("crcMode");
    if (!strcmp(crcModeString, "declared"))
        crcMode = CRC_DECLARED_CORRECT;
    else if (!strcmp(crcModeString, "computed"))
        crcMode = CRC_COMPUTED;
    else
        throw cRuntimeError("Unknown crc mode: '%s'", crcModeString);

    //registerService(Protocol::tcp, gate("upperIn"), gate("lowerIn"));
    //registerProtocol(Protocol::tcp, gate("lowerOut"), gate("upperOut"));
    registerService(Protocol::udp, gate("upperIn"), gate("lowerIn"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("upperOut"));

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

    // state

    // credit state
    CreState = INITIAL_STATE;

    credit_size = 592;
    // The parameters of feedback control

    targetratio = 0;
    targetecnratio=0;
    wmax = 0.06;
    wmin = 0.01;
    modethreshold=3;

    if (multipath)
    {
        maxrate = 0.05*linkspeed; // The max link capacity for transmitting credits. 0.45%
        currate = maxrate;
    }
    else
    {
        maxrate = 0.05*linkspeed; // The max link capacity for transmitting credits. 0.45%
        currate = maxrate;
    }
    if (!rate_control)
    {
        currate = maxrate;
    }
    max_lossratio = 0.9;
    // Initialize the credit sequence number of the receiver

    delta_cdt_time = credit_size/currate;//0.000019456s=19.456us=0.019456ms
    last_cdt_time = 0;

    // Used for judging the end of flow.
    this->max_idletime = bits_timeout/linkspeed;
    // this->max_idletime = 0.002;
    treceiver_flowinfo = {simTime(),0,0,1,-1,0,0,0,0,1,gamma,0,0,0,0,0,0,0,0,currate,maxrate,maxrate,false,Normal};

    tsender_tokeninfo = {0,0,0,0,0,simTime()};

    if (activate)
    {
        // statistics
        numMapReceived = 0;
        numMapDropped = 0;
        WATCH(numMapReceived);  // By using WATCH() function, we can see the value in the simulator.
        WATCH(numMapDropped);
    }
}

void ec4::handleMessage(cMessage *msg)
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

void ec4::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    EV_TRACE << "Self-message " << pck << " received\n";

    switch (timer->getKind()) {
        case SENDCRED:
            self_send_credit();
            break;
        case STOPCRED:
            self_stop_credit();
            break;
        case RATETIMER:
        {
            receiver_flowinfo tinfo = receiver_flowMap.find(timer->getDestAddr())->second;
            tinfo.TimeFrSteps++;
            receiver_flowMap[timer->getDestAddr()] = tinfo;
            increaseTxRate(timer->getDestAddr());
            scheduleAt(simTime()+RateTimer_th,rateTimer);
            break;
        }
        case ALPHATIMER:
        {
            updateAlpha(timer->getDestAddr());
            break;
        }
    }
}

void ec4::increaseTxRate(L3Address addr)
{

    receiver_flowinfo tinfo = receiver_flowMap.find(addr)->second;
    double oldspeed = tinfo.newspeed;

    if (max(tinfo.ByteFrSteps,tinfo.TimeFrSteps) < frSteps_th)
    {
        tinfo.SenderState = Fast_Recovery;
    }
    else if (min(tinfo.ByteFrSteps,tinfo.TimeFrSteps) > frSteps_th)
    {
        tinfo.SenderState = Hyper_Increase;
    }
    else
    {
        tinfo.SenderState = Additive_Increase;
    }
    EV<<"entering inceaseTxRate(), sender state = "<<tinfo.SenderState<<endl;

    if (tinfo.SenderState == Fast_Recovery)
    {// Fast Recovery
        tinfo.newspeed = (tinfo.newspeed + tinfo.targetRate) / 2;
    }
    else if (tinfo.SenderState == Additive_Increase)
    {// Additive Increase
        //tinfo.targetRate += Rai;
        //tinfo.targetRate = (tinfo.targetRate > currate) ? currate : tinfo.targetRate;
        tinfo.newspeed = (tinfo.newspeed + currate) / 2;
    }
    else if (tinfo.SenderState == Hyper_Increase)
    {// Hyper Increase
        //tinfo.iRhai++;
        //tinfo.targetRate += tinfo.iRhai*Rhai;
        //tinfo.targetRate = (tinfo.targetRate > currate) ? currate : tinfo.targetRate;
        tinfo.newspeed = (tinfo.newspeed + currate) / 2;
    }
    else
    {// Normal state

    }
    EV<<"after increasing,oldspeed="<<oldspeed<<",newspeed="<<tinfo.newspeed<<endl;
    //EV<<"after increasing, the current rate = "<<tinfo.newspeed<< ", target rate = "<<tinfo.targetRate<<endl;
    receiver_flowMap[addr] = tinfo;

}

void ec4::updateAlpha(L3Address addr)
{

    receiver_flowinfo tinfo = receiver_flowMap.find(addr)->second;
    tinfo.alpha = (1 - gamma) * tinfo.alpha;
    scheduleAt(simTime() + AlphaTimer_th, alphaTimer);
    receiver_flowMap[addr] = tinfo;
}

void ec4::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "q rcvd: %d\nq dropped: %d", numMapReceived, numMapDropped);
    getDisplayString().setTagArg("t", 0, buf);
}

// Process the message from lower layer.
void ec4::processUpperpck(Packet *pck)
{
    sender_flowinfo snd_info;

    auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
    L3Address srcAddr = addressReq->getSrcAddress();
    L3Address destAddr = addressReq->getDestAddress();

    //EV<<"Udp transmits the packet with the dest address of "<<destAddr<<endl;

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
    for (auto& region : pck->peekData()->getAllTags<CreationTimeTag>())
        snd_info.creaTime = region.getTag()->getCreationTime();
    // schedule the nxt tx packet time
    snd_info.nxtSendTime = simTime();
    sender_flowMap[destAddr] = snd_info;
    sender_tokeninfo tokeninfo = tsender_tokeninfo;
    sender_tokens[destAddr] = tokeninfo;
    send_credreq(destAddr);
    delete pck;
}
// Process the Packet from upper layer.
void ec4::processLowerpck(Packet *pck)
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
    else if (string(pck->getFullName()).find("EC4Data") != string::npos || string(pck->getFullName()).find("UdpBasicAppData") != string::npos)
    {
        receive_data(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void ec4::send_credreq(L3Address destaddr)
{
    Packet *cred_req = new Packet("credit_req");
    cred_req->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    cred_req->addTagIfAbsent<L3AddressReq>()->setSrcAddress(sndsrc);
    cred_req->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x1A);
    cred_req->setTimestamp(simTime());
    cred_req->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_req->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

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
    sendDown(cred_req); // send down the credit_request.
}

// Receive the SYN packet from the sender, and begin sending credit.
void ec4::receive_credreq(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    rcvsrc = l3addr->getDestAddress();

    // rate increasing timer
    rateTimer = new TimerMsg("rateTimer");
    rateTimer->setKind(RATETIMER);
    rateTimer->setDestAddr(l3addr->getSrcAddress());

    // alpha updating timer
    alphaTimer = new TimerMsg("alphaTimer");
    alphaTimer->setKind(ALPHATIMER);
    alphaTimer->setDestAddr(l3addr->getSrcAddress());

    receiver_flowinfo rcvflow = treceiver_flowinfo;
    rcvflow.nowRTT = 2*(simTime() - pck->getTimestamp());
    rcvflow.last_Fbtime = simTime();
    rcvflow.pck_in_rtt = 0;


    receiver_mpinfo rcvmp;
    rcvmp.virtualpaths = 1;

    rcvmp.speed[0] = rcvflow.newspeed;
    rcvmp.next_time[0] = 0;
    //EV <<"receive_credreq,current rate = "<< currate<<endl;
    receiver_flowMap[l3addr->getSrcAddress()] = rcvflow;
    receiver_mpMap[l3addr->getSrcAddress()] = rcvmp;


        cancelEvent(sendcredit);
        delayMap[l3addr->getSrcAddress()] = credit_size/currate;//delta_cdt_time 19.456ms
        next_creditaddr = l3addr->getSrcAddress();
        last_cdt_time = simTime();
        CreState = SEND_CREDIT_STATE;
        scheduleAt(simTime(),sendcredit); //schedule selfMessage


}

// Handle the self message: send credit.
void ec4::self_send_credit()
{
    if (CreState != STOP_CREDIT_STATE)
    {
        delta_cdt_time = simTime() - last_cdt_time;
        //EV <<"seld_send_credit(),time gap from last sending credits = "<< delta_cdt_time << endl;
        schedule_next_credit(delta_cdt_time);
    }
}

void ec4::schedule_next_credit(simtime_t delta_t)
{
    // for multi-path transmitting

    receiver_mpinfo mp_info = receiver_mpMap.find(next_creditaddr)->second;
    simtime_t min_t= 100000;
    int mpindex = -1;
    int mpsum = mp_info.virtualpaths;
    double time_jitter = 0;
    double sum_speed = 0;

    //EV<<"0000 the multi-paths size = "<<mpsum<<", for address "<<next_creditaddr.toIpv4()<<endl;
    for (int i=0;i<mpsum;i++)
    {
        mp_info.next_time[i] = mp_info.next_time[i] - delta_t;
        if (mp_info.next_time[i] == 0)
        {
            mp_info.next_time[i] = credit_size/mp_info.speed[i] + time_jitter;
            mpindex = i;
        }
        else
        {
            mp_info.next_time[i] = mp_info.next_time[i] + time_jitter;
        }
        if (mp_info.next_time[i] < min_t)
        {
            min_t = mp_info.next_time[i];//0
        }
        mp_info.next_time[i] = mp_info.next_time[i] + delta_t;
        sum_speed  = mp_info.speed[i] + sum_speed;
    }
    //in this topology:sum_speed=mp_info.speed[0];min_t=mp_info.next_time[0];

    //EV<<"schedule_nextcdt(), the current speed of the credit = "<<sum_speed<<endl;
    // send the current credit before scheduling the next credit.
    auto f_it = receiver_flowMap.find(next_creditaddr);
    receiver_flowinfo f_info = f_it->second;
    if (mpindex != -1) // mpMap has been updated during scheduling. multipath_control() has been called
    {
        send_credit(next_creditaddr, mp_info.path_id[mpindex], f_info.creditseq);
    }

    last_cdt_time = simTime();

    f_info.creditseq = f_info.creditseq + 1;
    receiver_flowMap[next_creditaddr] = f_info;
    receiver_mpMap[next_creditaddr] = mp_info;

    //EV<<"0000 the delay map size = "<<delayMap.size()<<endl;

    delayMap[next_creditaddr] = min_t + delta_t;//next_time[0]+delta_t

    //double avg_next_time = 0;
    auto d_it = delayMap.begin();
    for(; d_it != delayMap.end(); ++ d_it)
    {
        delayMap[d_it->first] = d_it->second - delta_t;//next_time[0]
        receiver_mpinfo tmpinfo = receiver_mpMap.find(d_it->first)->second;
        for (int i=0; i<tmpinfo.virtualpaths; i++)
        {
            tmpinfo.next_time[i] = tmpinfo.next_time[i] - delta_t;
            receiver_mpMap[d_it->first] = tmpinfo;
            //avg_next_time = credit_size/tmpinfo.next_time[i] + avg_next_time;
        }
    }

    //EV<<"schedule_nextcdt(),  next credit gap ="<<delayMap[next_creditaddr]<<",simtime="<<simTime()<<endl;

    //find_nextaddr();

    //EV <<"  The next delay = "<<next_cdt_delay<<", the next scheduled packet "<<next_creditaddr<<endl;

    //delta_cdt_time = next_cdt_delay;

    scheduleAt(simTime()+delayMap[next_creditaddr],sendcredit);

}

void ec4::find_nextaddr()
{
    simtime_t min_t= 100000;
    auto d_it = delayMap.begin();
    for(; d_it != delayMap.end(); ++ d_it)
    {
        //EV <<"0000 the value of the delay" <<delayMap.find(d_it->first)->second<<", "<<d_it->first<<endl;
        if (delayMap.find(d_it->first)->second < min_t)
        {
            min_t = d_it->second;
            next_creditaddr = d_it->first;
        }
        next_cdt_delay = min_t;
    }
}

// Generate and send credit to the destination.
void ec4::send_credit(L3Address destaddr,unsigned int pathid, int seq)
{
    Packet *credit = new Packet("credit");

    const auto& content = makeShared<Ipv4Header>();

    //srand((unsigned)(SIMTIME_DBL(simTime())*10000000000));
    int jitter_bytes = intrand(6)+1;

    //EV<<"send_credit(), the jitter bytes = "<<jitter_bytes<<", credit seq = "<<seq<<endl;
    //jitter_bytes = 4;

    content->setChunkLength(b(208+jitter_bytes*8));
    content->enableImplicitChunkSerialization = true;
    content->setCrcMode(crcMode);
    content->setCrc(0);
    content->setIdentification(seq);

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

    if (randomspreading)
    {
        pathid = intrand(sumpaths)+1;
        //pathid = rand()%sumpaths+1;
    }
    else
    {
        size_t value1 = hash<unsigned int>{}((unsigned int)rcvsrc.toIpv4().getInt());
        size_t value2 = hash<unsigned int>{}((unsigned int)destaddr.toIpv4().getInt());
        pathid = hash<unsigned int>{}((value1+value2));
        //EV<<"value1 = "<<value1<<", value2 = "<<value2<<", hashed id = "<<pathid<<endl;
        pathid = pathid%9;
        pathid = pathid%sumpaths + 1;
    }
    credit->setSchedulingPriority(pathid);

    //credit->setSchedulingPriority(pathid);
    credit->setTimestamp(simTime());
    credit->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    credit->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcvsrc);
    credit->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x2E);
    credit->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    credit->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    //EV << "00000000 send credit, DestAddr = "<<destaddr.toIpv4()<<", dscp value = "<<credit->addTagIfAbsent<DscpReq>()->getDifferentiatedServicesCodePoint()<<", random path = "<<pathid<<endl;
    sendDown(credit); // send down the credit
}

// Receive the credit, and send corresponding TCP segment.
void ec4::receive_credit(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    bool stop = false;
    L3Address stopaddr;

    //EV<<"receive_credit from the credit source of "<<l3addr->getSrcAddress()<<endl;
    // Check whether the flow has been dried and send credit stop Packet
    sender_flowinfo info = sender_flowMap.find(l3addr->getSrcAddress())->second;
    /*if((simTime()-info.cretime)>= max_idletime && findData(l3addr->getSrcAddress()) == nullptr)
    {
        stopaddr = l3addr->getSrcAddress();
        stop = true;
    }*/
    if(stop)
   {/*
        //EV << "0000 lets delete flow: "<<sender_flowMap.find(stopaddr)->first<<", the sender seq = "<<sender_flowMap.find(stopaddr)->second.seq_No<<endl;
        sender_flowinfo sndinfo = sender_flowMap.find(stopaddr)->second;
        if (!sndinfo.stop_sent)
        {
            send_stop(stopaddr);
        }
        sndinfo.stop_sent = true;
        sender_flowMap[stopaddr] = sndinfo;
        credit_Map.erase(stopaddr);
        sender_tokens.erase(stopaddr);
        delete pck;*/
    }
    else // no stop
    {
        if(useTokens)
        {
            sender_tokeninfo tokeninfo = sender_tokens.find(l3addr->getSrcAddress())->second;
            tokeninfo.tokens = tokeninfo.tokens + percdtTokens;//1476
            //EV<<"Receiving credit, the tokens is added to = "<< tokeninfo.tokens <<endl;
            tokeninfo.cdt_in_rtt++;
            const auto& credit_head = pck->peekAtFront<Ipv4Header>();
            if (credit_head->getExplicitCongestionNotification() == 3||pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification() == 3)
            {
                tokeninfo.ecn_in_rtt++;
            }
            if ((simTime()-tokeninfo.lastEcnTime) >= 4*(simTime()-pck->getTimestamp()))
            {
                tokeninfo.EcnPrtt = double(tokeninfo.ecn_in_rtt)/double(tokeninfo.cdt_in_rtt);
                tokeninfo.ecn_in_rtt = tokeninfo.cdt_in_rtt = 0;
            }
            tokeninfo.freshPathID = pck->getSchedulingPriority();
            int64_t delaytimes = 1;

            if(info.remainLength>=0&&tokeninfo.tokens>=0)
            {
                std::ostringstream str;
                str << packetName << "-" <<info.flowid<< "-" <<info.pckseq;
                Packet *packet = new Packet(str.str().c_str());
                const auto& payload = makeShared<ApplicationPacket>();
                payload->setSequenceNumber(info.pckseq);
                payload->setFlowid(info.flowid);
                if(info.remainLength>max_pck_size)//1468
                {
                    if(tokeninfo.tokens>=max_pck_size)
                    {
                        payload->setChunkLength(B(max_pck_size));
                        info.remainLength=info.remainLength-max_pck_size;
                        tokeninfo.tokens=tokeninfo.tokens-max_pck_size;
                    }
                    else{
                        payload->setChunkLength(B(tokeninfo.tokens));
                        info.remainLength=info.remainLength-tokeninfo.tokens;
                        tokeninfo.tokens=0;
                    }
                }
                else{
                    if(tokeninfo.tokens>=info.remainLength)
                    {
                        payload->setChunkLength(B(info.remainLength));
                        info.remainLength=0;
                        tokeninfo.tokens=tokeninfo.tokens-info.remainLength;
                    }
                    else{
                        payload->setChunkLength(B(tokeninfo.tokens));
                        info.remainLength=info.remainLength-tokeninfo.tokens;
                        tokeninfo.tokens=0;
                    }
                }
                packet->insertAtBack(payload);

                auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
                addressReq->setSrcAddress(info.srcAddr);
                addressReq->setDestAddress(info.destAddr);

                // insert udpHeader, set source and destination port
                const Protocol *l3Protocol = &Protocol::ipv4;
                auto udpHeader = makeShared<UdpHeader>();
                udpHeader->setSourcePort(info.srcPort);
                udpHeader->setDestinationPort(info.destPort);
                udpHeader->setCrc(info.crc);
                udpHeader->setCrcMode(info.crcMode);
                udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
                insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
                packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);

                //insert ec4 header
                const auto& data_head = makeShared<Ipv4Header>();
                data_head->setChunkLength(b(32));
                data_head->setIdentification(info.pckseq);
                data_head->setDiffServCodePoint(11); // determine the data packets is scheduled by MPC
                EV<<"The Ecn proportin in last RTT = "<<tokeninfo.EcnPrtt<<", total credits = "<< tokeninfo.cdt_in_rtt <<endl;
                if (uniform(0,1) < tokeninfo.EcnPrtt)
                {
                    data_head->setExplicitCongestionNotification(3);
                }
                else
                {
                    data_head->setExplicitCongestionNotification(0);
                }
                packet->insertAtFront(data_head);
                packet->setTimestamp(simTime());
                packet->setSchedulingPriority(tokeninfo.freshPathID);
                sendDelayed(packet, SimTime(delaytimes,SIMTIME_PS),outGate);
                EV_INFO<<"send "<<packet->getByteLength()<<" bytes,"<<"the remainlength is "<<info.remainLength<<endl;
                delaytimes++;
            }
            sender_tokens[l3addr->getSrcAddress()] = tokeninfo;
            delete pck;
        }
        else{
            std::ostringstream str;
            str << packetName << "-" <<info.flowid<< "-" <<info.pckseq;
            Packet *packet = new Packet(str.str().c_str());
            const auto& payload = makeShared<ApplicationPacket>();
            payload->setSequenceNumber(info.pckseq);
            payload->setFlowid(info.flowid);
            payload->setChunkLength(B(max_pck_size));
            packet->insertAtBack(payload);

            auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(info.srcAddr);
            addressReq->setDestAddress(info.destAddr);

            // insert udpHeader, set source and destination port
            const Protocol *l3Protocol = &Protocol::ipv4;
            auto udpHeader = makeShared<UdpHeader>();
            udpHeader->setSourcePort(info.srcPort);
            udpHeader->setDestinationPort(info.destPort);
            udpHeader->setCrc(info.crc);
            udpHeader->setCrcMode(info.crcMode);
            udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
            insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
            packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);

            //insert ec4 header
            const auto& data_head = makeShared<Ipv4Header>();
            data_head->setChunkLength(b(32));
            data_head->setIdentification(info.pckseq);
            data_head->setDiffServCodePoint(11); // determine the data packets is scheduled by MPC
            packet->insertAtFront(data_head);
            packet->setTimestamp(simTime());
            info.pckseq+=1;
            sender_flowMap[l3addr->getSrcAddress()]=info;
            sendDown(packet);
            delete pck;
            EV_INFO<<"send "<<packet->getByteLength()<<" bytes,"<<"the remainlength is "<<info.remainLength<<endl;
        }
    }
}

// Receive the TCP segment, determine the credit loss ratio and enter the feedback control.
void ec4::receive_data(Packet *pck)
{
    // If receive a TCP segment, determine which flow the segment belongs to.
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    //EV_INFO <<"00000000 The src address of the segment = "<< l3addr->getSrcAddress().toIpv4()<<" ! 00000000"<<endl;

    // updating speedboard

    // If the segment is not recognizable, directly pass it.
    auto it = receiver_flowMap.find(l3addr->getSrcAddress());

    auto ecn=pck->getPacketECN();
    //EV<<"receive_data(),ecn="<<ecn<<endl;

    const auto& data_head = pck->removeAtFront<Ipv4Header>();

    if (data_head->getDiffServCodePoint() == 11)// the packet is controlled by angry credits
    {
    if (it!=receiver_flowMap.end())
    {
            receiver_flowinfo tinfo = it->second;

            if(tinfo.SenderState!=Normal)
            {
                tinfo.ByteCounter+=pck->getByteLength();
                if(tinfo.ByteCounter>=ByteCounter_th)
                {
                    tinfo.ByteFrSteps++;
                    tinfo.ByteCounter-=ByteCounter_th;
                    //EV<<"byte counter expired, byte fr steps = "<<tinfo.ByteFrSteps<<endl;
                    increaseTxRate(l3addr->getSrcAddress());
                }
                else{
                    receiver_flowMap[l3addr->getSrcAddress()]=tinfo;
                }
            }
            tinfo.nowseq = data_head->getIdentification();
            //EV<<"receive_data(), now the one-side RTT = "<<tinfo.nowRTT<<", the time gap from last feedback control = "<< simTime()-tinfo.last_Fbtime <<endl;
            // Calculate the credit loss ratio, and enter the feedback control.
            if ((simTime()-tinfo.last_Fbtime) < 1*tinfo.nowRTT)//
            {
                tinfo.pck_in_rtt++;
                //modified by dinghuang
                if (ecn == 3)
                {
                    tinfo.ecn_in_rtt++;
                    //EV<<"receivece_data(),receive 1 ecn."<<endl;
                }
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
            }
            else
            {
                tinfo.pck_in_rtt++;
                //modified by dinghuang
                if (ecn == 3)
                {
                    tinfo.ecn_in_rtt++;
                    //EV<<"receivece_data(),receive 1 ecn."<<endl;
                }
                tinfo.sumlost = tinfo.nowseq - tinfo.lastseq - tinfo.pck_in_rtt;
                double lossratio = double(tinfo.sumlost)/double(tinfo.nowseq - tinfo.lastseq);
                //EV<<"receive_data(), sumlost = "<<tinfo.sumlost<<", nowseq = "<<tinfo.nowseq<<", lastseq = "<<tinfo.lastseq<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
                if(tinfo.sumlost >= 0) // else: error may occur
                {
                    if (!multipath)
                    {
                        tinfo.targetRate=tinfo.newspeed;
                        tinfo = feedback_control(tinfo,false);
                        receiver_mpinfo mpinfo = receiver_mpMap.find(it->first)->second;
                        for (int i=0;i<mpinfo.virtualpaths;i++)
                        {
                            //mpinfo.speed[i] = mpinfo.speed[i]*tinfo.newspeed/oldspeed;
                            mpinfo.speed[i]=tinfo.newspeed;
                        }
                        receiver_mpMap[it->first] = mpinfo;
                        //EV <<"receive_data() no multipath, new speed in mpMap = "<< receiver_mpMap.find(it->first)->second.speed[0] <<endl;
                        receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
                    }
                }
                else
                {
                    //EV<<"receive_data(), sumlost < 0, some packets are in flight."<< endl;
                }
                tinfo.nowRTT = times_of_RTT*(simTime() - pck->getTimestamp());
                tinfo.last_Fbtime = simTime();
                tinfo.sumlost = 0;
                tinfo.lastseq = tinfo.nowseq;
                tinfo.pck_in_rtt = 0;
                tinfo.ecn_in_rtt=0;
                receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
            }
            sendUp(pck);

    }
    }
    else // packet dont controlled by angry credits.
    {
        pck->insertAtFront(data_head);
        sendUp(pck);
    }

    ////
}

// Generate and send stop credit Packet to the destination.
void ec4::send_stop(L3Address addr)
{
    Packet *cred_stop = new Packet("credit_stop");
    cred_stop->addTagIfAbsent<L3AddressReq>()->setDestAddress(addr);
    cred_stop->addTagIfAbsent<L3AddressReq>()->setSrcAddress(sndsrc);
    cred_stop->addTagIfAbsent<DscpReq>()->setDifferentiatedServicesCodePoint(0x1A);
    cred_stop->setTimestamp(simTime());
    cred_stop->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_stop->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

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
    //EV << "00000000 send credit_stop, DestAddr = "<<addr.toIpv4()<<" !00000000"<<endl;
    sendDown(cred_stop); // send down the credit_stop.
}

// Receive the stop credit Packet from sender, and stop sending credit.
void ec4::receive_stopcred(Packet *pck)
{
    EV<<"0000 entering receivestop_cdt"<<endl;
    stop_addr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    scheduleAt(simTime(),stopcredit); //schedule selfmessage
}

void ec4::sendDown(Packet *pck)
{
    //EV << "sendDown(), data map size = " << data_Map.size() << ", path ID = "<<pck->getSchedulingPriority()<<endl;
    send(pck, outGate);
}

void ec4::sendUp(Packet *pck)
{
    //EV<<"ec4, oh sendup!"<<endl;
    send(pck,upGate);

}

// Handle the self message stop credit.
void ec4::self_stop_credit()
{
    EV<<"self_stop_credit(), flows before deleted = "<<receiver_flowMap.size()<<endl;

    receiver_flowMap.erase(stop_addr);
    receiver_mpMap.erase(stop_addr);
    delayMap.erase(stop_addr);

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

void ec4::finish()
{
    recordScalar("max ooo degree",max_ooo);
}

} // namespace inet


