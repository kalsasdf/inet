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

package inet.examples.angrydudu.routing_test;

import inet.common.misc.ThruputMeter;
import inet.angrydudu.xpass.xpasshost.xpassHost;
import inet.angrydudu.UC.UChost.ucHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network mprdmaNet
{
    @display("bgb=1121.9,399.1");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 10Gbps;
        }
    submodules:
        s1: ucHost {
            @display("p=666.5625,320.625");
            gates:
                ethg[1];
        }
        s2: ucHost {
            @display("p=731.25,320.625");
            gates:
                ethg[1];
        }
        s3: ucHost {
            @display("p=793.125,320.625");
            gates:
                ethg[1];
        }
        s4: ucHost {
            @display("p=850.78125,320.625");
            gates:
                ethg[1];
        }
        s5: ucHost {
            @display("p=912.65625,320.625");
            gates:
                ethg[1];
        }
        s6: ucHost {
            @display("p=970.3125,320.625");
            gates:
                ethg[1];
        }
        s7: ucHost {
            @display("p=1033.5938,320.625");
            gates:
                ethg[1];
        }
        s8: ucHost {
            @display("p=1094.0625,320.625");
            gates:
                ethg[1];
        }
        r1: ucHost {
            @display("p=184.21875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r2: ucHost {
            @display("p=244.6875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r3: ucHost {
            @display("p=316.40625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r4: ucHost {
            @display("p=371.25,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r5: ucHost {
            @display("p=437.34375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r6: ucHost {
            @display("p=496.40625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r7: ucHost {
            @display("p=549.84375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r8: ucHost {
            @display("p=610.3125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        Cor1: l3Switch {
            @display("p=298.125,118.125");
            gates:
                ethg[2];
        }
        Cor2: l3Switch {
            @display("p=549.84375,118.125");
            gates:
                ethg[2];
        }
        Cor3: l3Switch {
            @display("p=772.03125,118.125");
            gates:
                ethg[2];
        }
        Cor4: l3Switch {
            @display("p=1001.25,118.125");
            gates:
                ethg[2];
        }
        Tor1: l3Switch {
            @display("p=410.625,205.3125");
            gates:
                ethg[12];
        }
        Tor2: l3Switch {
            @display("p=881.71875,205.3125");
            gates:
                ethg[12];
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
            config = xml("<config>"
                        + "<interface among='r1 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r2 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r3 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r4 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r5 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r6 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r7 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r8 Tor1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s1 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s2 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s3 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s4 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s5 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s6 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s7 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s8 Tor2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='Cor* Tor*' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<multicast-group hosts='Tor*' address='224.0.0.5 224.0.0.6' />"
                        + "<multicast-group hosts='Cor*' address='224.0.0.5 224.0.0.6' />"
                        + "</config>");
        }
    connections:
        r1.ethg[0] <--> agchannel <--> Tor1.ethg[4];
        r2.ethg[0] <--> agchannel <--> Tor1.ethg[5];
        r3.ethg[0] <--> agchannel <--> Tor1.ethg[6];
        r4.ethg[0] <--> agchannel <--> Tor1.ethg[7];
        r5.ethg[0] <--> agchannel <--> Tor1.ethg[8];
        s1.ethg[0] <--> agchannel <--> Tor2.ethg[4];
        s2.ethg[0] <--> agchannel <--> Tor2.ethg[5];
        s3.ethg[0] <--> agchannel <--> Tor2.ethg[6];
        s4.ethg[0] <--> agchannel <--> Tor2.ethg[7];
        s5.ethg[0] <--> agchannel <--> Tor2.ethg[8];
        
        Tor1.ethg[9] <--> agchannel <--> r6.ethg[0];
        Tor1.ethg[10] <--> agchannel <--> r7.ethg[0];
        Tor1.ethg[11] <--> agchannel <--> r8.ethg[0];
        Tor2.ethg[9] <--> agchannel <--> s6.ethg[0];
        Tor2.ethg[10] <--> agchannel <--> s7.ethg[0];
        Tor2.ethg[11] <--> agchannel <--> s8.ethg[0];
        
        Tor1.ethg[0] <--> agchannel <--> Cor1.ethg[0];
        Tor1.ethg[1] <--> agchannel <--> Cor2.ethg[0];
        Tor1.ethg[2] <--> agchannel <--> Cor3.ethg[0];
        Tor1.ethg[3] <--> agchannel <--> Cor4.ethg[0];
        Tor2.ethg[0] <--> agchannel <--> Cor1.ethg[1];
        Tor2.ethg[1] <--> agchannel <--> Cor2.ethg[1];
        Tor2.ethg[2] <--> agchannel <--> Cor3.ethg[1];
        Tor2.ethg[3] <--> agchannel <--> Cor4.ethg[1];
}
