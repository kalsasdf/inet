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
#include "inet/applications/udpapp/HomaBasicApp.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

namespace inet {

Define_Module(HomaBasicApp);

int HomaBasicApp::counter;

HomaBasicApp::~HomaBasicApp()
{
    cancelAndDelete(selfMsg);
}

void HomaBasicApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        counter = 0;
        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("sendTimer");
    }
}

void HomaBasicApp::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    ApplicationBase::finish();
}

L3Address HomaBasicApp::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void HomaBasicApp::sendPacket()
{
    char msgName[100];
    L3Address destAddr = chooseDestAddr();
    const char *localAddress = par("localAddress");
    sprintf(msgName, "%s-%d", localAddress,counter++);
    AppMessage *appMessage = new AppMessage(msgName);
    appMessage->setByteLength(par("messageLength"));
    appMessage->setDestAddr(destAddr);
    appMessage->setSrcAddr(srcAddress);
    appMessage->setMsgCreationTime(simTime());
    appMessage->setTransportSchedDelay(appMessage->getCreationTime());
    //emit(packetSentSignal, appMessage);
    EV<<"send packet to homa, size = "<<appMessage->getByteLength()<<", srcAddr = "<<srcAddress<<endl;
    send(appMessage, "socketOut");
    numSent++;
}

void HomaBasicApp::processStart()
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

void HomaBasicApp::processSend()
{
    sendPacket();
    simtime_t d = simTime() + par("sendInterval");
    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        selfMsg->setKind(SEND);
        scheduleAt(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void HomaBasicApp::processStop()
{
}

void HomaBasicApp::handleMessageWhenUp(cMessage *msg)
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
        processPacket(check_and_cast<AppMessage *>(msg));
}

void HomaBasicApp::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void HomaBasicApp::processPacket(AppMessage *pk)
{
    //emit(packetReceivedSignal, pk);
    //EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    delete pk;
    numReceived++;
}

bool HomaBasicApp::handleNodeStart(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
    return true;
}

bool HomaBasicApp::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

void HomaBasicApp::handleNodeCrash()
{
    if (selfMsg)
        cancelEvent(selfMsg);
}

} // namespace inet

