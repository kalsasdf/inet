//
// Copyright (C) 2012 Andras Varga
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

#include "inet/angrydudu/PriorityQueue/PrrScheduler.h"
#include "inet/common/INETUtils.h"

namespace inet {

Define_Module(PrrScheduler);


simsignal_t PrrScheduler::BufferOccupancySignal =
        registerSignal("BufferOccupancy");

PrrScheduler::~PrrScheduler()
{

}

void PrrScheduler::initialize()
{
    SchedulerBase::initialize();

    enablePfc = par("enablePfc");
    enableSuspend = par("enableSuspend");

    shareBufferEcn = par("shareBufferEcn");
    EcnKmax = par("EcnKmax");
    EcnKmin = par("EcnKmin");
    EcnPmax = par("EcnPmax");
    Suspend_max = par("Suspend");
    Suspend_min = par("Restart");
    numInputs = gateSize("in");
    sharedBufferOccupancy = 0;
    for (int i = 0; i < numInputs; ++i) {
        ispaused[i] = false;
        pausesent[i] = false;
    }
    ASSERT(numInputs == (int)inputQueues.size());
}

bool PrrScheduler::schedulePacket()
{
    emit(BufferOccupancySignal,(unsigned long)sharedBufferOccupancy);
    for (int i = 0; i < numInputs; ++i) {
        if (ispaused.find(i)->second) // is paused
        {
            continue;
        }
        else{
            if (!inputQueues[i]->isEmpty()) {
                inputQueues[i]->requestPacket();
                //PassiveQueueBase::requestPacket();
                return true;
            }
        }
    }
    return false;
}

bool PrrScheduler::markEcn()
{
    if (!shareBufferEcn)
        return false;
    //return false; // if use cnp, disable this line.
    sharedBufferOccupancy = 0;
    for (int i = 0; i < numInputs; ++i) {
        sharedBufferOccupancy += inputQueues[i]->getQueueByteLength();
    }
    if (sharedBufferOccupancy > EcnKmax)
    {
        return true;
    }
    else if (sharedBufferOccupancy<=EcnKmax && sharedBufferOccupancy>EcnKmin)
    {
        if ( uniform(0,1) <= EcnPmax * double(sharedBufferOccupancy - EcnKmin)/double(EcnKmax - EcnKmin))
        {
            return true;
        }
        else // randomly no marking
        {
            return false;
        }
    }
    else // queue is short, no need to mark
    {
        return false;
    }
    return false;
}

int PrrScheduler::sendSuspend()
{
    if (!enableSuspend)
    {
        return 0;
    }
    sharedBufferOccupancy = 0;
    for (int i = 0; i < numInputs; ++i) {
        sharedBufferOccupancy += inputQueues[i]->getQueueByteLength();
    }
    EV<<"sendSuspend(): sharedBufferOccupancy = "<<sharedBufferOccupancy<<endl;
    if (sharedBufferOccupancy > Suspend_max)
    {
        return 1;
    }
    if (sharedBufferOccupancy < Suspend_min)
    {
        return -1;
    }
    return 0;
}

// never use
int PrrScheduler::sendPause(int Qn)
{
    if (!enablePfc)
    {
        return 0;
    }
    uint32_t queueLength = inputQueues[Qn]->getQueueByteLength();
    EV<<"queueLength = "<<queueLength<<", pausesent = "<<pausesent.find(Qn)->second<<
            ", Qn = "<<Qn<<endl;
    if (queueLength > Pause_max && !pausesent.find(Qn)->second)
    {
        pausesent[Qn] = true;
        return 1;
    }
    if (queueLength < Pause_min && pausesent.find(Qn)->second)
    {
        pausesent[Qn] = false;
        return -1;
    }
    return 0;
}

bool PrrScheduler::PausingQi(int Qi,bool pause)
{
    ispaused[Qi] = pause; // if pause == true, then the Qi is paused.
    return true;
}
} // namespace inet

