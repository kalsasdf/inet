//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/applications/udpapp/UdpSink.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

namespace inet {

Define_Module(UdpSink);

UdpSink::~UdpSink()
{
    cancelAndDelete(selfMsg);
}

void UdpSink::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        FCT_Vector.setName("FCT");

        sumflows = 0;
        sumFct = 0;
        numReceived = 0;
        avgFCT = 0;
        lastflowid = 0;
        lastpck_arrival_time = 0;
        recordTimes = 1;
        WATCH(avgFCT);
        WATCH(numReceived);
        delta_t = par("max_interval_time");

        linkspeed = par("linkSpeed");
        localPort = par("localPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("UDPSinkTimer");
    }
}

void UdpSink::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else if (msg->arrivedOn("socketIn"))
        socket.processMessage(msg);
    else
        throw cRuntimeError("Unknown incoming gate: '%s'", msg->getArrivalGate()->getFullName());
}

void UdpSink::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void UdpSink::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void UdpSink::refreshDisplay() const
{
    char buf[50];
    sprintf(buf, "rcvd: %d pks", numReceived);
    getDisplayString().setTagArg("t", 0, buf);
}

void UdpSink::finish()
{
    auto it = flow_completion_time.begin();
    while(it!=flow_completion_time.end())
    {
        FCT_Vector.recordWithTimestamp(it->second.creationtime, it->second.fct);
        //FCT_Vector.recordWithTimestamp(SimTime(recordTimes,SIMTIME_NS), it->second.fct);
        sumFct += it->second.fct;
        ++recordTimes;
        ++it;
    }

    sumflows = flow_completion_time.size();

    if (sumflows!=0)
    {
        avgFCT = sumFct/sumflows;
    }

    recordScalar("average Flow Completion Time",avgFCT);
    recordScalar("sum flows",double(sumflows));
    recordScalar("sum FCT",sumFct);

    ApplicationBase::finish();
    EV_INFO << getFullPath() << ": received " << numReceived << " packets\n";
}

void UdpSink::setSocketOptions()
{
    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);

    // join multicastGroup
    const char *groupAddr = par("multicastGroup");
    multicastGroup = L3AddressResolver().resolve(groupAddr);
    if (!multicastGroup.isUnspecified()) {
        if (!multicastGroup.isMulticast())
            throw cRuntimeError("Wrong multicastGroup setting: not a multicast address: %s", groupAddr);
        socket.joinMulticastGroup(multicastGroup);
    }
    socket.setCallback(this);
}

void UdpSink::processStart()
{
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    setSocketOptions();

    if (stopTime >= SIMTIME_ZERO) {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void UdpSink::processStop()
{
    if (!multicastGroup.isUnspecified())
        socket.leaveMulticastGroup(multicastGroup); // FIXME should be done by socket.close()
    socket.close();
}

void UdpSink::processPacket(Packet *pk)
{
    auto addressReq = pk->addTagIfAbsent<L3AddressInd>();
    L3Address this_flow_src = addressReq->getSrcAddress();

    long this_flow_id = pk->peekAtBack<ApplicationPacket>()->getFlowid();
    //long this_flow_id = pk->getSrcProcId();
    simtime_t this_creationTime;

    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    EV << "The flow id of the Packet is "<<this_flow_id<<", source address = "<<addressReq->getSrcAddress()<<endl;

    for (auto& region : pk->peekData()->getAllTags<CreationTimeTag>())
        this_creationTime = region.getTag()->getCreationTime();

    lastpck_arrival_time = simTime();


    if (flow_completion_time.empty())
    {
        fct_info thisinfo = {this_creationTime,simTime()-this_creationTime,this_flow_src};
        flow_completion_time.insert(std::make_pair(this_flow_id,thisinfo));
    }

    bool newflow = true;
    auto it = flow_completion_time.find(this_flow_id);
    if(it != flow_completion_time.end())
    {
        fct_info thisinfo = it->second;
        if((simTime()-thisinfo.creationtime)-thisinfo.fct>0.005)
        {
            if((simTime()-thisinfo.creationtime)-thisinfo.fct>0.005)
            {
                if(this_flow_id<0)
                    throw cRuntimeError("too many flows!");
                //throw cRuntimeError("packet interval is too long!");
            }
        }
        thisinfo.fct = simTime() - thisinfo.creationtime;
        flow_completion_time[this_flow_id] = thisinfo;
        lastflowid = this_flow_id;
        lastflowsrc = this_flow_src;
        newflow = false;
        EV<<"this flow completion times = "<<thisinfo.fct<<endl;
    }

    if (newflow)
    {
        sumflows++;
        fct_info thisinfo = {this_creationTime,simTime()-this_creationTime,this_flow_src};
        flow_completion_time[this_flow_id] = thisinfo;
        lastflowid = this_flow_id;
    }

    emit(packetReceivedSignal, pk);
    delete pk;

    numReceived++;
}

bool UdpSink::handleNodeStart(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
    return true;
}

bool UdpSink::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

void UdpSink::handleNodeCrash()
{
    if (selfMsg)
        cancelEvent(selfMsg);
}

} // namespace inet

