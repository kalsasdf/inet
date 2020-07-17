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

#ifndef __INET_FIFOQUEUE_H
#define __INET_FIFOQUEUE_H

#include "inet/common/INETDefs.h"
#include "inet/common/queue/PassiveQueueBase.h"
#include "inet/common/queue/IQueueAccess.h"

#include "inet/applications/common/SocketTag_m.h"

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/EthernetCRC.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/queue/QueueBase.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Message.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/TcpIpChecksum.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETMath.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/queue/QueueBase.h"
#include "inet/common/queue/PassiveQueueBase.h"

#include "inet/linklayer/common/FcsMode_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/Ieee802SapTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/linklayer/ieee8022/Ieee8022LlcHeader_m.h"


#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/ipv4/Icmp.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
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
#include "inet/networklayer/ipv4/Ipv4.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/common/L3Address.h"

namespace inet {

/**
 * Passive FIFO Queue with unlimited buffer space.
 */
class INET_API FifoQueue : public PassiveQueueBase, public IQueueAccess
{
  protected:
    // state
    cQueue queue;
    cGate *outGate;
    double kmin;
    double kmax;
    double pmax;
    //int byteLength;
    bool enableEcn;

    // statistics
    static simsignal_t queueLengthSignal;
    static simsignal_t bufferOccupancySignal;

  public:
    //FifoQueue() : outGate(nullptr), byteLength(0) {}

  protected:
    virtual void initialize() override;

    virtual cMessage *enqueue(cMessage *msg) override;

    virtual cMessage *dequeue() override;

    virtual void sendOut(cMessage *msg) override;

    virtual bool isEmpty() override;

    virtual int getLength() const override { return queue.getLength(); }

    virtual int getByteLength() const override { return byteLength; }
};

} // namespace inet

#endif // ifndef __INET_FIFOQUEUE_H

