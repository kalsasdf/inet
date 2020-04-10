//
// Copyright (C) 2012 OpenSim Ltd
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
// @author: Zoltan Bojthe
//

#include "inet/angrydudu/nic/queue/xDelayer.h"
#include "inet/common/Simsignals.h"

namespace inet {

Define_Module(xDelayer);

simsignal_t xDelayer::delaySignal = registerSignal("delay");

void xDelayer::initialize()
{
    delayPar = &par("delay");
}

void xDelayer::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        emit(packetSentSignal, msg);
        send(msg, "out");
    }
    else {
        emit(packetReceivedSignal, msg);

        simtime_t delay(*delayPar);
        EV<< " xDelayer, Uniform delay = "<<delay<<endl;
        emit(delaySignal, delay);
        scheduleAt(simTime() + delay, msg);
    }
}

} // namespace inet

