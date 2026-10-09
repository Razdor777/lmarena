#pragma once
//
// CommandRequestPacket.hpp
//

#include <string>
#include "Packet.hpp"

struct CommandOriginData {
    int     mType    = 0;
    int64_t mPlayerId = 0;     // Runtime entity ID of the player
    std::string mRequestId;
    int64_t mPlayerEntityUniqueId = 0;
};

class CommandRequestPacket : public Packet {
public:
    static const PacketID ID = PacketID::CommandRequest;

    std::string      mCommand;          // this+0x30
    CommandOriginData mOrigin;          // this+0x50
    bool             mIsInternal = false;
    int              mVersion    = 52;
};
