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

package inet.examples.angrydudu.IRN.fiveflows;

import inet.common.misc.ThruputMeter;
import inet.angrydudu.IRN.irnhost.irnHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network irnfiveflows
{
    @display("bgb=1120.7812,468.28125");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 1000Mbps;
        }
    submodules:
        host1: irnHost {
            @display("p=327.65625,81.5625;i=device/server");
        }
        host2: irnHost {
            @display("p=327.65625,153.28125;i=device/server");
        }
        host3: irnHost {
            @display("p=327.65625,226.40625;i=device/server");
        }
        host4: irnHost {
            @display("p=327.65625,298.125;i=device/server");
        }
        host5: irnHost {
            @display("p=327.65625,365.625;i=device/server");
        }
        sink1: irnHost {
            @display("p=867.65625,81.5625;i=device/server");
        }
        sink2: irnHost {
            @display("p=867.65625,153.28125;i=device/server");
        }
        sink3: irnHost {
            @display("p=867.65625,226.40625;i=device/server");
        }
        sink4: irnHost {
            @display("p=867.65625,298.125;i=device/server");
        }
        sink5: irnHost {
            @display("p=867.65625,365.625;i=device/server");
        }
        Router1: l3Switch {
            @display("p=537.1875,226.40625");
        }
        Router2: l3Switch {
            @display("p=687.65625,226.40625");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
    connections:
        host1.ethg++ <--> agchannel <--> Router1.ethg++;
        host2.ethg++ <--> agchannel <--> Router1.ethg++;
        host3.ethg++ <--> agchannel <--> Router1.ethg++;
        host4.ethg++ <--> agchannel <--> Router1.ethg++;
        host5.ethg++ <--> agchannel <--> Router1.ethg++;
        Router1.ethg++ <--> agchannel <--> Router2.ethg++;
        Router2.ethg++ <--> agchannel <--> sink1.ethg++;
        Router2.ethg++ <--> agchannel <--> sink2.ethg++;
        Router2.ethg++ <--> agchannel <--> sink3.ethg++;
        Router2.ethg++ <--> agchannel <--> sink4.ethg++;
        Router2.ethg++ <--> agchannel <--> sink5.ethg++;
}
