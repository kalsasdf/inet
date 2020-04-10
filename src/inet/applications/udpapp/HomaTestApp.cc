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
#include "inet/applications/udpapp/HomaTestApp.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

namespace inet {

Define_Module(HomaTestApp);

simsignal_t HomaTestApp::FlowCompletionTime = registerSignal("flowcompletiontime");
simsignal_t HomaTestApp::receivedmsgID = registerSignal("receivedmsgID");
unsigned long HomaTestApp::counter;
unsigned long HomaTestApp::receivedMsgs;

HomaTestApp::~HomaTestApp()
{
    cancelAndDelete(selfMsg);
}

void HomaTestApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        FCT_Vector.setName("FCT");
        counter = 0;
        receivedMsgs = 0;
        numSent = 0;
        numReceived = 0;
        cur_flowamount = 0;
        WATCH(numSent);
        WATCH(numReceived);

        workLoad = par("workLoad");
        trafficMode = par("trafficMode");
        linkSpeed = par("linkSpeed");
        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        max_flowamount = par("flowAmount");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("sendTimer");
    }
}

void HomaTestApp::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    recordScalar("Overall received messages", receivedMsgs);
    recordScalar("Overall sent messages", counter);
    ApplicationBase::finish();
}

L3Address HomaTestApp::chooseDestAddr(int k)
{
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void HomaTestApp::sendPacket()
{
    char msgName[100];
    int k = intrand(destAddresses.size());
    EV<<"k = "<<k<<endl;
    L3Address destAddr = chooseDestAddr(k);
    EV<<"Size of DestAddresses is "<<destAddresses.size()<<endl;
    if (destAddr == srcAddress)
    {
        k = (k+119)%(destAddresses.size());
        destAddr = chooseDestAddr(k);
        EV<<"destAddr = "<<destAddr<<endl;
    }
    EV<<"k = "<<k<<endl;
    ASSERT(destAddr != srcAddress);

    const char *localAddress = par("localAddress");
    sprintf(msgName, "%s-%lu", localAddress,counter++);
    AppMessage *appMessage = new AppMessage(msgName);
    appMessage->setByteLength(packetLength);
    appMessage->setDestAddr(destAddr);
    appMessage->setSrcAddr(srcAddress);
    appMessage->setMsgCreationTime(simTime());
    appMessage->setTransportSchedDelay(appMessage->getCreationTime());
    //emit(packetSentSignal, appMessage);
    EV<<"send packet to homa, size = "<<appMessage->getByteLength()<<", srcAddr = "<<srcAddress<<", destAddr = "<<destAddr<<endl;
    send(appMessage, "socketOut");
    numSent++;
}

void HomaTestApp::processStart()
{
    // convert local address to L3Address
    const char *localAddress = par("localAddress");
    L3AddressResolver().tryResolve(localAddress, srcAddress);
    EV<<"localAddress = "<<localAddress<<", srcAddress = "<<srcAddress<<endl;

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        destAddresses.push_back(result);
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        processSend();
    }
    else {
        if (stopTime >= SIMTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleAt(stopTime, selfMsg);
        }
    }
}

