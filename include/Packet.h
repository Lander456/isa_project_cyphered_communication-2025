//
// Created by tadeas on 2025-10-21.
//

#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <netinet/in.h>

enum class PacketType : uint8_t {
    HELLO = 200,
    ACK = 201,
    DATA = 202,
    RECEIVING = 203,
    GOODBYE = 204
};

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
        PacketType type;
        uint16_t payloadLength;
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

        static IcmpPacket createPacket(uint8_t icmpType, uint8_t code, uint16_t id, uint16_t icmpSequence, PacketType protocolType, const std::vector<uint8_t> &data);

    private:
        static uint16_t calculateChecksum(const void* data, size_t length);
    };
} //Packet
#endif //PACKET_H