//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 Andras Varga
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

#ifndef __INET_UDPTESTAPP_H
#define __INET_UDPTESTAPP_H

#include <vector>
#include <map>

#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace inet {

/**
 * UDP application. See NED for more info.
 */
class INET_API UdpTestApp : public ApplicationBase, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP, SENDLASTPCK, NEWFLOW };

    static long overallflowid;
    long flowid;
    L3Address flowDest;

    std::map<int,simtime_t> flow_creation_time;
    // parameters
    std::vector<L3Address> destAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, destPort = -1;
    simtime_t startTime;
    simtime_t stopTime;
    simtime_t sendInterval;
    simtime_t sleepTime;
    simtime_t burstTime;
    int cur_flowamount;
    int max_flowamount;

    simtime_t thisburst_start_time;

    double multiple_of_linkspeed; // sending packet at the speed of n multiple of linkspeed;
    int randomseed;
    int packetLength;
    double linkspeed; // Mbps
    double load;
    double flowsize; // KB
    bool test;

    std::map<L3Address,int> seq_cache;

    int lastpckLength;

    const char *packetName = nullptr;
    const char *trafficMode = nullptr;

    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;

    cOutVector FlowSizes_Vector;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual L3Address chooseDestAddr();
    virtual void sendPacket(int length);
    virtual void processPacket(Packet *msg);
    virtual void setSocketOptions();

    virtual void processStart();
    virtual void processSend();
    virtual void processSendLastPck();
    virtual void processStop();
    virtual void updateNextFlow(const char* TM);

    virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;
    virtual void handleNodeCrash() override;

    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;

  public:
    UdpTestApp() {}
    ~UdpTestApp();
};

} // namespace inet

#endif // ifndef __INET_UDPTESTAPP_H

