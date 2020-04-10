//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/angrydudu/MCCM/MccmSwitch/MccmRelayUnit.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EtherMacBase.h"
#include "inet/linklayer/ethernet/Ethernet.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Protocol.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/lifecycle/NodeOperations.h"

namespace inet {

Define_Module(MccmRelayUnit);

void MccmRelayUnit::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        numProcessedFrames = numDiscardedFrames = 0;

        addressTable = getModuleFromPar<IMacAddressTable>(par("macTableModule"), this);
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        queue_fulltime.clear();
        linkspeed = par("linkspeed");
        nextif = 0;
        empty_time = double(10^6)/double(linkspeed);

        WATCH(numProcessedFrames);
        WATCH(numDiscardedFrames);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        registerService(Protocol::ethernetMac, nullptr, gate("ifIn"));
        registerProtocol(Protocol::ethernetMac, gate("ifOut"), nullptr);
    }
}

void MccmRelayUnit::handleMessage(cMessage *msg)
{
    if (!isOperational) {
        EV << "Message '" << msg << "' arrived when module status is down, dropped it\n";
        delete msg;
        return;
    }
    Packet *packet = check_and_cast<Packet *>(msg);
    if (!strcmp(packet->getName(),"Inform"))
    {
        queue_fulltime[packet->getTag<InterfaceInd>()->getInterfaceId()] = simTime();
    }
    else
    {
        const auto& frame = packet->peekAtFront<EthernetMacHeader>();
        // Frame received from MAC unit
        emit(packetReceivedFromLowerSignal, packet);
        handleAndDispatchFrame(packet, frame);
    }
}

void MccmRelayUnit::handleAndDispatchFrame(Packet *packet, const Ptr<const EthernetMacHeader>& frame)
{
    //FIXME : should handle multicast mac addresses correctly

    int inputInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();

    numProcessedFrames++;

    // update address table
    addressTable->updateTableWithAddress(inputInterfaceId, frame->getSrc());

    // handle broadcast frames first
    if (frame->getDest().isBroadcast()) {
        EV << "Broadcasting broadcast frame " << frame << endl;
        broadcastFrame(packet, inputInterfaceId);
        return;
    }

    // Finds output port of destination address and sends to output port
    // if not found then broadcasts to all other ports instead
    int outputInterfaceId = addressTable->getPortForAddress(frame->getDest());
    // should not send out the same frame on the same ethernet port
    // (although wireless ports are ok to receive the same message)
    if (inputInterfaceId == outputInterfaceId) {
        EV << "Output port is same as input port, " << packet->getFullName()
           << " dest " << frame->getDest() << ", discarding frame\n";
        numDiscardedFrames++;
        delete packet;
        return;
    }

    if (outputInterfaceId >= 0) {
        nextif = outputInterfaceId;
        if (queue_fulltime.find(outputInterfaceId)->second - simTime() < empty_time)
        {
            find_nextif(outputInterfaceId); // find the most idle port;
            outputInterfaceId = nextif;
        }

        EV << "Sending frame " << frame << " with dest address " << frame->getDest() << " to port " << outputInterfaceId << endl;
        auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
        packet->clearTags();
        auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
        *newPacketProtocolTag = *oldPacketProtocolTag;
        delete oldPacketProtocolTag;
        packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(outputInterfaceId);
        packet->trim();
        emit(packetSentToLowerSignal, packet);
        send(packet, "ifOut");
    }
    else {
        EV << "Dest address " << frame->getDest() << " unknown, broadcasting frame " << packet << endl;
        broadcastFrame(packet, inputInterfaceId);
    }
}

void MccmRelayUnit::broadcastFrame(Packet *packet, int inputInterfaceId)
{
    auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
    packet->clearTags();
    auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
    *newPacketProtocolTag = *oldPacketProtocolTag;
    delete oldPacketProtocolTag;
    packet->trim();
    int numPorts = ift->getNumInterfaces();
    for (int i = 0; i < numPorts; ++i) {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->isLoopback() || !ie->isBroadcast())
            continue;
        int ifId = ie->getInterfaceId();
        if (inputInterfaceId != ifId) {
            Packet *dupFrame = packet->dup();
            dupFrame->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ifId);
            emit(packetSentToLowerSignal, dupFrame);
            send(dupFrame, "ifOut");
        }
    }
    delete packet;
}

void MccmRelayUnit::start()
{
    addressTable->clearTable();
    isOperational = true;
}

void MccmRelayUnit::stop()
{
    addressTable->clearTable();
    isOperational = false;
}

bool MccmRelayUnit::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();

    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if (static_cast<NodeStartOperation::Stage>(stage) == NodeStartOperation::STAGE_LINK_LAYER) {
            start();
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if (static_cast<NodeShutdownOperation::Stage>(stage) == NodeShutdownOperation::STAGE_LINK_LAYER) {
            stop();
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if (static_cast<NodeCrashOperation::Stage>(stage) == NodeCrashOperation::STAGE_CRASH) {
            stop();
        }
    }
    else {
        throw cRuntimeError("Unsupported operation '%s'", operation->getClassName());
    }

    return true;
}

void MccmRelayUnit::find_nextif(int regularif)
{
    simtime_t max_t= simTime() - queue_fulltime.find(regularif)->second;
    auto it = queue_fulltime.begin();
    for(; it != queue_fulltime.end(); ++ it)
    {
        EV <<"the last full time = " <<queue_fulltime.find(it->first)->second<<", the if id = "<<it->first<<endl;
        if (simTime() - queue_fulltime.find(it->first)->second > max_t)
        {
            max_t = simTime() - queue_fulltime.find(it->first)->second;
            nextif = it->first;
        }
    }
}

void MccmRelayUnit::finish()
{
    recordScalar("processed frames", numProcessedFrames);
    recordScalar("discarded frames", numDiscardedFrames);
}

} // namespace inet

