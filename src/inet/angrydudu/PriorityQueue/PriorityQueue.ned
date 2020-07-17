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


//    Best Effort
//    DSCP_BE = 0; //
//
//    Expedited Forwarding, RFC 2598
//    DSCP_EF   = 0x2E; // 101110 credit
//
//    DSCP_AF11 = 0x0A; // 001010 data
//    DSCP_AF12 = 0x0C; // 001100
//    DSCP_AF13 = 0x0E; // 001110 
//
//    DSCP_AF21 = 0x12; // 010010 ACK
//    DSCP_AF22 = 0x14; // 010100
//    DSCP_AF23 = 0x16; // 010110
//
//    DSCP_AF31 = 0x1A; // 011010 credit_req/stop
//    DSCP_AF32 = 0x1C; // 011100
//    DSCP_AF33 = 0x1E; // 011110
//
//    DSCP_AF41 = 0x22; // 100010
//    DSCP_AF42 = 0x24; // 100100
//    DSCP_AF43 = 0x26; // 100110
//
//    DSCP_CS1  = 0x08; // 001000
//    DSCP_CS2  = 0x10; // 010000
//    DSCP_CS3  = 0x18; // 011000
//    DSCP_CS4  = 0x20; // 100000
//    DSCP_CS5  = 0x28; // 101000
//    DSCP_CS6  = 0x30; // 110000
//    DSCP_CS7  = 0x38; // 111000


package inet.angrydudu.PriorityQueue;

import inet.angrydudu.ECNqueue.RedEcnQueue;
import inet.angrydudu.xpass.xpassqueue.RandomMeter;
import inet.angrydudu.xpass.xpassqueue.xpassClassifier;

import inet.angrydudu.nic.queue.xDelayer;
import inet.common.queue.DropTailQueue;
import inet.common.queue.EcnMarkQueue;
import inet.common.queue.IOutputQueue;
import inet.common.queue.PriorityScheduler;
import inet.common.queue.Sink;
import inet.common.queue.WrrScheduler;
import inet.common.queue.FifoQueue;

import inet.networklayer.diffserv.AFxyQueue;
import inet.networklayer.diffserv.BehaviorAggregateClassifier;
import inet.networklayer.diffserv.TokenBucketMeter;



//
// This is an example queue, that can be used in
// interfaces of DS core and edge nodes to support
// the AFxy (RFC 2597) and EF (RFC 3246) PHBs.
//
// The incoming packets are first classified according to
// their DSCP field. DSCPs other than AFxy and EF are handled
// as BE (best effort).
//
// EF packets are stored in a dedicated queue, and served first
// when a packet is requested. Because they can preempt the other
// queues, the rate of the EF packets should be limited to a fraction
// of the bandwith of the link. This is achieved by metering the EF
// traffic with a token bucket meter and dropping packets that
// does not conform to the traffic profile.
//
// There are other queues for AFx classes and BE. The AFx queues
// use RED to implement 3 different drop priorities within the class.
// BE packets are stored in a drop tail queue.
// Packets from AFxy and BE queues are sheduled by a WRR scheduler,
// which ensures that the remaining bandwith is allocated among the classes
// according to the specified weights.
//
// @see ~AFxyQueue
//
module PriorityQueue like IOutputQueue
{
    parameters:
        string interfaceTableModule;
        *.interfaceTableModule = default(absPath(interfaceTableModule));
    gates:
        input in;
        output out;
    submodules:
        classifier: PriorityClassifier {
            priorities = "9 99 0 1 2 3 4 5 6 7";
            @display("p=100,330");
        }
        creditMeter: RandomMeter {
            creditsECN = true;
            dropbyECN = false;
            usebuffer = false;
            cir = default("4.7%"); // credit = 76 phy bytes (jitter 68); segment = 1526 phy bytes 47.4%
            cbs = default(620B); // 8 76B packets
            Kmin = default(620B);
            Kmax = default(620B);
            Pmax = default(1);
            creditcapacity = 10;
            @display("p=250,20;is=s");
        }
        beQueue: RedEcnQueue {
            //kmax = 999;
            //kmin = 199;
            //pmax = 0.1;
            @display("p=250,600;is=s");
        }
        sink: Sink {
            @display("p=329,20;is=vs");
        }
        creditQueue: DropTailQueue {
            frameCapacity = default(5); // keep low, for low delay and jitter
            @display("p=400,60;is=s");
        }
        CNP_Channel: RedEcnQueue {
            @display("p=250,67;is=s");
        }
        Priority_0: RedEcnQueue {
            @display("p=250,120;is=s");
        }
        Priority_1: RedEcnQueue {
            @display("p=250,180;is=s");
        }
        Priority_2: RedEcnQueue {
            @display("p=250,240;is=s");
        }
        Priority_3: RedEcnQueue {
            @display("p=250,300;is=s");
        }
        Priority_4: RedEcnQueue {
            @display("p=250,360;is=s");
        }
        Priority_5: RedEcnQueue {
            @display("p=250,420;is=s");
        }
        Priority_6: RedEcnQueue {
            @display("p=250,480;is=s");
        }
        Priority_7: RedEcnQueue {
            @display("p=250,540;is=s");
        }
        wrr: PrrScheduler {
            //Suspend = 200000;
            //EcnKmax = 200000;
            //EcnKmin = 100000;
            //%EcnPmax = 0.1;
            @display("p=400,330");
        }
        priority: PriorityScheduler {//inet/common/queue
            @display("p=550,330");
        }

        creditDelayer: xDelayer {
            @display("p=158,20;is=vs");
        }
    connections:
        in --> classifier.in;
        classifier.outs++ --> creditDelayer.in;
        classifier.outs++ --> CNP_Channel.in;
        classifier.outs++ --> Priority_0.in;
        classifier.outs++ --> Priority_1.in;
        classifier.outs++ --> Priority_2.in;
        classifier.outs++ --> Priority_3.in;
        classifier.outs++ --> Priority_4.in;
        classifier.outs++ --> Priority_5.in;
        classifier.outs++ --> Priority_6.in;
        classifier.outs++ --> Priority_7.in;
        classifier.defaultOut --> beQueue.in;

        creditMeter.greenOut --> { @display("ls=green"); } --> creditQueue.in;
        creditMeter.redOut --> { @display("ls=red"); } --> sink.in++;
        CNP_Channel.out --> wrr.in++;
        Priority_0.out --> wrr.in++;
        Priority_1.out --> wrr.in++;
        Priority_2.out --> wrr.in++;
        Priority_3.out --> wrr.in++;
        Priority_4.out --> wrr.in++;
        Priority_5.out --> wrr.in++;
        Priority_6.out --> wrr.in++;
        Priority_7.out --> wrr.in++;
        beQueue.out --> wrr.in++;
        creditQueue.out --> priority.in++;
        wrr.out --> priority.in++;
        priority.out --> out;
        creditDelayer.out --> creditMeter.in++;
}

