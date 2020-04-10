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

package inet.examples.angrydudu.UC;

import inet.common.misc.ThruputMeter;
import inet.angrydudu.UC.UChost.ucHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network ucflow
{
    @display("bgb=1120.7812,468.28125");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 100000Mbps;
        }
    submodules:
        r1: ucHost {
            @display("p=867.65625,81.5625;i=device/server");
        }
        s1: ucHost {
            @display("p=327.65625,81.5625;i=device/server");
        }
        Router1: l3Switch {
            @display("p=537.1875,226.40625");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
        s2: ucHost {
            @display("p=327.65625,153.28125;i=device/server");
        }
        s3: ucHost {
            @display("p=327.65625,226.40625;i=device/server");
        }
        s4: ucHost {
            @display("p=327.65625,298.125;i=device/server");
        }
        s5: ucHost {
            @display("p=327.65625,365.625;i=device/server");
        }
        Router2: l3Switch {
            @display("p=687.65625,226.40625");
        }
        r2: ucHost {
            @display("p=867.65625,153.28125;i=device/server");
        }
        r3: ucHost {
            @display("p=867.65625,226.40625;i=device/server");
        }
        r4: ucHost {
            @display("p=867.65625,298.125;i=device/server");
        }
        r5: ucHost {
            @display("p=867.65625,365.625;i=device/server");
        }
        h1: ucHost {
            @display("p=536.71,80.085;i=device/server");
        }
        h2: ucHost {
            @display("p=691.26,77.275;i=device/server");
        }
    connections:
        s1.ethg++ <--> agchannel <--> Router1.ethg++;
        s2.ethg++ <--> agchannel <--> Router1.ethg++;
        s3.ethg++ <--> agchannel <--> Router1.ethg++;
        s4.ethg++ <--> agchannel <--> Router1.ethg++;
        s5.ethg++ <--> agchannel <--> Router1.ethg++;
        h1.ethg++ <--> agchannel <--> Router1.ethg++;
        Router2.ethg++ <--> agchannel <--> r1.ethg++;
        Router2.ethg++ <--> agchannel <--> r2.ethg++;
        Router2.ethg++ <--> agchannel <--> r3.ethg++;
        Router2.ethg++ <--> agchannel <--> r4.ethg++;
        Router2.ethg++ <--> agchannel <--> r5.ethg++;
        Router2.ethg++ <--> agchannel <--> h2.ethg++;
        Router1.ethg++ <--> agchannel <--> Router2.ethg++;
}
