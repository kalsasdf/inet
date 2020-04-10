/*
 * Util.h
 *
 *  Created on: Jan 27, 2016
 *      Author: behnamm
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <omnetpp.h>
#include "inet/homa/common/Minimal.h"
#include "inet/homa/transport/HomaTransport.h"
#include "inet/homa/transport/HomaPkt.h"

using namespace std;
using namespace omnetpp;

/**
 * Following filters are defined for collecting statistics from packets of type
 * HomaPkt. They are used to collect stats at the HomaTransport instances.
 */
namespace inet{

class SIM_API HomaMsgSizeFilter : public cObjectResultFilter
{
  PUBLIC:
    HomaMsgSizeFilter() {}
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object);
};

class SIM_API HomaPacketBytesFilter : public cObjectResultFilter
{
  PUBLIC:
    HomaPacketBytesFilter() {}
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object);
};

class SIM_API HomaUnschedPktBytesFilter : public cObjectResultFilter
{
  PUBLIC:
    HomaUnschedPktBytesFilter() {}
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object);
};

class SIM_API HomaGrantPktBytesFilter : public cObjectResultFilter
{
  PUBLIC:
    HomaGrantPktBytesFilter() {}
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object);
};

}

#endif /* UTIL_H_ */
