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

package inet.examples.angrydudu.exercise;

import inet.angrydudu.exercise.standardhost.Host;
import inet.common.misc.ThruputMeter;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.angrydudu.exercise.router;


//
// TODO auto-generated type
//
network onepath
{
    @display("bgb=1119.785,595.72");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 1us;
            datarate = 10 Gbps;
        }
    submodules:
        r1: Host {
            @display("p=866.885,25.29;i=device/server");
        }
        s1: Host {
            @display("p=325.96,25.29;i=device/server");
        }
        Router1: router {
            @display("p=523.59875,289.17252");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
        s2: Host {
            @display("p=325.67,96.85875;i=device/server");
        }
        s3: Host {
            @display("p=325.67,171.2575;i=device/server");
        }
        s4: Host {
            @display("p=325.67,234.42625;i=device/server");
        }
        s5: Host {
            @display("p=325.67,334.0925;i=device/server");
        }
        Router2: router {
            @display("p=699.0675,289.17252");
        }
        r2: Host {
            @display("p=866.1138,96.85875;i=device/server");
        }
        r3: Host {
            @display("p=866.1138,171.2575;i=device/server");
        }
        r4: Host {
            @display("p=866.1138,234.42625;i=device/server");
        }
        r5: Host {
            @display("p=866.1138,345.3225;i=device/server");
        }
        s6: Host {
            @display("p=325.67,412.7025;i=device/server");
        }
        s7: Host {
            @display("p=325.67,478.67877;i=device/server");
        }
        s8: Host {
            @display("p=325.67,558.6925;i=device/server");
        }
        r6: Host {
            @display("p=866.1138,412.7025;i=device/server");
        }
        r7: Host {
            @display("p=866.1138,478.67877;i=device/server");
        }
        r8: Host {
            @display("p=866.1138,558.6925;i=device/server");
        }
    connections:
        s1.ethg++ <--> agchannel <--> Router1.ethg++;
        s2.ethg++ <--> agchannel <--> Router1.ethg++;
        s3.ethg++ <--> agchannel <--> Router1.ethg++;
        s4.ethg++ <--> agchannel <--> Router1.ethg++;
        s5.ethg++ <--> agchannel <--> Router1.ethg++;
        s6.ethg++ <--> agchannel <--> Router1.ethg++;
        s7.ethg++ <--> agchannel <--> Router1.ethg++;
        s8.ethg++ <--> agchannel <--> Router1.ethg++;
        Router2.ethg++ <--> agchannel <--> r1.ethg++;
        Router2.ethg++ <--> agchannel <--> r2.ethg++;
        Router2.ethg++ <--> agchannel <--> r3.ethg++;
        Router2.ethg++ <--> agchannel <--> r4.ethg++;
        Router2.ethg++ <--> agchannel <--> r5.ethg++;
        Router2.ethg++ <--> agchannel <--> r6.ethg++;
        Router2.ethg++ <--> agchannel <--> r7.ethg++;
        Router2.ethg++ <--> agchannel <--> r8.ethg++;
        Router1.ethg++ <--> agchannel <--> Router2.ethg++;
}
