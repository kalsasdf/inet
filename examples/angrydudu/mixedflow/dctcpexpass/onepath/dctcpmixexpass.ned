//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package inet.examples.angrydudu.mixedflow.dctcpexpass.onepath;

import inet.common.misc.ThruputMeter;
import inet.angrydudu.DCQCN.dcqcnhost.dcqcnHost;
import inet.angrydudu.ANS.anshost.ansHost;
import inet.angrydudu.MPC.MpcHost.MpcHost;
import inet.angrydudu.homa.homahost.homatpHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;

network dctcpmixexpass
{
     @display("bgb=1119.785,595.72");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 10Gbps;
        }
    submodules:
        s11: dcqcnHost {
            @display("p=325.96,25.29;i=device/server");
        }
        s12: dcqcnHost {
            @display("p=325.96,81.49;i=device/server");
        }
        s13: dcqcnHost {
            @display("p=325.96,139.095;i=device/server");
        }
        s14: dcqcnHost {
            @display("p=325.96,202.32;i=device/server");
        }
        s15: dcqcnHost {
            @display("p=325.96,258.52;i=device/server");
        }
        r11: dcqcnHost {
            @display("p=866.885,25.29;i=device/server");
        }
        r12: dcqcnHost {
            @display("p=866.885,81.49;i=device/server");
        }
        r13: dcqcnHost {
            @display("p=866.885,139.095;i=device/server");
        }
        r14: dcqcnHost {
            @display("p=866.885,202.32;i=device/server");
        }
        r15: dcqcnHost {
            @display("p=866.885,257.115;i=device/server");
        }
        s21: MpcHost {
            @display("p=325.96,314.72;i=device/server");
        }
        s22: MpcHost {
            @display("p=325.96,370.92;i=device/server");
        }
        s23: MpcHost {
            @display("p=325.96,428.525;i=device/server");
        }
        s24: MpcHost {
            @display("p=325.96,491.75;i=device/server");
        }
        s25: MpcHost {
            @display("p=325.96,546.545;i=device/server");
        }
        r21: MpcHost {
            @display("p=866.885,314.72;i=device/server");
        }
        r22: MpcHost {
            @display("p=866.885,370.92;i=device/server");
        }
        r23: MpcHost {
            @display("p=866.885,428.525;i=device/server");
        }
        r24: MpcHost {
            @display("p=866.885,491.75;i=device/server");
        }
        r25: MpcHost {
            @display("p=866.885,546.545;i=device/server");
        }
        Router1: l3Switch {
            @display("p=536.71,282.405");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
        Router2: l3Switch {
            @display("p=685.64,282.405");
        }
    connections:
        s11.ethg++ <--> agchannel <--> Router1.ethg++;
        s12.ethg++ <--> agchannel <--> Router1.ethg++;
        s13.ethg++ <--> agchannel <--> Router1.ethg++;
        s14.ethg++ <--> agchannel <--> Router1.ethg++;
        s15.ethg++ <--> agchannel <--> Router1.ethg++;
        s21.ethg++ <--> agchannel <--> Router1.ethg++;
        s22.ethg++ <--> agchannel <--> Router1.ethg++;
        s23.ethg++ <--> agchannel <--> Router1.ethg++;
        s24.ethg++ <--> agchannel <--> Router1.ethg++;
        s25.ethg++ <--> agchannel <--> Router1.ethg++;
        Router2.ethg++ <--> agchannel <--> r11.ethg++;
        Router2.ethg++ <--> agchannel <--> r12.ethg++;
        Router2.ethg++ <--> agchannel <--> r13.ethg++;
        Router2.ethg++ <--> agchannel <--> r14.ethg++;
        Router2.ethg++ <--> agchannel <--> r15.ethg++;
        Router2.ethg++ <--> agchannel <--> r21.ethg++;
        Router2.ethg++ <--> agchannel <--> r22.ethg++;
        Router2.ethg++ <--> agchannel <--> r23.ethg++;
        Router2.ethg++ <--> agchannel <--> r24.ethg++;
        Router2.ethg++ <--> agchannel <--> r25.ethg++;
        Router1.ethg++ <--> agchannel <--> Router2.ethg++;
}