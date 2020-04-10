//
// Copyright (C) 2005 Andras Varga
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

#include "inet/angrydudu/nic/throughputMeter/ThroughputMeter.h"

namespace inet {

Define_Module(ThroughputMeter);

void ThroughputMeter::initialize()
{
    startTime = par("startTime");
    endTime = par("endTime");
    long _batchSize = par("batchSize");
    if ((_batchSize < 0) || (((long)(unsigned int)_batchSize) != _batchSize))
        throw cRuntimeError("Invalid 'batchSize=%ld' parameter at '%s' module", _batchSize, getFullPath().c_str());
    batchSize = (unsigned int)_batchSize;
    maxInterval = par("maxInterval");

    numPackets = numBits = numappBits = 0;
    intvlStartTime = intvlLastPkTime = 0;
    intvlNumPackets = intvlNumBits = intvlappBits = intvlcdtBits = 0;

    WATCH(numappBits);
    WATCH(numPackets);
    WATCH(numBits);
    WATCH(intvlStartTime);
    WATCH(intvlNumPackets);
    WATCH(intvlNumBits);
    WATCH(intvlcdtBits);
    WATCH(intvlappBits);

    goodputVector.setName("goodput (bit/sec)");
    bitpersecVector.setName("thruput (bit/sec)");
    pkpersecVector.setName("packet/sec");
    cdtbitpersecVector.setName("credit (bit/sec)");
}

void ThroughputMeter::handleMessage(cMessage *msg)
{
    if (simTime()<=endTime&&simTime()>=startTime)
    {
        EV<<"receive packet and meter it."<<endl;
        updateStats(simTime(), PK(msg)->getBitLength(),check_and_cast<cPacket *>(msg));
    }
    send(msg, "out");
}

void ThroughputMeter::updateStats(simtime_t now, unsigned long bits, cPacket *pck)
{
    numPackets++;
    numBits += bits;

    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || now - intvlStartTime >= maxInterval)
        beginNewInterval(now);

    intvlNumPackets++;
    intvlNumBits += bits;
    intvlLastPkTime = now;
    if(std::string(pck->getFullName()).find("tcpseg") != std::string::npos || std::string(pck->getFullName()).find("Data") != std::string::npos
            || std::string(pck->getFullName()).find("Homa") != std::string::npos || std::string(pck->getFullName()).find("Msg") != std::string::npos)
    {
        if(startTime == 0)
            startTime = simTime();
        numappBits += bits;
        intvlappBits += bits;
    }
    if(std::string(pck->getFullName()).find("credit") != std::string::npos)
    {
        intvlcdtBits += bits;
    }

}

void ThroughputMeter::beginNewInterval(simtime_t now)
{
    simtime_t duration = now - intvlStartTime;

    // record measurements
    double bitpersec = intvlNumBits / duration.dbl();
    double pkpersec = intvlNumPackets / duration.dbl();
    double appbitpersec = intvlappBits / duration.dbl();
    double cdtbitpersec = intvlcdtBits / duration.dbl();

    bitpersecVector.recordWithTimestamp(intvlStartTime, bitpersec);
    pkpersecVector.recordWithTimestamp(intvlStartTime, pkpersec);
    goodputVector.recordWithTimestamp(intvlStartTime, appbitpersec);
    cdtbitpersecVector.recordWithTimestamp(intvlStartTime, cdtbitpersec);

    // restart counters
    intvlStartTime = now;    // FIXME this should be *beginning* of tx of this packet, not end!
    intvlNumPackets = intvlNumBits = intvlappBits = intvlcdtBits = 0;
}

void ThroughputMeter::finish()
{
    simtime_t duration;
    if (simTime()<endTime)
        duration = simTime() - startTime;
    else
        duration = endTime - startTime;

    recordScalar("duration", duration);
    recordScalar("total packets", numPackets);
    recordScalar("total bits", numBits);
    recordScalar("total app bits", numappBits);

    recordScalar("avg throughput (bit/s)", numBits / duration.dbl());
    recordScalar("avg packets/s", numPackets / duration.dbl());
    recordScalar("avg goodput (bit/s)", numappBits / duration.dbl());
}

} // namespace inet

