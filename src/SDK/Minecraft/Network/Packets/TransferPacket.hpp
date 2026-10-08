#pragma once
//
// TransferPacket — server-to-client packet that redirects the client
// to a different server address and port.
//

#include "Packet.hpp"
#include <string>

class TransferPacket : public Packet {
public:
    static const PacketID ID = PacketID::Transfer;

    // Fields after Packet base (0x30)
    std::string mAddress; // Server address to transfer to
    uint16_t mPort;       // Server port to transfer to
};
