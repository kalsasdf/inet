/*
 * Util.cc
 *
 *  Created on: Jan 27, 2016
 *      Author: behnamm
 */


#include "Util.h"

namespace inet{

class HomaTransport;

/*
Register_ResultFilter("homaMsgSize", HomaMsgSizeFilter);
Register_ResultFilter("HomaPacketBytes", HomaPacketBytesFilter);
Register_ResultFilter("homaUnschedPktBytes", HomaUnschedPktBytesFilter);
Register_ResultFilter("homaGrantPktBytes", HomaGrantPktBytesFilter);
*/
void
HomaMsgSizeFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t,
    cObject *object)
{
    HomaPkt* HomaPck = check_and_cast<HomaPkt*>(object);
    uint32_t msgSize;
    PktType pktType = (PktType)HomaPck->getPktType();
    switch (pktType) {
        case PktType::REQUEST:
        case PktType::UNSCHED_DATA:
            msgSize = HomaPck->getUnschedFields().msgByteLen;
            break;
        case PktType::SCHED_DATA:
        case PktType::GRANT: {
            HomaTransport::InboundMessage* inbndMsg;
            HomaTransport::OutboundMessage* outMsg;
            HomaTransport* transport = HomaPck->ownerTransport;
            if ((inbndMsg =
                    transport->rxScheduler.lookupInboundMesg(HomaPck))) {
                msgSize = inbndMsg->getMsgSize();
            } else {
                outMsg = &(transport->sxController.getOutboundMsgMap()->at(
                        HomaPck->getMsgId()));
                ASSERT((transport->getLocalAddr() == HomaPck->getDestAddr()
                    && pktType == PktType::GRANT) ||
                    (transport->getLocalAddr() == HomaPck->getSrcAddr()
                    && pktType == PktType::SCHED_DATA));
                msgSize = outMsg->getMsgSize();
            }
            break;
        }
        default:
            throw cRuntimeError("PktType %d is not known!", pktType);

    }
    fire(this,  t , (double)msgSize,object);
}

void
HomaPacketBytesFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object)
{
    HomaPkt* HomaPck = check_and_cast<HomaPkt*>(object);
    fire(this, t, (double)HomaPkt::getBytesOnWire(HomaPck->getDataBytes(),
        (PktType)HomaPck->getPktType()),object);
}

void
HomaUnschedPktBytesFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object)
{
    HomaPkt* Homapck = check_and_cast<HomaPkt*>(object);
    switch (Homapck->getPktType()) {
        case PktType::REQUEST:
        case PktType::UNSCHED_DATA:
            fire(this, t, (double)HomaPkt::getBytesOnWire(
                    Homapck->getDataBytes(), (PktType)Homapck->getPktType()),object);
            return;
        default:
            return;
    }

}

void
HomaGrantPktBytesFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t,
        cObject *object)
{
    HomaPkt* Homapck = check_and_cast<HomaPkt*>(object);
    switch (Homapck->getPktType()) {
        case PktType::GRANT:
            fire(this, t, (double)HomaPkt::getBytesOnWire(
                    Homapck->getDataBytes(), (PktType)Homapck->getPktType()),object);
            return;
        default:
            return;
    }
}

}

