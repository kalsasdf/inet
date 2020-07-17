//
// Copyright (C) 2005 Andras Varga
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

#include "inet/common/INETDefs.h"

#include "inet/angrydudu/ECNqueue/RedEcnQueue.h"

namespace inet {

Define_Module(RedEcnQueue);

simsignal_t RedEcnQueue::queueLengthSignal = registerSignal("queueLength");
simsignal_t RedEcnQueue::bufferOccupancySignal = registerSignal("byteLength");

void RedEcnQueue::initialize()
{
    PassiveQueueBase::initialize();

    queue.setName(par("queueName"));

    //statistics
    emit(queueLengthSignal, queue.getLength());

    outGate = gate("out");

    in_pausing = false;

    // configuration
    frameCapacity = par("frameCapacity");
    bufferCapacity = par("bufferCapacity");

    enablePfc = par("enablePfc");
    enableEcn = par("enableEcn");
    Pause_max = par("Pause_max");
    Pause_min = par("Pause_min");
    kmin = par("kmin");
    kmax = par("kmax");
    pmax = par("pmax");
    //bufferOccupancyVector.setName("Buffer Occupancy/Byte");
}

cMessage *RedEcnQueue::enqueue(cMessage *msg)
{
    cPacket *packet = check_and_cast<cPacket *>(msg);
    if (byteLength + packet->getByteLength() > bufferCapacity) {
        EV << "Queue full, dropping packet.\n";
        return msg;
    }
    if (byteLength >= Pause_max && !in_pausing && enablePfc)
    {
        in_pausing = true;
        EV<<"send pause, pause ecn = 1"<<endl;
        Packet *pause = new Packet("Pause");
        const auto& Header = makeShared<Ipv4Header>();
        pause->setPacketECN(1);
        pause->setPriority(678);
        pause->setSchedulingPriority(6);
        pause->insertAtFront(Header);
        send(pause, outGate);
    }
    else if (byteLength <= Pause_min && in_pausing && enablePfc)
    {
        in_pausing = false;
        EV<<"send pause, pause ecn = 0"<<endl;
        Packet *pause = new Packet("Pause");
        const auto& Header = makeShared<Ipv4Header>();
        pause->setPacketECN(0);
        pause->setPriority(678);
        pause->setSchedulingPriority(6);
        pause->insertAtFront(Header);
        send(pause, outGate);
    }
    else {
        queue.insert(packet);
        byteLength += packet->getByteLength();
        emit(queueLengthSignal, queue.getLength());
        emit(bufferOccupancySignal, (unsigned long)byteLength);
        //return nullptr;
    }

    return nullptr;
}

cMessage *RedEcnQueue::dequeue()
{
    if (queue.isEmpty())
        return nullptr;

    EV<<"enter deququ"<<endl;

    cPacket *packet = check_and_cast<cPacket *>(queue.pop());
    byteLength -= packet->getByteLength();

    if (byteLength <= Pause_min && in_pausing && enablePfc)
    {
        in_pausing = false;
        EV<<"send pause, pause ecn = 0"<<endl;
        Packet *pause = new Packet("Pause");
        const auto& Header = makeShared<Ipv4Header>();
        pause->setPacketECN(0);
        pause->setPriority(678);
        pause->setSchedulingPriority(6);
        pause->insertAtFront(Header);
        send(pause, outGate);
    }
    //bufferOccupancyVector.recordWithTimestamp(simTime(), byteLength);
    // statistics
    emit(queueLengthSignal, queue.getLength());
    emit(bufferOccupancySignal, (unsigned long)byteLength);

    return packet;
}

void RedEcnQueue::sendOut(cMessage *packet)
{
    if (string(packet->getFullName()).find("Pause") != string::npos)
    {
        send(packet, outGate);
        return;
    }

    EV<<"sendOut(),Queue length ="<<byteLength<<endl;

    if (byteLength>kmax && enableEcn)
    {
        Packet *pck = check_and_cast<Packet*>(packet);
        auto ethHeader = pck->removeAtFront<EthernetMacHeader>();
        const auto& old = pck->removeAtFront<Ipv4Header>();
        const auto& ipv4header = makeShared<Ipv4Header>();
        ipv4header->setExplicitCongestionNotification(3);
        ipv4header->setVersion(old->getVersion());
        ipv4header->setHeaderLength(old->getHeaderLength());
        ipv4header->setSrcAddress(old->getSrcAddress());
        ipv4header->setDestAddress(old->getDestAddress());
        ipv4header->setCrc(old->getCrc());
        ipv4header->setCrcMode(old->getCrcMode());
        ipv4header->setProtocolId(old->getProtocolId());
        ipv4header->setTimeToLive(old->getTimeToLive());
        ipv4header->setIdentification(old->getIdentification());
        ipv4header->setMoreFragments(old->getMoreFragments());
        ipv4header->setDontFragment(old->getDontFragment());
        ipv4header->setFragmentOffset(old->getFragmentOffset());
        ipv4header->setTotalLengthField(old->getTotalLengthField());
        ipv4header->setDiffServCodePoint(old->getDiffServCodePoint());
        ipv4header->setOptions(old->getOptions());
        pck->insertAtFront(ipv4header);
        pck->insertAtFront(ethHeader);
        auto eHeader = pck->removeAtFront<EthernetMacHeader>();
        const auto& ipold = pck->peekAtFront<Ipv4Header>();
        EV<<"sendOut(), ecn = "<< ipold->getExplicitCongestionNotification() <<endl;
        pck->insertAtFront(eHeader);
        pck->setPacketECN(3);
        packet = check_and_cast<cPacket*>(pck);
    }
    else if (byteLength<=kmax && byteLength>kmin  && enableEcn)
    {
        if ( uniform(0,1) <= pmax * double(byteLength - kmin)/double(kmax - kmin))
        {
            Packet *pck = check_and_cast<Packet*>(packet);
            auto ethHeader = pck->removeAtFront<EthernetMacHeader>();
            const auto& old = pck->removeAtFront<Ipv4Header>();
            const auto& ipv4header = makeShared<Ipv4Header>();
            ipv4header->setExplicitCongestionNotification(3);
            ipv4header->setVersion(old->getVersion());
            ipv4header->setHeaderLength(old->getHeaderLength());
            ipv4header->setSrcAddress(old->getSrcAddress());
            ipv4header->setDestAddress(old->getDestAddress());
            ipv4header->setCrc(old->getCrc());
            ipv4header->setCrcMode(old->getCrcMode());
            ipv4header->setProtocolId(old->getProtocolId());
            ipv4header->setTimeToLive(old->getTimeToLive());
            ipv4header->setIdentification(old->getIdentification());
            ipv4header->setMoreFragments(old->getMoreFragments());
            ipv4header->setDontFragment(old->getDontFragment());
            ipv4header->setFragmentOffset(old->getFragmentOffset());
            ipv4header->setTotalLengthField(old->getTotalLengthField());
            ipv4header->setDiffServCodePoint(old->getDiffServCodePoint());
            ipv4header->setOptions(old->getOptions());
            pck->insertAtFront(ipv4header);
            pck->insertAtFront(ethHeader);
            auto eHeader = pck->removeAtFront<EthernetMacHeader>();
            const auto& ipold = pck->peekAtFront<Ipv4Header>();

            EV<<" sendOut(), randomly marking packet, ecn = "<< ipold->getExplicitCongestionNotification() <<endl;
            pck->insertAtFront(eHeader);
            pck->setPacketECN(3);
            packet = check_and_cast<cPacket*>(pck);
        }
        else // randomly no marking
        {
        }
    }
    else // queue is short, no need to mark
    {
    }
    send(packet, outGate);
}

bool RedEcnQueue::isEmpty()
{
    return queue.isEmpty();
}

} // namespace inet

