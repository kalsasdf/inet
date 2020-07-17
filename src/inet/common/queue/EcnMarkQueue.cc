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

#include "inet/common/queue/EcnMarkQueue.h"

namespace inet {

Define_Module(EcnMarkQueue);

simsignal_t EcnMarkQueue::queueLengthSignal = registerSignal("queueLength");

void EcnMarkQueue::initialize()
{
    PassiveQueueBase::initialize();

    queue.setName(par("queueName"));

    //statistics
    emit(queueLengthSignal, queue.getLength());

    outGate = gate("out");

    // configuration
    frameCapacity = par("frameCapacity");
}

cMessage *EcnMarkQueue::enqueue(cMessage *msg)
{
    if (frameCapacity && queue.getLength() >= frameCapacity) {
        EV << "Queue full, dropping packet.\n";
        return msg;
    }
    else {
        queue.insert(msg);
        emit(queueLengthSignal, queue.getLength());
        return nullptr;
    }
}

cMessage *EcnMarkQueue::dequeue()
{
    EV<<"enter dequeue."<<endl;
    if (queue.isEmpty())
        return nullptr;

    cMessage *msg = static_cast<cMessage *>(queue.pop());

    // statistics
    emit(queueLengthSignal, queue.getLength());

    return msg;
}

void EcnMarkQueue::sendOut(cMessage *packet)
{
    EV<<"enter sendout"<<endl;
    Packet *pck = check_and_cast<Packet*>(packet);
    /*auto ethHeader = pck->removeAtFront<EthernetMacHeader>();
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
    EV<<"meterPackets(), ecn = "<< ipold->getExplicitCongestionNotification() <<endl;
    pck->insertAtFront(eHeader);
    packet = check_and_cast<cPacket*>(pck);*/
    send(pck, outGate);
}

bool EcnMarkQueue::isEmpty()
{
    return queue.isEmpty();
}

} // namespace inet

