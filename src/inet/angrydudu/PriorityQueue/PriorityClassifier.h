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

#ifndef __INET_PRIORITYCLASSIFIER_H
#define __INET_PRIORITYCLASSIFIER_H

#include "inet/common/INETDefs.h"
#include "inet/common/packet/Packet.h"
#include <string.h>
using namespace std;

namespace inet {

/**
 * Behavior Aggregate Classifier.
 */
class INET_API PriorityClassifier : public cSimpleModule
{
  protected:
    int numOutGates = 0;
    std::map<uint32_t, int> priorityToGateIndexMap;

    int numRcvd = 0;

    static simsignal_t pkClassSignal;

  public:
    PriorityClassifier() {}

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;

    virtual int classifyPacket(Packet *packet);

    virtual uint32_t getPriority(Packet *packet);
};

} // namespace inet

#endif // ifndef __INET_PRIORITYCLASSIFIER_H

