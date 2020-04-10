//
// Copyright (C) 2012 Opensim Ltd.
// Author: Tamas Borbely
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

#ifndef __INET_PRRSCHEDULER_H
#define __INET_PRRSCHEDULER_H

#include "inet/common/INETDefs.h"
#include "inet/common/queue/SchedulerBase.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Message.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/TcpIpChecksum.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/arp/ipv4/ArpPacket_m.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/common/EcnTag_m.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/common/MulticastTag_m.h"
#include "inet/networklayer/common/NextHopAddressTag_m.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4SocketCommand_m.h"
#include "inet/networklayer/ipv4/IcmpHeader_m.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/linklayer/ethernet/EtherMacFullDuplex.h"

#include "inet/common/queue/IPassiveQueue.h"
#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"

namespace inet {

/**
 * This module implements a Weighted Round Robin Scheduler.
 */
class INET_API PrrScheduler : public SchedulerBase
{
  protected:
    uint32_t EcnKmax;
    uint32_t EcnKmin;
    double EcnPmax;
    int numInputs;    // number of input gates
    int *weights;    // array of weights (has numInputs elements)
    int *buckets;    // array of tokens in buckets (has numInputs elements)
    uint32_t sharedBufferSize;
    uint32_t sharedBufferOccupancy;
    uint32_t Suspend_max;
    uint32_t Suspend_min;

    uint32_t Pause_max;
    uint32_t Pause_min;

    bool enableSuspend;
    bool enablePfc;
    bool shareBufferEcn;

    static simsignal_t BufferOccupancySignal;
    std::map<int,bool> ispaused;
    std::map<int,bool> pausesent;
  public:
    PrrScheduler() : numInputs(0), weights(nullptr), buckets(nullptr) {}

    virtual uint32_t getQueueByteLength() override {return 0;}
  protected:
    virtual ~PrrScheduler();
    virtual void initialize() override;
    virtual bool schedulePacket() override;
    virtual bool markEcn() override;
    virtual int sendSuspend() override;
    virtual int sendPause(int Qn) override;
    virtual bool PausingQi(int Qn,bool pause) override;
};

} // namespace inet

#endif // ifndef __INET_PRIOSCHEDULER_H

