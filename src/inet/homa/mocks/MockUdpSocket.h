#include <vector>
#include <unordered_map>
#include "inet/homa/common/Minimal.h"
#include <omnetpp.h>
#include "inet/networklayer/common/L3Address.h"
#include "inet/homa/transport/HomaPkt.h"
#include "inet/common/packet/Packet.h"

namespace inet{

class MockUdpSocket {
  PUBLIC:
    std::vector<Packet*> sxPkts;

  PUBLIC:
    MockUdpSocket();
    ~MockUdpSocket();
    void setOutputGate(cGate* gate);
    void bind(uint32_t portNum);
    void sendTo(Packet* pkt, L3Address dest, int destPort);
    HomaPkt* getGrantPkt(L3Address dest, uint64_t mesgId);
};

}