void HomaTestApp::processSend()
{
    cur_flowamount++;
    updateNextFlow(trafficMode);
    sendPacket();
    simtime_t txTime = (packetLength * 8.0 * 1e-9)/linkSpeed;
    simtime_t d = simTime() + txTime/workLoad;
    EV<<"simTIme() = "<<simTime()<<", d = "<<d<<endl;
    if ((stopTime < SIMTIME_ZERO || d < stopTime) && cur_flowamount < max_flowamount) {
        selfMsg->setKind(SEND);
        scheduleAt(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void HomaTestApp::processStop()
{
    EV<<"Stop to send message! The amount of already sent messages = "<<cur_flowamount<<endl;
}

void HomaTestApp::handleMessageWhenUp(cMessage *msg)
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

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else
        processPacket(check_and_cast<AppMessage*>(msg));
}

void HomaTestApp::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void HomaTestApp::processPacket(AppMessage *pk)
{
    //emit(packetReceivedSignal, pk);
    emit(receivedmsgID,(unsigned long)pk->getMsgId());
    emit(FlowCompletionTime,(simTime() - pk->getMsgCreationTime()).dbl());
    receivedMsgs++;
    FCT_Vector.recordWithTimestamp(simTime(), (simTime() - pk->getMsgCreationTime()).dbl());
    EV << "Message ID = "<<pk->getMsgId()<<", FCT of this flow = " << simTime() - pk->getMsgCreationTime() <<
            ", overall received msgs = "<<receivedMsgs<<endl;
    delete pk;
    numReceived++;
}

void HomaTestApp::updateNextFlow(const char* TM)
{
    double seed = uniform(0,1);
    EV<<"seed = "<<seed<<endl;
    if (std::string(TM).find("CacheFollower") != std::string::npos)
    {// 50% 0~10kb, 3% 10~100kb, 18% 100kb~1mb, 29% 1mb~, average 701kb
        if (seed<=0.01)
        {
            packetLength = 70;
        }
        else if (seed<=0.015)
        {
            packetLength = rand()%(150-71) + 71;
        }
        else if (seed<=0.04)
        {
            packetLength = 150;
        }
        else if (seed<=0.08)
        {
            packetLength = rand()%(300-151) + 151;
        }
        else if (seed<=0.1)
        {
            packetLength = rand()%(350-301) + 301;
        }
        else if (seed<=0.19)
        {
            packetLength = 350;
        }
        else if (seed<=0.2)
        {
            packetLength = rand()%(450-351) + 351;
        }
        else if (seed<=0.28)
        {
            packetLength = rand()%(500-451) + 451;
        }
        else if (seed<=0.3)
        {
            packetLength = rand()%(600-501) + 501;
        }
        else if (seed<=0.35)
        {
            packetLength = rand()%(700-601) + 601;
        }
        else if (seed<=0.4)
        {
            packetLength = rand()%(1100-701) + 701;
        }
        else if (seed<=0.42)
        {
            packetLength = rand()%(2000-1101) + 1101;
        }
        else if (seed<=0.48)
        {
            packetLength = rand()%(10000-2001) + 2001;
        }
        else if (seed<=0.5)
        {
            packetLength = rand()%(30000-10001) + 10001;
        }
        else if (seed<=0.52)
        {
            packetLength = rand()%(100000-30001) + 30001;
        }
        else if (seed<=0.6)
        {
            packetLength = rand()%(200000-100001) + 100001;
        }
        else if (seed<=0.68)
        {
            packetLength = rand()%(400000-200001) + 200001;
        }
        else if (seed<=0.7)
        {
            packetLength = rand()%(600000-400001) + 400001;
        }
        else if (seed<=0.701)
        {
            packetLength = rand()%(15000-6001)*100 + 600001;
        }
        else if (seed<=0.8)
        {
            packetLength = rand()%(20000-15001)*100 + 1500001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(24000-20001)*100 + 2000001;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(30000-24001)*100 + 2400001;
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
            packetLength = rand()%(10000-101) + 101;
        }
        else if (seed<=0.8346)
        {
            packetLength = rand()%(15252-1001)*10 + 10001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(39054-15253)*10 + 152523;
        }
        else if (seed<=0.953846)
        {
            packetLength = rand()%(32235-3906)*100 + 390542;
        }
        else if (seed<=0.99)
        {
            packetLength = rand()%(10000-323)*10000 + 3223543;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(10000-1001)*100000 + 100000001;
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
            packetLength = rand()%(300-151) + 151;
        }
        else if (seed<=0.2)
        {
            packetLength = 300;
        }
        else if (seed<=0.3)
        {
            packetLength = rand()%(1000-601) + 601;
        }
        else if (seed<=0.4)
        {
            packetLength = rand()%(2000-1001) + 1001;
        }
        else if (seed<=0.5)
        {
            packetLength = rand()%(3100-2001) + 2001;
        }
        else if (seed<=0.6)
        {
            packetLength = rand()%(6000-3101) + 3101;
        }
        else if (seed<=0.71)
        {
            packetLength = rand()%(20000-6001) + 6001;
        }
        else if (seed<=0.8)
        {
            packetLength = rand()%(6000-2001)*10 + 20001;
        }
        else if (seed<=0.82)
        {
            packetLength = rand()%(15000-6001)*10 + 60001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(30000-15001)*10 + 150001;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(50000-30001)*10 + 300001;
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
            packetLength = 9000;
        }
        else if (seed<=0.2)
        {
            packetLength = rand()%(18582-9001) + 9001;
        }
        else if (seed<=0.3)
        {
            packetLength = rand()%(28140-18583) + 18583;
        }
        else if (seed<=0.4)
        {
            packetLength = rand()%(38913-28141) + 28141;
        }
        else if (seed<=0.53)
        {
            packetLength = rand()%(7747-3892)*10 + 38914;
        }
        else if (seed<=0.6)
        {
            packetLength = rand()%(20000-7747)*10 + 77469;
        }
        else if (seed<=0.7)
        {
            packetLength = rand()%(10000-2001)*100 + 200001;
        }
        else if (seed<=0.8)
        {
            packetLength = rand()%(20000-10001)*100 + 1000001;
        }
        else if (seed<=0.9)
        {
            packetLength = rand()%(50000-20001)*100 + 2000001;
        }
        else if (seed<=0.97)
        {
            packetLength = rand()%(10000-5001)*1000 + 5000001;
        }
        else if (seed<=1)
        {
            packetLength = rand()%(30000-10001)*1000 + 10000001;
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
            packetLength = 48;
        }
        else if (seed<=0.7265)
        {
            packetLength = 56;
        }
        else if (seed<=1)
        {
            packetLength = 128;
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
            packetLength = 48;
        }
        else if (seed<=0.6345)
        {
            packetLength = 128;
        }
        else if (seed<=0.8172)
        {
            packetLength = 3544;
        }
        else if (seed<=1)
        {
            packetLength = 3552;
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
            packetLength = 48;
        }
        else if (seed<=0.0234)
        {
            packetLength = 56;
        }
        else if (seed<=0.0242)
        {
            packetLength = 64;
        }
        else if (seed<=0.025)
        {
            packetLength = 80;
        }
        else if (seed<=0.0258)
        {
            packetLength = 112;
        }
        else if (seed<=0.9115)
        {
            packetLength = 128;
        }
        else if (seed<=0.9123)
        {
            packetLength = 176;
        }
        else if (seed<=0.9131)
        {
            packetLength = 304;
        }
        else if (seed<=0.921)
        {
            packetLength = 308;
        }
        else if (seed<=0.9218)
        {
            packetLength = 560;
        }
        else if (seed<=0.9889)
        {
            packetLength = 1072;
        }
        else if (seed<=0.9697)
        {
            packetLength = 2096;
        }
        else if (seed<=0.9905)
        {
            packetLength = 4144;
        }
        else if (seed<=1)
        {
            packetLength = 16432;
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
            packetLength = 48;
        }
        else if (seed<=0.4065)
        {
            packetLength = 56;
        }
        else if (seed<=0.6638)
        {
            packetLength = 64;
        }
        else if (seed<=0.6747)
        {
            packetLength = 72;
        }
        else if (seed<=0.6954)
        {
            packetLength = 80;
        }
        else if (seed<=0.6980)
        {
            packetLength = 96;
        }
        else if (seed<=0.7180)
        {
            packetLength = 112;
        }
        else if (seed<=0.7316)
        {
            packetLength = 120;
        }
        else if (seed<=0.7445)
        {
            packetLength = 128;
        }
        else if (seed<=0.7664)
        {
            packetLength = 144;
        }
        else if (seed<=0.7873)
        {
            packetLength = 176;
        }
        else if (seed<=0.7900)
        {
            packetLength = 192;
        }
        else if (seed<=0.8109)
        {
            packetLength = 304;
        }
        else if (seed<=0.8302)
        {
            packetLength = 336;
        }
        else if (seed<=0.8495)
        {
            packetLength = 368;
        }
        else if (seed<=0.8688)
        {
            packetLength = 848;
        }
        else if (seed<=0.8881)
        {
            packetLength = 1072;
        }
        else if (seed<=0.9074)
        {
            packetLength = 1200;
        }
        else if (seed<=0.9267)
        {
            packetLength = 2640;
        }
        else if (seed<=0.9511)
        {
            packetLength = 4144;
        }
        else if (seed<=0.9756)
        {
            packetLength = 4400;
        }
        else if (seed<=1)
        {
            packetLength = 9296;
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
}

bool HomaTestApp::handleNodeStart(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
    return true;
}

bool HomaTestApp::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

void HomaTestApp::handleNodeCrash()
{
    if (selfMsg)
        cancelEvent(selfMsg);
}

} // namespace inet

