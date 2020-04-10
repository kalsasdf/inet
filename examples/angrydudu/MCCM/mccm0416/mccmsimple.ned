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

package inet.examples.angrydudu.MCCM.mccm0416;

import inet.angrydudu.MCCM.MccmSwitch.MccmSwitch;
import inet.angrydudu.MCCM.mccmhost.mccmHost;
import inet.common.misc.ThruputMeter;
import inet.angrydudu.IRN.irnhost.irnHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;
import inet.node.ethernet.EtherSwitch;


//
// TODO auto-generated type
//
network mccmsimple
{
    @display("bgb=1120.7812,468.28125");
    types:
        channel MccmChannel extends DatarateChannel
        {
            delay = 2us;
            datarate = 1000Mbps;
        }
    submodules:
        host1: mccmHost {
            @display("p=293.645,119.425;i=device/server");
        }
        host2: mccmHost {
            @display("p=293.645,191.08;i=device/server");
        }
        host3: mccmHost {
            @display("p=293.645,264.14;i=device/server");
        }
        host4: mccmHost {
            @display("p=293.645,335.795;i=device/server");
        }
        sink1: mccmHost {
            @display("p=833.165,119.425;i=device/server");
        }
        sink2: mccmHost {
            @display("p=833.165,191.08;i=device/server");
        }
        sink3: mccmHost {
            @display("p=833.165,264.14;i=device/server");
        }
        sink4: mccmHost {
            @display("p=833.165,335.795;i=device/server");
        }
        Swtich0: MccmSwitch {
            @display("p=663.16,226.205");
        }
        Switch1: MccmSwitch {
            @display("p=502.99,118.02");
        }
        Swtich2: MccmSwitch {
            @display("p=502.99,189.675");
        }
        Swtich3: MccmSwitch {
            @display("p=502.99,262.735");
        }
        Swtich4: MccmSwitch {
            @display("p=502.99,332.985");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
    connections:
        host1.ethg++ <--> MccmChannel <--> Switch1.ethg++;
        host2.ethg++ <--> MccmChannel <--> Swtich2.ethg++;
        host3.ethg++ <--> MccmChannel <--> Swtich3.ethg++;
        host4.ethg++ <--> MccmChannel <--> Swtich4.ethg++;
        Switch1.ethg++ <--> MccmChannel <--> Swtich0.ethg++;
        Swtich2.ethg++ <--> MccmChannel <--> Swtich0.ethg++;
        Swtich3.ethg++ <--> MccmChannel <--> Swtich0.ethg++;
        Swtich4.ethg++ <--> MccmChannel <--> Swtich0.ethg++;
        Swtich0.ethg++ <--> MccmChannel <--> sink1.ethg++;
        Swtich0.ethg++ <--> MccmChannel <--> sink2.ethg++;
        Swtich0.ethg++ <--> MccmChannel <--> sink3.ethg++;
        Swtich0.ethg++ <--> MccmChannel <--> sink4.ethg++;
}
