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

package inet.homa.common.homaserver;

import inet.homa.application.ITrafficGeneratorApp;
import inet.applications.contract.IApp;
import inet.common.MessageDispatcher;

module homaserverApplicationLayer extends homaserverTransportLayer
{
    parameters:
        int numApps = default(0);
        int numTrafficGeneratorApp = default(0);
        @figure[applicationLayer](type=rectangle; pos=1070,11; size=200,20; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");

        @display("bgb=1255.2749,771.75");
    submodules:
        trafficGeneratorApp[numTrafficGeneratorApp]: <default("WorkloadSynthesizer")> like ITrafficGeneratorApp {
            parameters:
                @display("p=596.4513,64.52375;is=s");
                appConfig = xmldoc("config.xml", "/topologyConfig/hostConfig[@id=$PARENTMODULE_INDEX]/appConfig");
                nicLinkSpeed = nicLinkSpeed;
                fabricLinkSpeed = fabricLinkSpeed;
                edgeLinkDelay = edgeLinkDelay;
                fabricLinkDelay = fabricLinkDelay;

                hostSwTurnAroundTime = hostSwTurnAroundTime;
                hostNicSxThinkTime = hostNicSxThinkTime;
                switchFixDelay = switchFixDelay;
        }

        app[numApps]: <> like IApp {
            parameters:
                @display("p=750.6788,64.52375,row,150;is=s");
        }
        at: MessageDispatcher {
            parameters:
                @display("p=749.10504,122.7525;b=1000,5,,,,1");
        }

    connections allowunconnected:
        for i=0..numApps-1 {
            app[i].socketOut --> at.in++;
            app[i].socketIn <-- at.out++;
        }

        for i=0..numTrafficGeneratorApp-1 {
            trafficGeneratorApp[i].transportOut --> at.in++;
            trafficGeneratorApp[i].transportIn <-- at.out++;
        }

        at.out++ --> transportScheme.upperIn++ if hasUdp;
        at.in++ <-- transportScheme.upperOut++ if hasUdp;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;

        at.out++ --> sctp.appIn if hasSctp;
        at.in++ <-- sctp.appOut if hasSctp;

        at.out++ --> tn.in++;
        at.in++ <-- tn.out++;
}
