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


package inet.angrydudu.ECNqueue;

import inet.common.queue.IOutputQueue;
//
// Drop-tail queue, to be used in network interfaces.
// Conforms to the ~IOutputQueue interface.
//
simple RedEcnQueue like IOutputQueue
{
    parameters:
        int frameCapacity = default(150);
        int bufferCapacity @unit(B)= default(5000000B);
        double kmin @unit(B)= default(999999B);
        double kmax @unit(B)= default(999999B);
        double pmax = default(0);
    	int Pause_max @unit(B)= default(80000B);
    	int Pause_min @unit(B)= default(10000B);
    	bool enablePfc = default(false);
    	bool enableEcn = default(false);
        string queueName = default("l2queue"); // name of the inner cQueue object, used in the 'q' tag of the display string
        @display("i=block/queue");
        @signal[packetReceived](type=cPacket);
        @signal[packetEnqueued](type=cPacket);
        @signal[packetDequeued](type=cPacket);
        @signal[packetDropped](type=cPacket);
        @signal[queueingTime](type=simtime_t; unit=s);
        @signal[queueLength](type=long);
        @signal[byteLength](type=unsigned long);
        //@statistic[packetReceived](title="received packets"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[dropPk](title="dropped packets"; source=packetDropped; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        //@statistic[queueingTime](title="queueing time"; record=histogram,vector; interpolationmode=none);
        //@statistic[queueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);
        @statistic[byteLength](title="buffer occupancy"; record=max,timeavg,vector; interpolationmode=sample-hold);
    gates:
        input in;
        output out;
}

