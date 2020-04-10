//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package inet.angrydudu.MCCM.mccmhost;

import inet.angrydudu.MCCM.MccmCore.MccmCore;
import inet.common.MessageDispatcher;
import inet.common.misc.ThruputMeter;
import inet.transportlayer.contract.ISctp;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import inet.routing.contract.IOspf;

module mccmTransportLayer extends mccmNetworkLayer
{
    parameters:
        bool hasUdp = default(true);
        bool hasTcp = default(true);
        bool hasSctp = default(false);
        @figure[transportLayer](type=rectangle; pos=1070,161; size=200,20; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");
    submodules:
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=886.725,222.075;is=s");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
            parameters:
                @display("p=700.875,222.075;is=s");
        }
        sctp: <default(firstAvailableOrEmpty("Sctp"))> like ISctp if hasSctp {
            parameters:
                @display("p=1061.5499,222.075;is=s");
        }
        tn: MessageDispatcher {
            parameters:
                @display("p=750,296;b=1000,5,,,,1");
        }
        mccm: MccmCore {
            @display("p=578.02496,222.075");
        }
    connections allowunconnected:

        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        mccm.lowerOut --> tn.in++ if hasTcp;
        mccm.lowerIn <-- tn.out++ if hasTcp;

        mccm.upperOut --> tcp.ipIn;
        mccm.upperIn <-- tcp.ipOut;

        sctp.ipOut --> tn.in++ if hasSctp;
        tn.out++ --> sctp.ipIn if hasSctp;

        tn.out++ --> ipv4.transportIn if hasIpv4;
        tn.in++ <-- ipv4.transportOut if hasIpv4;

        tn.out++ --> ipv6.transportIn if hasIpv6;
        tn.in++ <-- ipv6.transportOut if hasIpv6;

        tn.out++ --> generic.transportIn if hasGn;
        tn.in++ <-- generic.transportOut if hasGn;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;
}
