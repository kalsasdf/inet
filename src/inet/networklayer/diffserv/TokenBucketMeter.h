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

#ifndef __INET_TOKENBUCKETMETER_H
#define __INET_TOKENBUCKETMETER_H

#include <queue>

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/EthernetCRC.h"
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

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/ipv4/Icmp.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/common/queue/QueueBase.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"

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

#include "inet/networklayer/ipv4/Ipv4.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/common/EcnTag_m.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETMath.h"


#include "inet/common/INETDefs.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Message.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/TcpIpChecksum.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/queue/QueueBase.h"
#include "inet/common/IProtocolRegistrationListener.h"

#include "inet/applications/common/SocketTag_m.h"

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
#include "inet/networklayer/contract/IArp.h"
#include "inet/networklayer/ipv4/Icmp.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/contract/INetworkProtocol.h"
#include "inet/networklayer/ipv4/Ipv4FragBuf.h"
#include "inet/networklayer/common/L3Address.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

namespace inet {

/**
 * Simple token bucket meter.
 */
class INET_API TokenBucketMeter : public cSimpleModule
{
  protected:

    cQueue credit_queue;

    double CIR = NaN;    // Commited Information Rate (bits/sec)
    long CBS = 0;    // Committed Burst Size (in bits)
    long redCBSmax = 0;    // ecn thresholds(in bits)
    long redCBSmin = 0;    // ecn thresholds(in bits)
    double redPmax = 0;    // ecn thresholds(in bits)
    bool colorAwareMode = false;
    bool ecnmarking = false;
    bool ecndrop = false;
    double Pecn = 0;
    int dropped_inloop = 0;
    int sum_inloop = 0;
    int max_inloop = 10;

    int kmin = 10;
    int kmax = 20;
    double pmax = 0.2;

    long Tc = 0;    // token bucket for committed burst
    long ecnTcmax = 0;
    long ecnTcmin = 0;
    simtime_t lastUpdateTime;

    int numRcvd = 0;
    int numRed = 0;


    bool isfirstpck;

  public:
      TokenBucketMeter() {}

    protected:
      virtual int numInitStages() const override { return NUM_INIT_STAGES; }
      virtual void initialize(int stage) override;
      virtual void handleMessage(cMessage *msg) override;
      virtual void refreshDisplay() const override;

      virtual int meterPacket(cPacket *packet);
};

} // namespace inet

#endif // ifndef __INET_TOKENBUCKETMETER_H

