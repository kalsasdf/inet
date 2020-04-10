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

#include "inet/networklayer/diffserv/TokenBucketMeter.h"
#include "inet/networklayer/diffserv/DiffservUtil.h"
#include "inet/common/ModuleAccess.h"
#include <iostream>

namespace inet {

using namespace DiffservUtil;

Define_Module(TokenBucketMeter);

void TokenBucketMeter::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numRcvd = 0;
        numRed = 0;
        WATCH(numRcvd);
        WATCH(numRed);

        CBS = 8 * par("cbs").intValue();
        colorAwareMode = par("colorAwareMode");
        ecnmarking = par("creditsECN");
        Tc = CBS;
        redCBSmax = 8 * par("Kmax").intValue();
        redCBSmin = 8 * par("Kmin").intValue();
        redPmax = par("Pmax");
        ecndrop = par("dropbyECN");

        isfirstpck = true;

        max_inloop = 10;

    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        const char *cirStr = par("cir");
        IInterfaceTable *ift = findModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        CIR = parseInformationRate(cirStr, "cir", ift, *this, 0);
        lastUpdateTime = simTime();
    }
}
void TokenBucketMeter::handleMessage(cMessage *msg)
{
    cPacket *packet = check_and_cast<cPacket *>(msg);

    numRcvd++;
    int color = meterPacket(packet);
    if (color == GREEN) {
        send(packet, "greenOut");
    }
    else {
        numRed++;
        send(packet, "redOut");
    }
}

void TokenBucketMeter::refreshDisplay() const
{
    char buf[50] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "rcvd: %d ", numRcvd);
    if (numRed > 0)
        sprintf(buf + strlen(buf), "red:%d ", numRed);
    getDisplayString().setTagArg("t", 0, buf);
}

int TokenBucketMeter::meterPacket(cPacket *packet)
{
    // update token buckets
    simtime_t currentTime = simTime();
    long numTokens = (long)(SIMTIME_DBL(currentTime - lastUpdateTime) * CIR);
    lastUpdateTime = currentTime;
    if (Tc + numTokens <= CBS)
    {
        Tc += numTokens;
    }
    else
    {
        Tc = CBS;
    }

    // update meter state
    int oldColor = colorAwareMode ? getColor(packet) : -1;
    int newColor;
    int packetSizeInBits = packet->getBitLength();
    if (oldColor <= GREEN && Tc - packetSizeInBits >= 0) {
        Tc -= packetSizeInBits;
        if (ecnmarking)
        {
            if (Tc<=packetSizeInBits)
            {
                srand((unsigned)(packet->getId()));
                double temp_ratio = (double)(rand()%99)/(double)99;
                if (temp_ratio <= Pecn)
                {
                    Packet *pck = check_and_cast<Packet*>(packet);
                    auto ethHeader = pck->removeAtFront<EthernetMacHeader>();
                    const auto& old = pck->removeAtFront<Ipv4Header>();
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
                    pck->insertAtFront(ipv4header);
                    pck->insertAtFront(ethHeader);
                    auto eHeader = pck->removeAtFront<EthernetMacHeader>();
                    const auto& ipold = pck->peekAtFront<Ipv4Header>();
                    EV<<"meterPackets() randomly, ecn = "<< ipold->getExplicitCongestionNotification() <<", temp ratio = "<<temp_ratio<<endl;
                    pck->insertAtFront(eHeader);
                    packet = check_and_cast<cPacket*>(pck);
                    if (ecndrop)
                    {
                        newColor = RED;
                        dropped_inloop--;
                    }
                    else
                    {
                        newColor = GREEN;
                    }
                }
                else
                {
                    newColor = GREEN;
                }
            }
            else
            {
                EV<<"Enough tokens, no need for ecn and drop"<<endl;
                newColor = GREEN;
            }
        }
        else
        {
            if (Tc <= CBS-redCBSmax)
            {
                newColor = RED;
                EV<<"meterPackets(), 100% drop packet！"<<endl;
            }
            else if (Tc<=CBS-redCBSmin)
            {
                if ((double)(rand()%10000)/(double)(10000.00*redPmax) <= double(CBS-redCBSmin-Tc)/double(redCBSmax - redCBSmin))
                {
                    newColor = RED;
                    EV<<"meterPackets(), <100% drop packet！"<<endl;
                }
                else
                {
                    newColor = GREEN;
                }
            }
            else
            {
                newColor = GREEN;
            }
        }
    }
    else
        newColor = RED;


    if (sum_inloop>max_inloop)
    {
        Pecn = double(dropped_inloop)/double(sum_inloop);
        sum_inloop = 0;
        dropped_inloop = 0;
        EV<<"Pecn = "<< Pecn<<endl;
    }
    else
    {
        if (newColor==RED)
        {
            dropped_inloop++ ;
        }
        sum_inloop++ ;
    }

    setColor(packet, newColor);
    return newColor;
}

} // namespace inet

