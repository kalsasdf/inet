//
// Copyright (C) 2016 Andras Varga
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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.networklayer.common.L3Address;
import inet.homa.transport.HomaPkt;

namespace inet;

cplusplus {{
#include <vector>
#include "inet/networklayer/common/L3Address.h"
}}
//
// Generic application packet
//
// TODO: rename to ApplicationHeader and remove ByteCountChunk from ApplicationHeaderSerializer
class HomaHeader extends FieldsChunk
{
    inet::L3Address srcAddr;
    inet::L3Address destAddr;
    uint64_t msgId;
    int64_t BitLen;
    uint32_t priority = 1;
    uint32_t ECN;
    int pktType @enum(PktType);

    // depending on the pktType, only one of the three following struct are
    // relevant in the context of a HomaPkt.
    UnschedFields unschedFields;    // Used for both request and unsched
    GrantFields grantFields;            // Used only for grants
    SchedDataFields schedDataFields;    // Used only for scheduled data
}
