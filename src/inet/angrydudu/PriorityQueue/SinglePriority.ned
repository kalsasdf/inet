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

package inet.angrydudu.PriorityQueue;

import inet.common.queue.FifoQueue;
import inet.common.queue.RedDropper;
import inet.common.queue.ThresholdDropper;


//
// This is an example queue, that implements
// one class of the Assured Forwarding PHB group (RFC 2597).
//
// Packets with the same AFx class, but different drop priorities
// arrive at the afx1In, afx2In, and afx3In gates. The received
// packets are stored in the same queue. Before the packet
// is enqueued, a RED dropping algorithm may decide to selectively
// drop them, based on the average length of the queue and the RED parameters
// of the drop priority of the packet.
//
// The afxyMinth, afxyMaxth, and afxyMaxp parameters must have values that
// ensures that packets with lower drop priorities are dropped with lower
// or equal probability than packets with higher drop priorities.
//
// @see ~DiffservQueue
//
module SinglePriority
{
    parameters:
        @display("i=block/queue;q=l2queue");

    gates:
        input in;
        output out;
    submodules:
        fifoQueue: FifoQueue {
            @display("p=250,100");
        }
        ThresholdDropper: ThresholdDropper {
            numGates = 1;
			//frameCapacity = 200; // if positive, then limits the sum of frames in output queues
			byteCapacity = 10000000;  // if positive, then limits the sum of bytes in the output queues
            @display("p=100,100");
        }
    connections:
        in --> { @display("m=w"); } --> ThresholdDropper.in[0];
        ThresholdDropper.out[0] --> { @display("m=m,100,20,0,50"); } --> fifoQueue.in++;
        fifoQueue.out --> { @display("m=e"); } --> out;
}
