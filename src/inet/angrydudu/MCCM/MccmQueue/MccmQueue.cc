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

#include "inet/common/INETDefs.h"

#include "inet/angrydudu/MCCM/MccmQueue/MccmQueue.h"

namespace inet {

Define_Module(MccmQueue);

simsignal_t MccmQueue::queueLengthSignal = registerSignal("queueLength");

void MccmQueue::initialize()
{
    PassiveQueueBase::initialize();

    queue.setName(par("queueName"));

    //statistics
    emit(queueLengthSignal, queue.getLength());

    outGate = gate("out");

    // configuration
    frameCapacity = par("frameCapacity");
}

cMessage *MccmQueue::enqueue(cMessage *msg)
{
    if (frameCapacity && queue.getLength() >= frameCapacity) {
        EV << "Queue full, dropping packet.\n";
        return msg;
    }
    else {
        queue.insert(msg);
        emit(queueLengthSignal, queue.getLength());
        if (queue.getLength() >= 4)
        {
            Packet *inform = new Packet("Inform");
            EV<<" send out the inform packet: "<<inform->getName()<<endl;
            send(inform, outGate);
        }
        return nullptr;
    }
}

cMessage *MccmQueue::dequeue()
{
    if (queue.isEmpty())
        return nullptr;

    cMessage *msg = static_cast<cMessage *>(queue.pop());

    // statistics
    emit(queueLengthSignal, queue.getLength());

    return msg;
}

void MccmQueue::sendOut(cMessage *msg)
{
    send(msg, outGate);
}

bool MccmQueue::isEmpty()
{
    return queue.isEmpty();
}

} // namespace inet


