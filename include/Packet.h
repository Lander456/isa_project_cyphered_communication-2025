//
// Created by tadeas on 2025-10-21.
//

#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <vector>
#include <netinet/in.h>

#pragma pack(push, 1)
namespace Packet {
    struct icmpHeader {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t id;
        uint16_t sequence;
    };

    struct protocolInfo {
        uint8_t type;
        uint16_t sequence;
        uint8_t flags;
        uint16_t length;
    };

    struct PseudoHeader {
        in6_addr source;
        in6_addr destination;
        uint32_t length;
        uint8_t zeros[3];
        uint8_t nextHeader;
    };
#pragma pack(pop)

    class IcmpPacket {
    public:
        icmpHeader header{};
        protocolInfo protocol{};
        std::vector<uint8_t> data;
        void getChecksumIPv4();
        void getChecksumIPv6(const in6_addr &source, in6_addr &destination);
        [[nodiscard]] std::vector<uint8_t> serialize() const;
        static IcmpPacket parse(const uint8_t* data, size_t length);

    private:
        static uint16_t calculateChecksum(const void* data, size_t length);
    };
} //Packet


#endif //PACKET_H