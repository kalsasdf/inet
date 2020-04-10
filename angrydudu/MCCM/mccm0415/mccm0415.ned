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

package inet.examples.angrydudu.MCCM.mccm0415;

import inet.common.misc.ThruputMeter;
import inet.angrydudu.xpass.xpasshost.xpassHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;
import inet.node.ethernet.EtherSwitch;


//
// TODO auto-generated type
//
network mccm0415
{
    @display("bgb=1121.9,399.1");
    types:
        channel CorChannel extends DatarateChannel
        {
            delay = 4us;
            datarate = 40Gbps;
        }
        channel TorChannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 10Gbps;
        }
    submodules:
        Cor1: l3Switch {
            @display("p=405,43.59375");
            gates:
                ethg[4];
        }
        Cor2: l3Switch {
            @display("p=635.625,43.59375");
            gates:
                ethg[4];
        }
        Agg1: l3Switch {
            @display("p=177.1875,151.875");
            gates:
                ethg[4];
        }
        Agg2: l3Switch {
            @display("p=405,151.875");
            gates:
                ethg[4];
        }
        Agg3: l3Switch {
            @display("p=635.625,151.875");
            gates:
                ethg[4];
        }
        Agg4: l3Switch {
            @display("p=866.25,151.875");
            gates:
                ethg[4];
        }
        Tor1: EtherSwitch {
            @display("p=118.125,255.9375");
            gates:
                ethg[5];
        }
        Tor2: EtherSwitch {
            @display("p=229.21875,255.9375");
            gates:
                ethg[5];
        }
        Tor3: EtherSwitch {
            @display("p=345.9375,255.9375");
            gates:
                ethg[5];
        }
        Tor4: EtherSwitch {
            @display("p=462.65625,255.9375");
            gates:
                ethg[5];
        }
        Tor5: EtherSwitch {
            @display("p=576.5625,255.9375");
            gates:
                ethg[5];
        }
        Tor6: EtherSwitch {
            @display("p=690.46875,255.9375");
            gates:
                ethg[5];
        }
        Tor7: EtherSwitch {
            @display("p=807.1875,255.9375");
            gates:
                ethg[5];
        }
        Tor8: EtherSwitch {
            @display("p=919.6875,255.9375");
            gates:
                ethg[5];
        }
        s1: xpassHost {
            @display("p=78.75,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s2: xpassHost {
            @display("p=102.65625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s3: xpassHost {
            @display("p=126.5625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s4: xpassHost {
            @display("p=150.46875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s5: xpassHost {
            @display("p=189.84375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s6: xpassHost {
            @display("p=213.75,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s7: xpassHost {
            @display("p=237.65625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s8: xpassHost {
            @display("p=261.5625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
        s9: xpassHost {
            @display("p=306.5625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s10: xpassHost {
            @display("p=330.46875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s11: xpassHost {
            @display("p=354.375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s12: xpassHost {
            @display("p=378.28125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s13: xpassHost {
            @display("p=423.28125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s14: xpassHost {
            @display("p=447.1875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s15: xpassHost {
            @display("p=471.09375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s16: xpassHost {
            @display("p=495,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s17: xpassHost {
            @display("p=537.1875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s18: xpassHost {
            @display("p=561.09375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s19: xpassHost {
            @display("p=585,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s20: xpassHost {
            @display("p=608.90625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s21: xpassHost {
            @display("p=651.09375,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s22: xpassHost {
            @display("p=675,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s23: xpassHost {
            @display("p=698.90625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s24: xpassHost {
            @display("p=722.8125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s25: xpassHost {
            @display("p=767.8125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s26: xpassHost {
            @display("p=791.71875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s27: xpassHost {
            @display("p=815.625,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s28: xpassHost {
            @display("p=839.53125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s29: xpassHost {
            @display("p=880.3125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s30: xpassHost {
            @display("p=904.21875,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s31: xpassHost {
            @display("p=928.125,320.625;i=device/server");
            gates:
                ethg[1];
        }
        s32: xpassHost {
            @display("p=952.03125,320.625;i=device/server");
            gates:
                ethg[1];
        }
    connections:
        s1.ethg[0] <--> TorChannel <--> Tor1.ethg[0];
        s2.ethg[0] <--> TorChannel <--> Tor1.ethg[1];
        s3.ethg[0] <--> TorChannel <--> Tor1.ethg[2];
        s4.ethg[0] <--> TorChannel <--> Tor1.ethg[3];
        s5.ethg[0] <--> TorChannel <--> Tor2.ethg[0];
        s6.ethg[0] <--> TorChannel <--> Tor2.ethg[1];
        s7.ethg[0] <--> TorChannel <--> Tor2.ethg[2];
        s8.ethg[0] <--> TorChannel <--> Tor2.ethg[3];
        s9.ethg[0] <--> TorChannel <--> Tor3.ethg[0];
        s10.ethg[0] <--> TorChannel <--> Tor3.ethg[1];
        s11.ethg[0] <--> TorChannel <--> Tor3.ethg[2];
        s12.ethg[0] <--> TorChannel <--> Tor3.ethg[3];
        s13.ethg[0] <--> TorChannel <--> Tor4.ethg[0];
        s14.ethg[0] <--> TorChannel <--> Tor4.ethg[1];
        s15.ethg[0] <--> TorChannel <--> Tor4.ethg[2];
        s16.ethg[0] <--> TorChannel <--> Tor4.ethg[3];
        s17.ethg[0] <--> TorChannel <--> Tor5.ethg[0];
        s18.ethg[0] <--> TorChannel <--> Tor5.ethg[1];
        s19.ethg[0] <--> TorChannel <--> Tor5.ethg[2];
        s20.ethg[0] <--> TorChannel <--> Tor5.ethg[3];
        s21.ethg[0] <--> TorChannel <--> Tor6.ethg[0];
        s22.ethg[0] <--> TorChannel <--> Tor6.ethg[1];
        s23.ethg[0] <--> TorChannel <--> Tor6.ethg[2];
        s24.ethg[0] <--> TorChannel <--> Tor6.ethg[3];
        s25.ethg[0] <--> TorChannel <--> Tor7.ethg[0];
        s26.ethg[0] <--> TorChannel <--> Tor7.ethg[1];
        s27.ethg[0] <--> TorChannel <--> Tor7.ethg[2];
        s28.ethg[0] <--> TorChannel <--> Tor7.ethg[3];
        s29.ethg[0] <--> TorChannel <--> Tor8.ethg[0];
        s30.ethg[0] <--> TorChannel <--> Tor8.ethg[1];
        s31.ethg[0] <--> TorChannel <--> Tor8.ethg[2];
        s32.ethg[0] <--> TorChannel <--> Tor8.ethg[3];
        Agg1.ethg[0] <--> TorChannel <--> Tor1.ethg[4];
        Agg1.ethg[1] <--> TorChannel <--> Tor2.ethg[4];
        Agg2.ethg[0] <--> TorChannel <--> Tor3.ethg[4];
        Agg2.ethg[1] <--> TorChannel <--> Tor4.ethg[4];
        Agg3.ethg[0] <--> TorChannel <--> Tor5.ethg[4];
        Agg3.ethg[1] <--> TorChannel <--> Tor6.ethg[4];
        Agg4.ethg[0] <--> TorChannel <--> Tor7.ethg[4];
        Agg4.ethg[1] <--> TorChannel <--> Tor8.ethg[4];
        Cor1.ethg[0] <--> CorChannel <--> Agg1.ethg[2];
        Cor1.ethg[1] <--> CorChannel <--> Agg2.ethg[2];
        Cor1.ethg[2] <--> CorChannel <--> Agg3.ethg[2];
        Cor1.ethg[3] <--> CorChannel <--> Agg4.ethg[2];
        Cor2.ethg[0] <--> CorChannel <--> Agg1.ethg[3];
        Cor2.ethg[1] <--> CorChannel <--> Agg2.ethg[3];
        Cor2.ethg[2] <--> CorChannel <--> Agg3.ethg[3];
        Cor2.ethg[3] <--> CorChannel <--> Agg4.ethg[3];
}
