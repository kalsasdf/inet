//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpTestApp.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

namespace inet {

Define_Module(UdpTestApp);

long UdpTestApp::overallflowid;

UdpTestApp::~UdpTestApp()
{
    cancelAndDelete(selfMsg);
}

void UdpTestApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

        FlowSizes_Vector.setName("flows size/B");

        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        localPort = par("localPort");
        destPort = par("destPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        trafficMode = par("trafficMode");
        multiple_of_linkspeed = par("multiplelinkSpeed");
        randomseed = par("randomSeed");
        flowsize = par("flowSize");
        load = par("workLoad");
        linkspeed = par("linkSpeed");
        test = par("testMode");
        max_flowamount = par("flowAmount");
        cur_flowamount = 0;
        overallflowid = 0;
        flowid = overallflowid;


        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("sendTimer");
    }
}

void UdpTestApp::finish()
{
    //recordScalar("packets sent", numSent);
    //recordScalar("packets received", numReceived);
    ApplicationBase::finish();
}

void UdpTestApp::setSocketOptions()
{
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket.setTimeToLive(timeToLive);

    int typeOfService = par("typeOfService");
    if (typeOfService != -1)
        socket.setTypeOfService(typeOfService);

    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        InterfaceEntry *ie = ift->getInterfaceByName(multicastInterface);
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
    if (joinLocalMulticastGroups) {
        MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
    }
    socket.setCallback(this);
}

L3Address UdpTestApp::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void UdpTestApp::sendPacket(int length)
{
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    const auto& payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(length));
    payload->setSequenceNumber(seq_cache.find(flowDest)->second);
    payload->setFlowid(flowid);
    auto creationTimeTag = payload->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(flow_creation_time.find(flowid)->second);
    packet->insertAtBack(payload);
    packet->setSrcProcId(flowid);
    emit(packetSentSignal, packet);
    socket.sendTo(packet, flowDest, destPort);
    EV<<"Send packet with the SN = "<<numSent<<", the dest address = "<<flowDest<<endl;
    numSent++;
    seq_cache[flowDest] = seq_cache.find(flowDest)->second + 1;
}

void UdpTestApp::processStart()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    //socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    socket.bind(L3Address(), localPort);
    setSocketOptions();
    EV<<"src set addr = "<<L3AddressResolver().resolve(localAddress).toIpv4().getInt()<<endl;
    EV<<"src ip addr = "<<L3Address().toIpv4().getInt()<<endl;

    if(test)
    {
        if(L3AddressResolver().resolve(localAddress).toIpv4().getInt()<=L3AddressResolver().resolve("s120").toIpv4().getInt())
        {
            const char *destAddrs2 = par("destAddressesII");
            cStringTokenizer tokenizer2(destAddrs2);
            const char *token;
            while ((token = tokenizer2.nextToken()) != nullptr) {
                destAddressStr.push_back(token);
                L3Address result;
                L3AddressResolver().tryResolve(token, result);
                if (result.isUnspecified())
                    EV_ERROR << "cannot resolve destination address: " << token << endl;
                destAddresses.push_back(result);
                seq_cache[result] = 0;
            }
        }
        else
        {
            const char *destAddrs1 = par("destAddressesI");
            cStringTokenizer tokenizer1(destAddrs1);
            const char *token;

            while ((token = tokenizer1.nextToken()) != nullptr) {
                destAddressStr.push_back(token);
                L3Address result;
                L3AddressResolver().tryResolve(token, result);
                if (result.isUnspecified())
                    EV_ERROR << "cannot resolve destination address: " << token << endl;
                destAddresses.push_back(result);
                seq_cache[result] = 0;
            }
        }
    }
    else
    {
        const char *destAddrs1 = par("destAddressesI");
        cStringTokenizer tokenizer1(destAddrs1);
        const char *token;

        while ((token = tokenizer1.nextToken()) != nullptr) {
            destAddressStr.push_back(token);
            L3Address result;
            L3AddressResolver().tryResolve(token, result);
            if (result.isUnspecified())
                EV_ERROR << "cannot resolve destination address: " << token << endl;
            destAddresses.push_back(result);
            seq_cache[result] = 0;
        }
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        thisburst_start_time = simTime();
        overallflowid++;
        flowid = overallflowid;
        flow_creation_time[flowid] = thisburst_start_time;
        flowDest = chooseDestAddr();
        updateNextFlow(trafficMode);
        EV<<"burstTime = "<<burstTime<<", sleepTime = "<<sleepTime<<endl;
        processSend();
    }
    else {
        if (stopTime >= SIMTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleAt(stopTime, selfMsg);
        }
    }
}

