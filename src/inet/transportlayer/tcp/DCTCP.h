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

#ifndef __INET_DCTCP_H
#define __INET_DCTCP_H

#include "inet/common/INETDefs.h"

#include "inet/transportlayer/tcp/flavours/TcpTahoeRenoFamily.h"

namespace inet {

namespace tcp {

/**
 * State variables for DCTCP.
 */
typedef TcpTahoeRenoFamilyStateVariables DCTCPStateVariables;

/**
 * Implements DCTCP.
 */
class INET_API DCTCP : public TcpTahoeRenoFamily
{
  protected:
    DCTCPStateVariables *& state;    // alias to TCLAlgorithm's 'state'

    /** Create and return a DCTCPStateVariables object. */
    virtual TcpStateVariables *createStateVariables() override
    {
        return new DCTCPStateVariables();
    }

    /** Utility function to recalculate ssthresh */
    virtual void recalculateSlowStartThreshold();

    /** hs Calculate the value of F based on ECN to cut the window*/
    virtual void calcuF();

    virtual void updateandcutwnd();

    /** Redefine what should happen on retransmission */
    virtual void processRexmitTimer(TcpEventCode& event) override;

  public:
    /** Ctor */
    DCTCP();

    simtime_t last_control_time = 0;

    int sum_ack = 0;
    int sum_ecn = 0;
    double ecn_a;
    double ecn_g;
    double ecn_f;

    /** Redefine what should happen when data got acked, to add congestion window management */
    virtual void receivedDataAck(uint32 firstSeqAcked, bool ecn) override;

    /** Redefine what should happen when dupAck was received, to add congestion window management */
    virtual void receivedDuplicateAck() override;
};

} // namespace tcp

} // namespace inet

#endif // ifndef __INET_DCTCP_H

