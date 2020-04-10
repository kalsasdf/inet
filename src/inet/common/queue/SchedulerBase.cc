//
// Copyright (C) 2012 Opensim Ltd.
// Author: Tamas Borbely
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

#include <algorithm>

#include "inet/common/queue/SchedulerBase.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Message.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/TcpIpChecksum.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/arp/ipv4/ArpPacket_m.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/common/EcnTag_m.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/common/MulticastTag_m.h"
#include "inet/networklayer/common/NextHopAddressTag_m.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4SocketCommand_m.h"
#include "inet/networklayer/ipv4/IcmpHeader_m.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/linklayer/ethernet/EtherMacFullDuplex.h"

#include "inet/common/queue/IPassiveQueue.h"
#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"


namespace inet {

SchedulerBase::SchedulerBase()
    : packetsRequestedFromUs(0), packetsToBeRequestedFromInputs(0), outGate(nullptr)
{
}

SchedulerBase::~SchedulerBase()
{
}

void SchedulerBase::initialize()
{
    int numInputs = gateSize("in");
    for (int i = 0; i < numInputs; ++i) {
        cGate *inGate = isGateVector("in") ? gate("in", i) : gate("in");
        cGate *connectedGate = inGate->getPathStartGate();
        if (!connectedGate)
            throw cRuntimeError("Scheduler input gate %d is not connected", i);
        IPassiveQueue *inputModule = dynamic_cast<IPassiveQueue *>(connectedGate->getOwnerModule());
        if (!inputModule)
            throw cRuntimeError("Scheduler input gate %d should be connected to an IPassiveQueue", i);
        inputModule->addListener(this);
        inputQueues.push_back(inputModule);
    }

    outGate = gate("out");

    // TODO update state when topology changes
}

void SchedulerBase::finalize()
{
    for (auto & elem : inputQueues)
        (elem)->removeListener(this);
}

void SchedulerBase::handleMessage(cMessage *msg)
{
    int priority = check_and_cast<Packet*>(msg)->getPriority();
    if (check_and_cast<Packet*>(msg)->getPriority() == 678){
        priority = msg->getArrivalGate()->getIndex();
    }
    EV<<"msg->getArrivalGateId() = "<<priority<<endl;
    ASSERT(packetsRequestedFromUs > 0);
    if ((string(msg->getFullName()).find("Suspend") == string::npos && string(msg->getFullName()).find("Pause") == string::npos) || check_and_cast<Packet*>(msg)->getPriority() == 99)
    {
        EV<<"packetsRequestedFromUs = "<<packetsRequestedFromUs<<endl;
        packetsRequestedFromUs--;
    }
    if (string(msg->getFullName()).find("Pause") != string::npos)
    {
        if (check_and_cast<Packet*>(msg)->getSchedulingPriority() == 99)
        {
            PausingQi(check_and_cast<Packet*>(msg)->getPriority(),check_and_cast<Packet*>(msg)->getPacketECN());
        }
        else
        {
            check_and_cast<Packet*>(msg)->setPriority(priority);
        }
    }
    sendOut(msg);
}

void SchedulerBase::packetEnqueued(IPassiveQueue *inputQueue)
{
    Enter_Method("packetEnqueued(...)");

    if (packetsToBeRequestedFromInputs > 0) {
        bool success = schedulePacket();
        if (success)
            packetsToBeRequestedFromInputs--;
    }
    else if (packetsRequestedFromUs == 0)
        notifyListeners();
}

void SchedulerBase::requestPacket()
{
    Enter_Method("requestPacket()");

    packetsRequestedFromUs++;
    packetsToBeRequestedFromInputs++;
    bool success = schedulePacket();
    if (success)
        packetsToBeRequestedFromInputs--;
}

void SchedulerBase::sendOut(cMessage *msg)
{
    if (string(check_and_cast<Packet*>(msg)->getFullName()).find("CNP") == string::npos &&
            (string(check_and_cast<Packet*>(msg)->getFullName()).find("Data") != string::npos
                    || string(check_and_cast<Packet*>(msg)->getFullName()).find("Msg") != string::npos) )
    {
        const auto& macheader = check_and_cast<Packet*>(msg)->removeAtFront<EthernetMacHeader>();
        const auto& ipv4header = check_and_cast<Packet*>(msg)->peekAtFront<Ipv4Header>();
        if (markEcn())
        {
            check_and_cast<Packet*>(msg)->setPacketECN(3);
        }
        if (sendSuspend() == 1)
        {
            EV<<"send suspend, suspend ecn = 1"<<endl;
            in_suspending = true;
            Packet *sus = new Packet("Suspend");
            const auto& Header = makeShared<Ipv4Header>();
            Header->setDestAddress(ipv4header->getDestAddress());
            sus->setPacketECN(1);
            sus->setPriority(0);
            sus->insertAtFront(Header);
            send(sus, outGate);
        }
        else if (sendSuspend() == -1 && in_suspending)
        {
            EV<<"send suspend, suspend ecn = 0"<<endl;
            in_suspending = false;
            Packet *sus = new Packet("Suspend");
            const auto& Header = makeShared<Ipv4Header>();
            Header->setDestAddress(ipv4header->getDestAddress());
            sus->setPacketECN(0);
            sus->setPriority(0);
            sus->insertAtFront(Header);
            send(sus, outGate);
        }// else sendSuspend = 0
        check_and_cast<Packet*>(msg)->insertAtFront(macheader);
    }
    EV<<"the packet ECN = "<<check_and_cast<Packet*>(msg)->getPacketECN()<<endl;
    send(msg, outGate);
}

bool SchedulerBase::isEmpty()
{
    for (auto & elem : inputQueues)
        if (!(elem)->isEmpty())
            return false;

    return true;
}

void SchedulerBase::clear()
{
    for (auto & elem : inputQueues)
        (elem)->clear();

    packetsRequestedFromUs = 0;
    packetsToBeRequestedFromInputs = 0;
}

cMessage *SchedulerBase::pop()
{
    for (auto & elem : inputQueues)
        if (!(elem)->isEmpty())
            return (elem)->pop();

    return nullptr;
}

void SchedulerBase::addListener(IPassiveQueueListener *listener)
{
    auto it = find(listeners.begin(), listeners.end(), listener);
    if (it == listeners.end())
        listeners.push_back(listener);
}

void SchedulerBase::removeListener(IPassiveQueueListener *listener)
{
    auto it = find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end())
        listeners.erase(it);
}

void SchedulerBase::notifyListeners()
{
    for (auto & elem : listeners)
        (elem)->packetEnqueued(this);
}

} // namespace inet

