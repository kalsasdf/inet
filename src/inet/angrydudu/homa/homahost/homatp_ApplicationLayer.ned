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

package inet.angrydudu.homa.homahost;

import inet.applications.contract.IApp;
import inet.common.MessageDispatcher;
import inet.homa.transport.HomaTransport;
import inet.homa.transport.ITransportScheme;
import inet.homa.application.ITrafficGeneratorApp;

module homatpApplicationLayer extends homatpTransportLayer
{
    parameters:
        int numApps = default(0);
        int numTrafficGeneratorApp = default(0);
        @figure[applicationLayer](type=rectangle; pos=1070,11; size=200,20; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");
        bool hasTransportScheme = default(numTrafficGeneratorApp>0);
        @display("bgb=1255.2749,771.75");
    submodules:
        app[numApps]: <> like IApp {
            parameters:
                @display("p=750.6788,64.52375,row,150;is=s");
        }
        at: MessageDispatcher {
            parameters:
                @display("p=749.10504,174.68626;b=1000,5,,,,1");
        }
        //homa: <default(firstAvailableOrEmpty("HomaTransport"))> like ITransportScheme if hasTransportScheme {
        //    parameters:
        //        @display("p=618.48376,78.6875;is=s");
        //        transportConfig = xmldoc("config.xml", "/topologyConfig/hostConfig[@id=$PARENTMODULE_INDEX]/transportConfig");
        //        nicLinkSpeed = nicLinkSpeed;
        //}
        homa: HomaTransport {
            @display("p=596.4513,113.310005;is=s");
        }
        trafficGeneratorApp[numTrafficGeneratorApp]: <default("WorkloadSynthesizer")> like ITrafficGeneratorApp {
            parameters:
                @display("p=498.87878,64.52375;is=s");
                appConfig = xmldoc("config.xml", "/topologyConfig/hostConfig[@id=$PARENTMODULE_INDEX]/appConfig");
                nicLinkSpeed = nicLinkSpeed;
                fabricLinkSpeed = fabricLinkSpeed;
                edgeLinkDelay = edgeLinkDelay;
                fabricLinkDelay = fabricLinkDelay;

                hostSwTurnAroundTime = hostSwTurnAroundTime;
                hostNicSxThinkTime = hostNicSxThinkTime;
                switchFixDelay = switchFixDelay;
        }

    connections allowunconnected:
        for i=0..numApps-1 {
            app[i].socketOut --> homa.upperIn++;
            app[i].socketIn <-- homa.upperOut++;
        }
        //for i=0..numTrafficGeneratorApp-1 {
        //    trafficGeneratorApp[i].transportOut --> homa.upperIn++;
        //    trafficGeneratorApp[i].transportIn <-- homa.upperOut++;
        //}

        homa.lowerOut --> at.in++;
        homa.lowerIn <-- at.out++;

        at.out++ --> udp.appIn if hasUdp;
        at.in++ <-- udp.appOut if hasUdp;

        //at.out++ --> homa.upperIn if hasUdp;
        //at.in++ <-- homa.upperOut if hasUdp;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;

        at.out++ --> sctp.appIn if hasSctp;
        at.in++ <-- sctp.appOut if hasSctp;

        at.out++ --> tn.in++;
        at.in++ <-- tn.out++;
}
