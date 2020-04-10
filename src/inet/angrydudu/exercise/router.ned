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

package inet.angrydudu.exercise;

import inet.applications.contract.IApp;
import inet.node.base.ApplicationLayerNodeBase;
import inet.routing.contract.IBgp;
import inet.routing.contract.IOspf;
import inet.routing.contract.IPim;

module router extends ApplicationLayerNodeBase
{
    parameters:
        @display("i=device/router;is=s");
        @figure[submodules];
        forwarding = true;
        bool hasOspf = default(false);
        bool hasRip = default(false);
        bool hasBgp = default(false);
        bool hasPim = default(false);
        bool hasDhcp = default(false);
        hasUdp = default(hasRip || hasDhcp);
        hasTcp = default(hasBgp);
        *.routingTableModule = default("^.ipv4.routingTable");

    submodules:
        ospf: <"Ospf"> like IOspf if hasOspf {
            parameters:
                @display("p=975,226");
        }
        bgp: <"Bgp"> like IBgp if hasBgp {
            parameters:
                ospfRoutingModule = default(hasOspf ? "^.ospf" : "");
                @display("p=600,100");
        }
        rip: <"Rip"> like IApp if hasRip {
            parameters:
                @display("p=975,76");
        }
        pim: <"Pim"> like IPim if hasPim {
            parameters:
                @display("p=825,226");
        }
        dhcp: <"DhcpServer"> like IApp if hasDhcp {
            parameters:
                @display("p=1125,76");
        }

    connections allowunconnected:
        ospf.ipOut --> tn.in++ if hasOspf;
        ospf.ipIn <-- tn.out++ if hasOspf;

        bgp.socketOut --> at.in++ if hasBgp;
        bgp.socketIn <-- at.out++ if hasBgp;

        rip.socketOut --> at.in++ if hasRip;
        rip.socketIn <-- at.out++ if hasRip;

        pim.networkLayerOut --> tn.in++ if hasPim;
        pim.networkLayerIn <-- tn.out++ if hasPim;

        dhcp.socketOut --> at.in++ if hasDhcp;
        dhcp.socketIn <-- at.out++ if hasDhcp;
}