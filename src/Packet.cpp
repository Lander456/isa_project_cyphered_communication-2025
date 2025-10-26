//
// Created by tadeas on 2025-10-21.
//

#include "../include/Packet.h"

#include <cstdlib>
#include <cstring>

namespace Packet {

    uint16_t IcmpPacket::calculateChecksum(const void* data, size_t length) {
        const auto* dataPtr = static_cast<const uint16_t*>(data);
        uint32_t checksum = 0;

        while (length > 1) {
            checksum += *dataPtr++;
            length -= 2;
        }

        if (length == 1) {
            uint16_t oddByte = 0;
            *reinterpret_cast<uint8_t*>(&oddByte) = *reinterpret_cast<const uint8_t*>(dataPtr);
            checksum += oddByte;
        }

        while (checksum >> 16) {
            checksum = (checksum & 0xFFFF) + (checksum >> 16);
        }

        return static_cast<uint16_t>(checksum);
    }

    IcmpPacket IcmpPacket::parse(const uint8_t *data, size_t length) {

        size_t offset = 0;
        const auto* header = reinterpret_cast<const struct IcmpHeader*>(data);
    }

    std::vector<uint8_t> IcmpPacket::serialize() const {
        std::vector<uint8_t> buffer(sizeof(header) + sizeof(protocol) + data.size());

        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &header, sizeof(header));
        offset += sizeof(header);

        std::memcpy(buffer.data() + offset, &protocol, sizeof(protocol));
        offset += sizeof(protocol);

        if (!data.empty()) {
            std::memcpy(buffer.data() + offset, data.data(), data.size());
        }

        return buffer;
    }

    void IcmpPacket::getChecksumIPv4() {
        header.checksum = 0;

        std::vector<uint8_t> buffer(sizeof(header) + sizeof(protocol) + data.size());
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &header, sizeof(header));
        offset += sizeof(header);

        std::memcpy(buffer.data() + offset, &protocol, sizeof(protocol));
        offset += sizeof(protocol);

        if (!data.empty()) {
            std::memcpy(buffer.data() + offset, data.data(), data.size());
        }

        uint16_t checksum = calculateChecksum(buffer.data(), buffer.size());

        header.checksum = checksum;
    }

    void IcmpPacket::getChecksumIPv6(const in6_addr &source, in6_addr &destination) {
        header.checksum = 0;

        std::vector<uint8_t> icmpData(sizeof(header) + sizeof(protocol) + data.size());
        size_t offset = 0;

        std::memcpy(icmpData.data() + offset, &header, sizeof(header));
        offset += sizeof(header);

        std::memcpy(icmpData.data() + offset, &protocol, sizeof(protocol));
        offset += sizeof(protocol);

        if (!data.empty()) {
            std::memcpy(icmpData.data() + offset, data.data(), data.size());
        }

        PseudoHeader pseudo {};

        pseudo.source = source;
        pseudo.destination = destination;
        pseudo.length = htonl(data.size());
        pseudo.nextHeader = IPPROTO_ICMPV6;

        std::vector<uint8_t> buffer(sizeof(pseudo) + icmpData.size());
        std::memcpy(buffer.data(), &pseudo, sizeof(pseudo));
        std::memcpy(buffer.data() + sizeof(pseudo), icmpData.data(), icmpData.size());

        uint16_t checksum = calculateChecksum(buffer.data(), buffer.size());

        header.checksum = checksum;
    }
} //Packet