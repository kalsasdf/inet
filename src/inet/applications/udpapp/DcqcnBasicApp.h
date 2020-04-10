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

#ifndef __INET_DCQCNBASICAPP_H
#define __INET_DCQCNBASICAPP_H

#include <vector>

#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/homa/common/Minimal.h"
#include "inet/homa/application/MsgSizeDistributions.h"
#include "inet/homa/application/AppMessage_m.h"
#include "inet/homa/transport/HomaConfigDepot.h"
#include "inet/homa/homaheaders.h"
#include "inet/homa/transport/HomaPkt.h"

namespace inet {

/**
 * UDP application. See NED for more info.
 */
class INET_API DcqcnBasicApp : public ApplicationBase, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP };

    // parameters
    std::vector<L3Address> destAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, destPort = -1;
    L3Address srcAddress;
    simtime_t startTime;
    simtime_t stopTime;
    const char *packetName = nullptr;
    double packetLength;
    uint32_t AppPriority;

    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;
    // statistics:
    static uint32_t flowid;    // counter for generating a global number for each packet

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual L3Address chooseDestAddr();
    virtual void sendPacket();
    virtual void processPacket(Packet *msg);

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();

    virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;
    virtual void handleNodeCrash() override;

    virtual void setSocketOptions();
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
  public:
    DcqcnBasicApp() {}
    ~DcqcnBasicApp();
};

} // namespace inet

#endif // ifndef __INET_DCQCNBASICAPP_H

