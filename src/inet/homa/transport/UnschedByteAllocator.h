/*
 * UnschedByteAllocator.h
 *
 *  Created on: Oct 15, 2015
 *      Author: behnamm
 */

#include <map>
#include <unordered_map>
#include "omnetpp.h"
#include "inet/homa/common/Minimal.h"
#include "inet/homa/transport/HomaPkt.h"
#include "inet/homa/transport/HomaConfigDepot.h"

#ifndef UNSCHEDBYTEALLOCATOR_H_
#define UNSCHEDBYTEALLOCATOR_H_
namespace inet{

class UnschedByteAllocator
{
  PUBLIC:
    explicit UnschedByteAllocator(HomaConfigDepot* homaConfig);
    ~UnschedByteAllocator();
    std::vector<uint16_t> getReqUnschedDataPkts(uint32_t, uint32_t msgSize);
    void updateReqDataBytes(HomaPkt* grantPkt);
    void updateUnschedBytes(HomaPkt* grantPkt);

  PRIVATE:
    void initReqBytes(uint32_t rxAddr);
    void initUnschedBytes(uint32_t rxAddr);
    uint32_t getReqDataBytes(uint32_t rxAddr, uint32_t msgSize);
    uint32_t getUnschedBytes(uint32_t rxAddr, uint32_t msgSize);

  PRIVATE:
    std::unordered_map<uint32_t, std::map<uint32_t, uint32_t>>
            rxAddrUnschedbyteMap;
    std::unordered_map<uint32_t, std::map<uint32_t, uint32_t>>
            rxAddrReqbyteMap;
    HomaConfigDepot* homaConfig;
    uint32_t maxReqPktDataBytes;
    uint32_t maxUnschedPktDataBytes;
};
}
#endif /* UNSCHEDBYTEALLOCATOR_H_ */
