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

#include "Lagger.h"
//#include "inet/linklayer/ethernet/Ethernet.h"
#include "inet/linklayer/ethernet/EtherMacBase.h"

namespace inet{

Define_Module(Lagger);

Lagger::Lagger()
{
    hookType = "";
    delayPar = NULL;
    mac = NULL;
}
void
Lagger::initialize()
{
    delayPar = &par("delayPar");
    hookType = par("hookType").stdstringValue();

    // get pointer to the mac module of this Interface
    //cModule* parentEthIface = getParentModule();
    //mac = parentEthIface->getSubmodule("eth");
    //if (!parentEthIface->par("duplexMode")) {
    //    throw cRuntimeError("Lagger module must be used in a EthernetInterface with"
    //            " a FullDuplexMac.");
    //}
}

void
Lagger::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        send(msg, "out");
    } else {
        if (hookType == "InputHook") {
            inputHookPktHandler(msg);
        } else if (hookType == "OutputHook") {
            outputHookPktHandler(msg);
        } else {
            throw cRuntimeError("Lagger type '%s' is not recognized.",
                    hookType.c_str());
        }
    }
}

void
Lagger::inputHookPktHandler(cMessage* msg)
{
    simtime_t delay = delayPar->doubleValue();
    if (mac->par("isReceiveOnStart").boolValue()){

        // Assuming the link connected to the mac layer has equal tx
        // and rx rates.
        double rxLinkRate = dynamic_cast<EtherMacBase*>(mac)->getTxRate();
        if (!msg->hasPar("rxLinkRate")) {
            msg->addPar("rxLinkRate");
        }
        msg->par("rxLinkRate").setDoubleValue(rxLinkRate);
    }
    scheduleAt(simTime() + delay, msg);
}

void
Lagger::outputHookPktHandler(cMessage* msg)
{
    simtime_t delay = delayPar->doubleValue();
    if (mac->par("isReceiveOnStart").boolValue() && msg->hasPar("rxLinkRate") &&
            msg->par("rxLinkRate").hasChanged()){
        double pktRxLinkRate = msg->par("rxLinkRate").doubleValue();
        double txLinkRate = dynamic_cast<EtherMacBase*>(mac)->getTxRate();
        if (pktRxLinkRate < txLinkRate) {
            int64_t frameByteLen = PK(msg)->getByteLength() +
                (4+14) + 7 + 1 +
                (96 >> 3);
            delay += ((8.0 * frameByteLen) / pktRxLinkRate);
        }
    }
    scheduleAt(simTime() + delay, msg);
}

}
