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
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network xSmallrouting
{
    @display("bgb=1121.9,399.1");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 0.01us;
            datarate = 1000Mbps;
        }
    submodules:
        s1: xpassHost {
            @display("p=622.96875,320.625");
            gates:
                ethg[1];
        }
        s2: xpassHost {
            @display("p=705.9375,320.625");
            gates:
                ethg[1];
        }
        s3: xpassHost {
            @display("p=810,320.625");
            gates:
                ethg[1];
        }
        s4: xpassHost {
            @display("p=892.96875,320.625");
            gates:
                ethg[1];
        }
        r1: xpassHost {
            @display("p=194.0625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r2: xpassHost {
            @display("p=271.40625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r3: xpassHost {
            @display("p=397.96875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        r4: xpassHost {
            @display("p=469.6875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        spine1: l3Switch {
            @display("p=351.5625,118.125");
            gates:
                ethg[4];
        }
        spine2: l3Switch {
            @display("p=760.78125,118.125");
            gates:
                ethg[4];
        }
        leaf1: l3Switch {
            @display("p=244.6875,205.3125");
            gates:
                ethg[4];
        }
        leaf2: l3Switch {
            @display("p=442.96875,205.3125");
            gates:
                ethg[4];
        }
        leaf3: l3Switch {
            @display("p=666.5625,205.3125");
            gates:
                ethg[4];
        }
        leaf4: l3Switch {
            @display("p=849.375,205.3125");
            gates:
                ethg[4];
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
            config = xml("<config>"
                        + "<interface among='r1 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r2 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r3 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='r4 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s1 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s2 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s3 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s4 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf1' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf2' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf3' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf4' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf1' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf2' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf3' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf4' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<multicast-group hosts='leaf*' address='224.0.0.5 224.0.0.6' />"
                        + "<multicast-group hosts='spine*' address='224.0.0.5 224.0.0.6' />"
                        //+ "<route hosts='r1' destination='*' gateway='leaf1'/>"
                        //+ "<route hosts='r2' destination='*' gateway='leaf1'/>"
                        //+ "<route hosts='r3' destination='*' gateway='leaf2'/>"
                        //+ "<route hosts='r4' destination='*' gateway='leaf2'/>"
                        //+ "<route hosts='s1' destination='*' gateway='leaf3'/>"
                        //+ "<route hosts='s2' destination='*' gateway='leaf3'/>"
                        //+ "<route hosts='s3' destination='*' gateway='leaf4'/>"
                        //+ "<route hosts='s4' destination='*' gateway='leaf4'/>"
                        //+ "<route hosts='spine*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth0'/>"
                        //+ "<route hosts='leaf*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth0'/>"
                        //+ "<route hosts='spine*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth1'/>"
                        //+ "<route hosts='leaf*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth1'/>"
                        //+ "<route hosts='spine*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth2'/>"
                        //+ "<route hosts='leaf*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth2'/>"
                        //+ "<route hosts='spine*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth3'/>"
                        //+ "<route hosts='leaf*' destination='224.0.0.0' netmask='240.0.0.0' interface='eth3'/>"
                        + "</config>");
        }
    connections:
        leaf1.ethg[0] <--> agchannel <--> spine1.ethg[0];
        leaf1.ethg[1] <--> agchannel <--> spine2.ethg[0];
        leaf2.ethg[0] <--> agchannel <--> spine1.ethg[1];
        leaf2.ethg[1] <--> agchannel <--> spine2.ethg[1];
        leaf3.ethg[0] <--> agchannel <--> spine1.ethg[2];
        leaf3.ethg[1] <--> agchannel <--> spine2.ethg[2];
        leaf4.ethg[0] <--> agchannel <--> spine1.ethg[3];
        leaf4.ethg[1] <--> agchannel <--> spine2.ethg[3];
        r1.ethg[0] <--> agchannel <--> leaf1.ethg[2];
        r2.ethg[0] <--> agchannel <--> leaf1.ethg[3];
        r3.ethg[0] <--> agchannel <--> leaf2.ethg[2];
        r4.ethg[0] <--> agchannel <--> leaf2.ethg[3];
        s1.ethg[0] <--> agchannel <--> leaf3.ethg[2];
        s2.ethg[0] <--> agchannel <--> leaf3.ethg[3];
        s3.ethg[0] <--> agchannel <--> leaf4.ethg[2];
        s4.ethg[0] <--> agchannel <--> leaf4.ethg[3];
}