void UdpTestApp::processSend()
{
    simtime_t dt = simTime() + sendInterval;

    //if (dt >= stopTime)
    //{
    //    selfMsg->setKind(STOP);
    //    scheduleAt(stopTime, selfMsg);
    //}
    if (cur_flowamount < max_flowamount)
    {
        if (dt <= thisburst_start_time+burstTime) {
            sendPacket(packetLength);
            selfMsg->setKind(SEND);
            scheduleAt(dt, selfMsg);
        }
        else if (simTime() < thisburst_start_time+burstTime)
        {
            lastpckLength = packetLength*(thisburst_start_time + burstTime - simTime())/sendInterval;
            EV<<"The last packet of this flow with the size = "<<lastpckLength<<endl;
            selfMsg->setKind(SENDLASTPCK);
            scheduleAt(thisburst_start_time+burstTime, selfMsg);
        }
        else if (thisburst_start_time+burstTime+sleepTime<stopTime)
        {
            selfMsg->setKind(NEWFLOW);
            thisburst_start_time = thisburst_start_time + burstTime + sleepTime;
            updateNextFlow(trafficMode);
            EV<<"burstTime = "<<burstTime<<", sleepTime = "<<sleepTime<<endl;
            scheduleAt(thisburst_start_time, selfMsg);
        }
        else
        {
            selfMsg->setKind(STOP);
            scheduleAt(simTime(), selfMsg);
        }
    }
    else
    {
        selfMsg->setKind(STOP);
        scheduleAt(simTime(), selfMsg);
    }
}

void UdpTestApp::processSendLastPck()
{
    sendPacket(lastpckLength);
    simtime_t nextburst_start_time = thisburst_start_time + burstTime + sleepTime;

    if (nextburst_start_time<stopTime)
    {
        selfMsg->setKind(NEWFLOW);
        thisburst_start_time = nextburst_start_time;
        updateNextFlow(trafficMode);
        EV<<"Entering next burst, burstTime = "<<burstTime<<", sleepTime = "<<sleepTime<<endl;
        scheduleAt(thisburst_start_time, selfMsg);
    }
    else
    {
        selfMsg->setKind(STOP);
        scheduleAt(simTime(), selfMsg);
    }
}

void UdpTestApp::processStop()
{
    socket.close();
}

void UdpTestApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case SEND:
                processSend();
                break;

            case STOP:
                processStop();
                break;

            case SENDLASTPCK:
                processSendLastPck();
                break;

            case NEWFLOW:
                overallflowid++;
                flowid = overallflowid;
                flow_creation_time[flowid] = thisburst_start_time;
                flowDest = chooseDestAddr();
                processSend();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else
        socket.processMessage(msg);
}

void UdpTestApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void UdpTestApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void UdpTestApp::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void UdpTestApp::processPacket(Packet *pk)
{
    emit(packetReceivedSignal, pk);
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    delete pk;
    numReceived++;
}

bool UdpTestApp::handleNodeStart(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
    return true;
}

bool UdpTestApp::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

void UdpTestApp::handleNodeCrash()
{
    if (selfMsg)
        cancelEvent(selfMsg);
}

