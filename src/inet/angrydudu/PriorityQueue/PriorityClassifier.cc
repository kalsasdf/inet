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

#include "inet/common/INETDefs.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"


#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv6/Ipv6Header.h"
#include "inet/linklayer/ieee8022/Ieee8022LlcHeader_m.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"

#include "inet/angrydudu/PriorityQueue/PriorityClassifier.h"
#include "inet/networklayer/diffserv/DiffservUtil.h"

namespace inet {

using namespace DiffservUtil;

Define_Module(PriorityClassifier);

simsignal_t PriorityClassifier::pkClassSignal = registerSignal("pkClass");

void PriorityClassifier::initialize()
{
    numOutGates = gateSize("outs");
    std::vector<int> priorities;
    const char *prios = par("priorities");
    cStringTokenizer tokens(prios);
    while (tokens.hasMoreTokens())
    {
        int temp_priority = atoi(tokens.nextToken());
        EV<<"the initialized priority = "<<temp_priority<<endl;
        priorities.push_back(temp_priority);
    }
    //parseDSCPs(par("priorities"), "priorities", priorities);
    int numPriorities = (int)priorities.size();
    if (numPriorities > numOutGates)
        throw cRuntimeError("%s priority values are given, but the module has only %d out gates",
                numPriorities, numOutGates);
    for (int i = 0; i < numPriorities; ++i)
    {
        priorityToGateIndexMap[priorities[i]] = i;
        EV_INFO<<"priorities[i] = "<<priorities[i]<<", i = "<<i<<endl;
    }

    numRcvd = 0;
    WATCH(numRcvd);
}

void PriorityClassifier::handleMessage(cMessage *msg)
{
    Packet *packet = check_and_cast<Packet *>(msg);
    EV<<"Scheduling priority of the received packet = "<<packet->getPriority()<<endl;
    numRcvd++;
    int clazz = classifyPacket(packet);
    emit(pkClassSignal, clazz);

    if (string(msg->getFullName()).find("Pause") != string::npos)
    {
        send(packet, "outs" , 1); // pass through the highest priority channel
    }
    else if (clazz >= 0)
        send(packet, "outs", clazz);
    else
        send(packet, "defaultOut");
}

void PriorityClassifier::refreshDisplay() const
{
    char buf[20] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "rcvd:%d ", numRcvd);
    getDisplayString().setTagArg("t", 0, buf);
}

int PriorityClassifier::classifyPacket(Packet *packet)
{
    uint32_t priority = getPriority(packet);
    if (priority >= 0) {
        auto it = priorityToGateIndexMap.find(priority);
        if (it != priorityToGateIndexMap.end())
        {
            return it->second;
        }
    }
    return -1;
}

// revised by Shan Huang.
uint32_t PriorityClassifier::getPriority(Packet *packet)
{
    //if (string(packet->getFullName()).find("Homa") != string::npos)
        return packet->getPriority();
    //return -1;
}

} // namespace inet

