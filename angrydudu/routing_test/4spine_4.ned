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
network spine4s
{
    @display("bgb=1292.8275,541.0275");
    types:
        channel agchannel extends ThruputMeteringChannel
        {
            delay = 0.01us;
            datarate = 10000Mbps;
            thruputDisplayFormat = "b";
        }
    submodules:
        s1: xpassHost {
            @display("p=400.065,340.995");
            gates:
                ethg[1];
        }
        s2: xpassHost {
            @display("p=445.71,340.995");
            gates:
                ethg[1];
        }
        s3: xpassHost {
            @display("p=491.355,340.995");
            gates:
                ethg[1];
        }
        s4: xpassHost {
            @display("p=539.685,340.995");
            gates:
                ethg[1];
        }
        s5: xpassHost {
            @display("p=579.96,340.995");
            gates:
                ethg[1];
        }
        s6: xpassHost {
            @display("p=624.2625,340.995");
            gates:
                ethg[1];
        }
        s7: xpassHost {
            @display("p=684.675,340.995");
            gates:
                ethg[1];
        }
        s8: xpassHost {
            @display("p=733.005,340.995");
            gates:
                ethg[1];
        }
        s9: xpassHost {
            @display("p=778.65,340.995");
            gates:
                ethg[1];
        }
        s10: xpassHost {
            @display("p=833.6925,340.995");
            gates:
                ethg[1];
        }
        s11: xpassHost {
            @display("p=882.0225,340.995");
            gates:
                ethg[1];
        }
        s12: xpassHost {
            @display("p=933.03754,340.995");
            gates:
                ethg[1];
        }
        s13: xpassHost {
            @display("p=989.4225,340.995");
            gates:
                ethg[1];
        }
        s14: xpassHost {
            @display("p=1041.78,340.995");
            gates:
                ethg[1];
        }
        s15: xpassHost {
            @display("p=1087.425,340.995");
            gates:
                ethg[1];
        }
        s16: xpassHost {
            @display("p=1147.8375,340.995");
            gates:
                ethg[1];
        }
        s17: xpassHost {
            @display("p=1196.1675,340.995");
            gates:
                ethg[1];
        }
        s18: xpassHost {
            @display("p=1244.4976,340.995");
            gates:
                ethg[1];
        }
        r1: xpassHost {
            @display("p=114.1125,340.995;i=device/server");
            gates:
                ethg[1];
        }
        r2: xpassHost {
            @display("p=155.73,340.995;i=device/server");
            gates:
                ethg[1];
        }
        r3: xpassHost {
            @display("p=201.375,340.995;i=device/server");
            gates:
                ethg[1];
        }
        r4: xpassHost {
            @display("p=245.6775,340.995;i=device/server");
            gates:
                ethg[1];
        }
        r5: xpassHost {
            @display("p=298.035,340.995;i=device/server");
            gates:
                ethg[1];
        }
        r6: xpassHost {
            @display("p=345.0225,340.995;i=device/server");
            gates:
                ethg[1];
        }
        spine1: l3Switch {
            @display("p=245.6775,116.7975");
            gates:
                ethg[8];
        }
        spine2: l3Switch {
            @display("p=507.65625,118.125");
            gates:
                ethg[8];
        }
        spine3: l3Switch {
            @display("p=804.15753,116.7975");
            gates:
                ethg[8];
        }
        spine4: l3Switch {
            @display("p=1110.2476,116.7975");
            gates:
                ethg[8];
        }
        leaf1: l3Switch {
            @display("p=157.0725,225.54001");
            gates:
                ethg[7];
        }
        leaf2: l3Switch {
            @display("p=298.035,225.54001");
            gates:
                ethg[7];
        }
        leaf3: l3Switch {
            @display("p=445.71,225.54001");
            gates:
                ethg[7];
        }
        leaf4: l3Switch {
            @display("p=579.96,225.54001");
            gates:
                ethg[7];
        }
        leaf5: l3Switch {
            @display("p=733.005,225.54001");
            gates:
                ethg[7];
        }
        leaf6: l3Switch {
            @display("p=882.0225,225.54001");
            gates:
                ethg[7];
        }
        leaf7: l3Switch {
            @display("p=1041.78,225.54001");
            gates:
                ethg[7];
        }
        leaf8: l3Switch {
            @display("p=1196.1675,225.54001");
            gates:
                ethg[7];
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
            config = xml("<config>"
                        + "<interface among='r1 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r2 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r3 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='r4 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='r5 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='r6 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s1 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s2 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s3 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s4 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s5 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s6 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s7 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s8 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s9 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s10 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s11 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s12 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s13 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s14 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s15 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s16 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s17 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s18 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf1' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf2' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf3' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf4' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf5' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf6' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf7' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine1 leaf8' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf1' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf2' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf3' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf4' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf5' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf6' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf7' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine2 leaf8' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf1' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf2' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf3' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf4' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf5' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf6' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf7' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine3 leaf8' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf1' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf2' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf3' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf4' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf5' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf6' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf7' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<interface among='spine4 leaf8' address='10.0.0.x' netmask='255.255.255.x' />"
                        + "<multicast-group hosts='leaf*' address='224.0.0.5 224.0.0.6' />"
                        + "<multicast-group hosts='spine*' address='224.0.0.5 224.0.0.6' />"
                        + "</config>");
        }
    connections:
        leaf1.ethg[0] <--> agchannel <--> spine1.ethg[0];
        leaf1.ethg[1] <--> agchannel <--> spine2.ethg[0];
        leaf1.ethg[2] <--> agchannel <--> spine3.ethg[0];
        leaf1.ethg[3] <--> agchannel <--> spine4.ethg[0];

        leaf2.ethg[0] <--> agchannel <--> spine1.ethg[1];
        leaf2.ethg[1] <--> agchannel <--> spine2.ethg[1];
        leaf2.ethg[2] <--> agchannel <--> spine3.ethg[1];
        leaf2.ethg[3] <--> agchannel <--> spine4.ethg[1];

        leaf3.ethg[0] <--> agchannel <--> spine1.ethg[2];
        leaf3.ethg[1] <--> agchannel <--> spine2.ethg[2];
        leaf3.ethg[2] <--> agchannel <--> spine3.ethg[2];
        leaf3.ethg[3] <--> agchannel <--> spine4.ethg[2];

        leaf4.ethg[0] <--> agchannel <--> spine1.ethg[3];
        leaf4.ethg[1] <--> agchannel <--> spine2.ethg[3];
        leaf4.ethg[2] <--> agchannel <--> spine3.ethg[3];
        leaf4.ethg[3] <--> agchannel <--> spine4.ethg[3];

        leaf5.ethg[0] <--> agchannel <--> spine1.ethg[4];
        leaf5.ethg[1] <--> agchannel <--> spine2.ethg[4];
        leaf5.ethg[2] <--> agchannel <--> spine3.ethg[4];
        leaf5.ethg[3] <--> agchannel <--> spine4.ethg[4];

        leaf6.ethg[0] <--> agchannel <--> spine1.ethg[5];
        leaf6.ethg[1] <--> agchannel <--> spine2.ethg[5];
        leaf6.ethg[2] <--> agchannel <--> spine3.ethg[5];
        leaf6.ethg[3] <--> agchannel <--> spine4.ethg[5];

        leaf7.ethg[0] <--> agchannel <--> spine1.ethg[6];
        leaf7.ethg[1] <--> agchannel <--> spine2.ethg[6];
        leaf7.ethg[2] <--> agchannel <--> spine3.ethg[6];
        leaf7.ethg[3] <--> agchannel <--> spine4.ethg[6];

        leaf8.ethg[0] <--> agchannel <--> spine1.ethg[7];
        leaf8.ethg[1] <--> agchannel <--> spine2.ethg[7];
        leaf8.ethg[2] <--> agchannel <--> spine3.ethg[7];
        leaf8.ethg[3] <--> agchannel <--> spine4.ethg[7];

        r1.ethg[0] <--> agchannel <--> leaf1.ethg[4];
        r2.ethg[0] <--> agchannel <--> leaf1.ethg[5];
        r3.ethg[0] <--> agchannel <--> leaf1.ethg[6];
        r4.ethg[0] <--> agchannel <--> leaf2.ethg[4];
        r5.ethg[0] <--> agchannel <--> leaf2.ethg[5];
        r6.ethg[0] <--> agchannel <--> leaf2.ethg[6];
        s1.ethg[0] <--> agchannel <--> leaf3.ethg[4];
        s2.ethg[0] <--> agchannel <--> leaf3.ethg[5];
        s3.ethg[0] <--> agchannel <--> leaf3.ethg[6];
        s4.ethg[0] <--> agchannel <--> leaf4.ethg[4];
        s5.ethg[0] <--> agchannel <--> leaf4.ethg[5];
        s6.ethg[0] <--> agchannel <--> leaf4.ethg[6];
        s7.ethg[0] <--> agchannel <--> leaf5.ethg[4];
        s8.ethg[0] <--> agchannel <--> leaf5.ethg[5];
        s9.ethg[0] <--> agchannel <--> leaf5.ethg[6];
        s10.ethg[0] <--> agchannel <--> leaf6.ethg[4];
        s11.ethg[0] <--> agchannel <--> leaf6.ethg[5];
        s12.ethg[0] <--> agchannel <--> leaf6.ethg[6];
        s13.ethg[0] <--> agchannel <--> leaf7.ethg[4];
        s14.ethg[0] <--> agchannel <--> leaf7.ethg[5];
        s15.ethg[0] <--> agchannel <--> leaf7.ethg[6];
        s16.ethg[0] <--> agchannel <--> leaf8.ethg[4];
        s17.ethg[0] <--> agchannel <--> leaf8.ethg[5];
        s18.ethg[0] <--> agchannel <--> leaf8.ethg[6];
}