void UdpTestApp::updateNextFlow(const char* TM)
{
    packetLength = par("messageLength");
    //srand((unsigned)simTime().raw());
    double seed = uniform(0,1);
    EV<<"seed = "<<seed<<endl;
    int int_flowsize;
    cur_flowamount++;
    if (std::string(TM).find("CacheFollower") != std::string::npos)
    {// 50% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 29% 1mb~, average 701kb
        if (seed<=0.01)
        {
            int_flowsize = 70;
            packetLength = int_flowsize;
        }
        else if (seed<=0.015)
        {
            int_flowsize = rand()%(150-71) + 71;
            packetLength = int_flowsize;
        }
        else if (seed<=0.04)
        {
            int_flowsize = 150;
            packetLength = int_flowsize;
        }
        else if (seed<=0.08)
        {
            int_flowsize = rand()%(300-151) + 151;
            packetLength = int_flowsize;
        }
        else if (seed<=0.1)
        {
            int_flowsize = rand()%(350-301) + 301;
            packetLength = int_flowsize;
        }
        else if (seed<=0.19)
        {
            int_flowsize = 350;
            packetLength = int_flowsize;
        }
        else if (seed<=0.2)
        {
            int_flowsize = rand()%(450-351) + 351;
            packetLength = int_flowsize;
        }
        else if (seed<=0.28)
        {
            int_flowsize = rand()%(500-451) + 451;
            packetLength = int_flowsize;
        }
        else if (seed<=0.3)
        {
            int_flowsize = rand()%(600-501) + 501;
            packetLength = int_flowsize;
        }
        else if (seed<=0.35)
        {
            int_flowsize = rand()%(700-601) + 601;
            packetLength = int_flowsize;
        }
        else if (seed<=0.4)
        {
            int_flowsize = rand()%(1100-701) + 701;
            packetLength = int_flowsize;
        }
        else if (seed<=0.42)
        {
            int_flowsize = rand()%(2000-1101) + 1101;
            if (int_flowsize<=1468)
            {
                packetLength = int_flowsize;
            }
        }
        else if (seed<=0.48)
        {
            int_flowsize = rand()%(10000-2001) + 2001;
        }
        else if (seed<=0.5)
        {
            int_flowsize = rand()%(30000-10001) + 10001;
        }
        else if (seed<=0.52)
        {
            int_flowsize = rand()%(100000-30001) + 30001;
        }
        else if (seed<=0.6)
        {
            int_flowsize = rand()%(200000-100001) + 100001;
        }
        else if (seed<=0.68)
        {
            int_flowsize = rand()%(400000-200001) + 200001;
        }
        else if (seed<=0.7)
        {
            int_flowsize = rand()%(600000-400001) + 400001;
        }
        else if (seed<=0.701)
        {
            int_flowsize = rand()%(15000-6001)*100 + 600001;
        }
        else if (seed<=0.8)
        {
            int_flowsize = rand()%(20000-15001)*100 + 1500001;
        }
        else if (seed<=0.9)
        {
            int_flowsize = rand()%(24000-20001)*100 + 2000001;
        }
        else if (seed<=1)
        {
            int_flowsize = rand()%(30000-24001)*100 + 2400001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("DataMining") != std::string::npos)
    {// 78% 0~10kb, 5% 10~100kb, 8% 100kb~1mb, 9% 1mb~, average 7410kb
        if (seed<=0.8)
        {
            int_flowsize = rand()%(10000-101) + 101;
            if (int_flowsize<=1468)
            {
                packetLength = int_flowsize;
            }
        }
        else if (seed<=0.8346)
        {
            int_flowsize = rand()%(15252-1001)*10 + 10001;
        }
        else if (seed<=0.9)
        {
            int_flowsize = rand()%(39054-15253)*10 + 152523;
        }
        else if (seed<=0.953846)
        {
            int_flowsize = rand()%(32235-3906)*100 + 390542;
        }
        else if (seed<=0.99)
        {
            int_flowsize = rand()%(10000-323)*10000 + 3223543;
        }
        else if (seed<=1)
        {
            int_flowsize = rand()%(10000-1001)*100000 + 100000001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("WebServer") != std::string::npos)
    {// 63% 0~10kb, 18% 10~100kb, 19% 100kb~1mb, 0% 1mb~, average 64kb
        if (seed<=0.12)
        {
            int_flowsize = rand()%(300-151) + 151;
            packetLength = int_flowsize;
        }
        else if (seed<=0.2)
        {
            int_flowsize = 300;
            packetLength = int_flowsize;
        }
        else if (seed<=0.3)
        {
            int_flowsize = rand()%(1000-601) + 601;
            packetLength = int_flowsize;
        }
        else if (seed<=0.4)
        {
            int_flowsize = rand()%(2000-1001) + 1001;
            if (int_flowsize<=1468)
            {
                packetLength = int_flowsize;
            }
        }
        else if (seed<=0.5)
        {
            int_flowsize = rand()%(3100-2001) + 2001;
        }
        else if (seed<=0.6)
        {
            int_flowsize = rand()%(6000-3101) + 3101;
        }
        else if (seed<=0.71)
        {
            int_flowsize = rand()%(20000-6001) + 6001;
        }
        else if (seed<=0.8)
        {
            int_flowsize = rand()%(6000-2001)*10 + 20001;
        }
        else if (seed<=0.82)
        {
            int_flowsize = rand()%(15000-6001)*10 + 60001;
        }
        else if (seed<=0.9)
        {
            int_flowsize = rand()%(30000-15001)*10 + 150001;
        }
        else if (seed<=1)
        {
            int_flowsize = rand()%(50000-30001)*10 + 300001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if ((std::string(TM).find("WebSearch") != std::string::npos))
    {// 49% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 20% 1mb~ (big14000kb), average 1600kb
        if (seed<=0.15)
        {
            int_flowsize = 9000;
        }
        else if (seed<=0.2)
        {
            int_flowsize = rand()%(18582-9001) + 9001;
        }
        else if (seed<=0.3)
        {
            int_flowsize = rand()%(28140-18583) + 18583;
        }
        else if (seed<=0.4)
        {
            int_flowsize = rand()%(38913-28141) + 28141;
        }
        else if (seed<=0.53)
        {
            int_flowsize = rand()%(7747-3892)*10 + 38914;
        }
        else if (seed<=0.6)
        {
            int_flowsize = rand()%(20000-7747)*10 + 77469;
        }
        else if (seed<=0.7)
        {
            int_flowsize = rand()%(10000-2001)*100 + 200001;
        }
        else if (seed<=0.8)
        {
            int_flowsize = rand()%(20000-10001)*100 + 1000001;
        }
        else if (seed<=0.9)
        {
            int_flowsize = rand()%(50000-20001)*100 + 2000001;
        }
        else if (seed<=0.97)
        {
            int_flowsize = rand()%(10000-5001)*1000 + 5000001;
        }
        else if (seed<=1)
        {
            int_flowsize = rand()%(30000-10001)*1000 + 10000001;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCep") != std::string::npos)
    {
        if (seed<=0.4436)
        {
            int_flowsize = 48;
            packetLength = int_flowsize;
        }
        else if (seed<=0.7265)
        {
            int_flowsize = 56;
            packetLength = int_flowsize;
        }
        else if (seed<=1)
        {
            int_flowsize = 128;
            packetLength = int_flowsize;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCcg") != std::string::npos)
    {
        if (seed<=0.6316)
        {
            int_flowsize = 48;
            packetLength = int_flowsize;
        }
        else if (seed<=0.6345)
        {
            int_flowsize = 128;
            packetLength = int_flowsize;
        }
        else if (seed<=0.8172)
        {
            int_flowsize = 3544;
        }
        else if (seed<=1)
        {
            int_flowsize = 3552;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCft") != std::string::npos)
    {
        if (seed<=0.0226)
        {
            int_flowsize = 48;
            packetLength = int_flowsize;
        }
        else if (seed<=0.0234)
        {
            int_flowsize = 56;
            packetLength = int_flowsize;
        }
        else if (seed<=0.0242)
        {
            int_flowsize = 64;
            packetLength = int_flowsize;
        }
        else if (seed<=0.025)
        {
            int_flowsize = 80;
            packetLength = int_flowsize;
        }
        else if (seed<=0.0258)
        {
            int_flowsize = 112;
            packetLength = int_flowsize;
        }
        else if (seed<=0.9115)
        {
            int_flowsize = 128;
            packetLength = int_flowsize;
        }
        else if (seed<=0.9123)
        {
            int_flowsize = 176;
            packetLength = int_flowsize;
        }
        else if (seed<=0.9131)
        {
            int_flowsize = 304;
            packetLength = int_flowsize;
        }
        else if (seed<=0.921)
        {
            int_flowsize = 308;
            packetLength = int_flowsize;
        }
        else if (seed<=0.9218)
        {
            int_flowsize = 560;
            packetLength = int_flowsize;
        }
        else if (seed<=0.9889)
        {
            int_flowsize = 1072;
            packetLength = int_flowsize;
        }
        else if (seed<=0.9697)
        {
            int_flowsize = 2096;
        }
        else if (seed<=0.9905)
        {
            int_flowsize = 4144;
        }
        else if (seed<=1)
        {
            int_flowsize = 16432;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else if (std::string(TM).find("HPCmg") != std::string::npos)
    {
        if (seed<=0.0978)
        {
            int_flowsize = 48;
            packetLength = int_flowsize;
        }
        else if (seed<=0.4065)
        {
            int_flowsize = 56;
            packetLength = int_flowsize;
        }
        else if (seed<=0.6638)
        {
            int_flowsize = 64;
        }
        else if (seed<=0.6747)
        {
            int_flowsize = 72;
        }
        else if (seed<=0.6954)
        {
            int_flowsize = 80;
        }
        else if (seed<=0.6980)
        {
            int_flowsize = 96;
        }
        else if (seed<=0.7180)
        {
            int_flowsize = 112;
        }
        else if (seed<=0.7316)
        {
            int_flowsize = 120;
        }
        else if (seed<=0.7445)
        {
            int_flowsize = 128;
        }
        else if (seed<=0.7664)
        {
            int_flowsize = 144;
        }
        else if (seed<=0.7873)
        {
            int_flowsize = 176;
        }
        else if (seed<=0.7900)
        {
            int_flowsize = 192;
        }
        else if (seed<=0.8109)
        {
            int_flowsize = 304;
        }
        else if (seed<=0.8302)
        {
            int_flowsize = 336;
        }
        else if (seed<=0.8495)
        {
            int_flowsize = 368;
        }
        else if (seed<=0.8688)
        {
            int_flowsize = 848;
        }
        else if (seed<=0.8881)
        {
            int_flowsize = 1072;
        }
        else if (seed<=0.9074)
        {
            int_flowsize = 1200;
        }
        else if (seed<=0.9267)
        {
            int_flowsize = 2640;
        }
        else if (seed<=0.9511)
        {
            int_flowsize = 4144;
        }
        else if (seed<=0.9756)
        {
            int_flowsize = 4400;
        }
        else if (seed<=1)
        {
            int_flowsize = 9296;
        }
        else
        {
            throw cRuntimeError("Wrong flow information");
        }
    }
    else
    {
        throw cRuntimeError("Unrecognized traffic mode!");
    }

    flowsize = double(int_flowsize);
    EV<<"Generate a new flow, size = "<<flowsize<<endl;
    FlowSizes_Vector.recordWithTimestamp(simTime(),flowsize);
    sendInterval = double(packetLength)*8/double(linkspeed);
    sendInterval = sendInterval/1000000;
    sendInterval = sendInterval/multiple_of_linkspeed;
    burstTime = sendInterval * (flowsize/packetLength);
    sleepTime = (burstTime*multiple_of_linkspeed)/load - burstTime;

}

} // namespace inet

