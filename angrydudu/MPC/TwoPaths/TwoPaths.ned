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

package inet.examples.angrydudu.MPC.TwoPaths;

import inet.common.misc.ThruputMeter;
import inet.angrydudu.MPC.MpcHost.MpcHost;
import inet.angrydudu.UC.UChost.ucHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network TwoPaths
{
    @display("bgb=1023.75,397.96875");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 10Gbps;
        }
    submodules:
        s1: MpcHost {
            @display("p=600,320");
            gates:
                ethg[1];
        }
        s2: MpcHost {
            @display("p=700,320");
            gates:
                ethg[1];
        }
        s3: MpcHost {
            @display("p=800,320");
            gates:
                ethg[1];
        }
        s4: MpcHost {
            @display("p=900,320");
            gates:
                ethg[1];
        }
        r1: MpcHost {
            @display("p=100,320;i=device/server");
            gates:
                ethg[1];
        }
        r2: MpcHost {
            @display("p=200,320;i=device/server");
            gates:
                ethg[1];
        }
        r3: MpcHost {
            @display("p=300,320;i=device/server");
            gates:
                ethg[1];
        }
        r4: MpcHost {
            @display("p=400,320;i=device/server");
            gates:
                ethg[1];
        }
        Cor1: l3Switch {
            @display("p=250,100");
            gates:
                ethg[2];
        }
        Cor2: l3Switch {
            @display("p=750,100");
            gates:
                ethg[2];
        }
        Tor1: l3Switch {
            @display("p=250,200");
            gates:
                ethg[6];
        }
        Tor2: l3Switch {
            @display("p=750,200");
            gates:
                ethg[6];
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
            config = xml("<config>"
                        + "<interface among='r1 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r2 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r3 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r4 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s1 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s2 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s3 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s4 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='Cor* Tor*' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<multicast-group hosts='Tor*' address='224.0.0.5 224.0.0.6' />"
                        + "<multicast-group hosts='Cor*' address='224.0.0.5 224.0.0.6' />"
                        + "</config>");
        }
    connections:
        r1.ethg[0] <--> agchannel <--> Tor1.ethg[0];
        r2.ethg[0] <--> agchannel <--> Tor1.ethg[1];
        r3.ethg[0] <--> agchannel <--> Tor1.ethg[2];
        r4.ethg[0] <--> agchannel <--> Tor1.ethg[3];
        s1.ethg[0] <--> agchannel <--> Tor2.ethg[0];
        s2.ethg[0] <--> agchannel <--> Tor2.ethg[1];
        s3.ethg[0] <--> agchannel <--> Tor2.ethg[2];
        s4.ethg[0] <--> agchannel <--> Tor2.ethg[3];
        Tor1.ethg[4] <--> agchannel <--> Cor1.ethg[0];
        Tor1.ethg[5] <--> agchannel <--> Cor2.ethg[0];
        Tor2.ethg[4] <--> agchannel <--> Cor1.ethg[1];
        Tor2.ethg[5] <--> agchannel <--> Cor2.ethg[1];
}
