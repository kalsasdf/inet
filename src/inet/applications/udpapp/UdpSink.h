//
// Copyright (C) 2004 Andras Varga
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_UDPSINK_H
#define __INET_UDPSINK_H

#include "inet/common/INETDefs.h"
#include <map>
#include<queue>

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace inet {

/**
 * Consumes and prints packets received from the Udp module. See NED for more info.
 */
class INET_API UdpSink : public ApplicationBase, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, STOP };

    UdpSocket socket;
    int localPort = -1;
    L3Address multicastGroup;
    simtime_t startTime;
    simtime_t stopTime;
    simtime_t lastpck_creation_time;
    simtime_t thisflow_creation_time;
    simtime_t lastpck_arrival_time;
    long sumflows;
    simtime_t sumFct;
    long lastflowid;
    L3Address lastflowsrc;
    int recordTimes;
    cMessage *selfMsg = nullptr;
    int linkspeed; // Mbps

    typedef struct{
            // Used for determining whether the flow has been dried.
            simtime_t creationtime;
            simtime_t fct;
            L3Address srcAddr;
        }fct_info;
    std::map<long,fct_info> flow_completion_time;

    simtime_t last_flow_creation_time;
    simtime_t last_flow_time;
    simtime_t avgFCT;
    simtime_t delta_t;
    long rcved_flows;

    cOutVector FCT_Vector;

    int numReceived = 0;

  public:
    UdpSink() {}
    virtual ~UdpSink();

  protected:
    virtual void processPacket(Packet *msg);
    virtual void setSocketOptions();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;

    virtual void processStart();
    virtual void processStop();

    virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;
    virtual void handleNodeCrash() override;
};

} // namespace inet

#endif // ifndef __INET_UDPSINK_H

