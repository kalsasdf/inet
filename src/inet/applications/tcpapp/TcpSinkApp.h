//
// Copyright (C) 2004 Andras Varga
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

#ifndef __INET_TCPSINKAPP_H
#define __INET_TCPSINKAPP_H

#include "inet/applications/tcpapp/TcpServerHostApp.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/tcpapp/TcpFlowApp.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/ResultFilters.h"
#include "inet/common/Simsignals_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/physicallayer/base/packetlevel/FlatReceptionBase.h"

#include "inet/physicallayer/common/packetlevel/SignalTag_m.h"

#include <stdlib.h>
#include <string.h>
namespace inet {

/**
 * Accepts any number of incoming connections, and discards whatever arrives
 * on them.
 */
class INET_API TcpSinkApp : public TcpServerHostApp
{
  protected:
    long bytesRcvd = 0;

    std::map<int,simtime_t> flow_completion_time;

    simtime_t last_flow_creation_time;
    simtime_t last_flow_time;
    simtime_t avgFCT = 0;
    int rcved_flows = 0;

    cOutVector FCT_Vector;

    static simsignal_t avgFCTsignal;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void finish() override;
    virtual void refreshDisplay() const override;

  public:
    TcpSinkApp();
    ~TcpSinkApp();

    friend class TcpSinkAppThread;
};

class INET_API TcpSinkAppThread : public TcpServerThreadBase
{
  protected:
    long bytesRcvd;
    TcpSinkApp *sinkAppModule = nullptr;

    std::map<simtime_t,simtime_t> flow_completion_time;

    simtime_t last_flow_creation_time;
    simtime_t last_flow_time;
    simtime_t avgFCT;
    int rcved_flows;

    cOutVector FCT_Vector;


    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void refreshDisplay() const override;

    //TcpServerThreadBase:
    /**
     * Called when connection is established.
     */
    virtual void established() override;

    /*
     * Called when a data packet arrives.
     */
    virtual void dataArrived(Packet *msg, bool urgent) override;

    /*
     * Called when a timer (scheduled via scheduleAt()) expires.
     */
    virtual void timerExpired(cMessage *timer) override { throw cRuntimeError("Model error: unknown timer message arrived"); }

    virtual void init(TcpServerHostApp *hostmodule, TcpSocket *socket) override { TcpServerThreadBase::init(hostmodule, socket); sinkAppModule = check_and_cast<TcpSinkApp *>(hostmod); }
};

} // namespace inet

#endif // ifndef __INET_TCPSINKAPP_H

