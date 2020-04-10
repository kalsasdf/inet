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

#include "inet/angrydudu/xpass/xpassqueue/RandomMeter.h"
#include "inet/networklayer/diffserv/DiffservUtil.h"
#include "inet/common/ModuleAccess.h"
#include <iostream>

namespace inet {

using namespace DiffservUtil;

Define_Module(RandomMeter);

void RandomMeter::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numRcvd = 0;
        numRed = 0;
        credit_cached = 0;
        WATCH(numRcvd);
        WATCH(numRed);
        WATCH(credit_cached);

        CBS = 8 * par("cbs").intValue();
        colorAwareMode = par("colorAwareMode");
        ecnmarking = par("creditsECN");
        Tc = CBS;
        redCBSmax = 8 * par("Kmax").intValue();
        redCBSmin = 8 * par("Kmin").intValue();
        redPmax = par("Pmax");
        ecndrop = par("dropbyECN");
        usebuffer = par("usebuffer");

        isfirstpck = true;

        creditcapacity = par("creditcapacity");
        max_inloop = par("max_inloop");

    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        const char *cirStr = par("cir");
        IInterfaceTable *ift = findModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        CIR = parseInformationRate(cirStr, "cir", ift, *this, 0);
        lastUpdateTime = simTime();
    }
}
void RandomMeter::handleMessage(cMessage *msg)
{
    numRcvd++;
    cPacket * packet = check_and_cast<cPacket *>(msg);
    int color = meterPacket(packet);
    if (color == GREEN)
    {
        EV<<"RandomMeter send out the packet!"<<endl;
    }
}

void RandomMeter::refreshDisplay() const
{
    char buf[50] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "rcvd: %d ", numRcvd);
    if (numRed > 0)
        sprintf(buf + strlen(buf), "red:%d ", numRed);
    getDisplayString().setTagArg("t", 0, buf);
}

int RandomMeter::meterPacket(cPacket *pck)
{
    // update token buckets
    sum_inloop++ ;

    simtime_t currentTime = simTime();
    long numTokens = (long)(SIMTIME_DBL(currentTime - lastUpdateTime) * CIR);
    if (numTokens<0)
        numTokens = 100000;
    EV<<"00000000 numTokens = "<< numTokens<<endl;
    lastUpdateTime = currentTime;
    if (Tc + numTokens <= CBS)
    {
        Tc += numTokens;
    }
    else
    {
        Tc = CBS;
    }

    int oldColor = colorAwareMode ? getColor(pck) : -1;
    int newColor = -1;
    if (pck->getKind() == 1)
    {
        oldColor = RED;
        EV<<"set credit to RED based on the packet kind marked by the Encap"<<endl;
    }
    int packetSizeInBits = pck->getBitLength();
    packetSizeInBits = packetSizeInBits + 64; //translate to phy length
    if (oldColor <= GREEN && Tc - packetSizeInBits >= 0) {
        Tc -= packetSizeInBits;
        cPacket *packet = check_and_cast<cPacket *>(pck);
        if (ecnmarking)
        {
            //srand((unsigned)(packet->getId()));
            double temp_ratio = uniform(0,1);
            if (temp_ratio < Pecn)
            {
                Packet *tpacket = check_and_cast<Packet*>(packet);
                auto ethHeader = tpacket->removeAtFront<EthernetMacHeader>();
                const auto& old = tpacket->removeAtFront<Ipv4Header>();
                const auto& ipv4header = makeShared<Ipv4Header>();

                ipv4header->setExplicitCongestionNotification(3);
                ipv4header->setVersion(old->getVersion());
                ipv4header->setHeaderLength(old->getHeaderLength());
                ipv4header->setSrcAddress(old->getSrcAddress());
                ipv4header->setDestAddress(old->getDestAddress());
                ipv4header->setCrc(old->getCrc());
                ipv4header->setCrcMode(old->getCrcMode());
                ipv4header->setProtocolId(old->getProtocolId());
                ipv4header->setTimeToLive(old->getTimeToLive());
                ipv4header->setIdentification(old->getIdentification());
                ipv4header->setMoreFragments(old->getMoreFragments());
                ipv4header->setDontFragment(old->getDontFragment());
                ipv4header->setFragmentOffset(old->getFragmentOffset());
                ipv4header->setTotalLengthField(old->getTotalLengthField());
                ipv4header->setDiffServCodePoint(old->getDiffServCodePoint());
                ipv4header->setOptions(old->getOptions());
                tpacket->insertAtFront(ipv4header);
                tpacket->insertAtFront(ethHeader);
                auto eHeader = tpacket->removeAtFront<EthernetMacHeader>();
                const auto& ipold = tpacket->peekAtFront<Ipv4Header>();
                EV<<"meterPackets() randomly, ecn = "<< ipold->getExplicitCongestionNotification() <<", temp ratio = "<<temp_ratio<<endl;
                tpacket->insertAtFront(eHeader);
                packet = check_and_cast<cPacket*>(tpacket);
            }
        }
        newColor = GREEN;
        setColor(packet, newColor);
        send(packet,"greenOut");
    }
    else
    {
        numRed++;
        EV<<"meterPackets() dropping, Tc = "<< Tc <<", packetSizeInBits = "<<packetSizeInBits<<", oldColor = "<<oldColor<<endl;
        setColor(pck, RED);
        send(pck,"redOut");
        dropped_inloop++;
    }



    if (sum_inloop>max_inloop)
    {
        Pecn = double(dropped_inloop)/double(sum_inloop);
        sum_inloop = 0;
        dropped_inloop = 0;
        EV<<"Pecn = "<< Pecn<<endl;
    }

    return newColor;
}

} // namespace inet

