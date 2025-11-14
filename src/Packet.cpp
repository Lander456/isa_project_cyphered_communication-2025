//
// Created by tadeas on 2025-10-21.
//

#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "../include/Packet.h"

namespace Packet {

    uint16_t IcmpPacket::calculateChecksum(const void* data, size_t length) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        uint32_t checksum = 0;

        while (length > 1) {
            checksum += (bytes[0] << 8) | bytes[1];
            bytes += 2;
            length -= 2;
        }
        if (length == 1) {
            checksum += bytes[0] << 8;
        }
        while (checksum >> 16)
            checksum = (checksum & 0xFFFF) + (checksum >> 16);

        checksum = static_cast<uint16_t>(~checksum);
        return checksum;
    }

    void IcmpPacket::swapByteOrder(IcmpPacket &packet) {
        packet.ipHeader.ip_len = ntohs(packet.ipHeader.ip_len);
        packet.ipHeader.ip_id = ntohs(packet.ipHeader.ip_id);
        packet.ipHeader.ip_off = ntohs(packet.ipHeader.ip_off);
        packet.ipHeader.ip_sum = ntohs(packet.ipHeader.ip_sum);
        packet.ipHeader.ip_src.s_addr = ntohl(packet.ipHeader.ip_src.s_addr);
        packet.ipHeader.ip_dst.s_addr = ntohl(packet.ipHeader.ip_dst.s_addr);
        packet.icmpHeader.id = ntohs(packet.icmpHeader.id);
        packet.icmpHeader.checksum = ntohs(packet.icmpHeader.checksum);
        packet.icmpHeader.sequence = ntohs(packet.icmpHeader.sequence);
        packet.protocol.payloadLength = ntohs(packet.protocol.payloadLength);
    }

    IcmpPacket IcmpPacket::parse(const uint8_t *data, const size_t length) {
        IcmpPacket packet;

        if (length < (data[0] & 0x0F) * 4 + sizeof(packet.icmpHeader) + sizeof(packet.protocol)) {
            return {};
        }

        size_t offset = 0;
        size_t stop = (data[0] & 0x0F) * 4;
        std::memcpy(&packet.ipHeader, data, stop);

        offset += stop;
        stop = sizeof(packet.icmpHeader);
        std::memcpy(&packet.icmpHeader, data + offset, stop);

        offset += stop;

        stop = sizeof(packet.protocol);
        std::memcpy(&packet.protocol, data + offset, stop);
        offset += stop;

        size_t remaining = length - offset;
        if (remaining > 0) {
            packet.data.resize(remaining);
            std::memcpy(packet.data.data(), data + offset, remaining);
        }

        return packet;
    }

    std::vector<uint8_t> IcmpPacket::serialize() const {
        std::vector<uint8_t> buffer(sizeof(icmpHeader) + sizeof(protocol) + data.size());

        size_t offset = 0;

        std::memcpy(buffer.data() + offset, &icmpHeader, sizeof(icmpHeader));
        offset += sizeof(icmpHeader);

        std::memcpy(buffer.data() + offset, &protocol, sizeof(protocol));
        offset += sizeof(protocol);

        if (!data.empty()) {
            std::memcpy(buffer.data() + offset, data.data(), data.size());
        }

        return buffer;
    }

    void IcmpPacket::getChecksumIPv4() {
        icmpHeader.checksum = 0;

        std::vector<uint8_t> buffer(sizeof(icmpHeader) + sizeof(protocol) + data.size());
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &icmpHeader, sizeof(icmpHeader));
        offset += sizeof(icmpHeader);

        std::memcpy(buffer.data() + offset, &protocol, sizeof(protocol));
        offset += sizeof(protocol);

        if (!data.empty()) {
            std::memcpy(buffer.data() + offset, data.data(), data.size());
        }

        uint16_t checksum = calculateChecksum(buffer.data(), buffer.size());

        icmpHeader.checksum = checksum;
    }

    void IcmpPacket::getChecksumIPv6(const in6_addr &source, const in6_addr &destination) {
        icmpHeader.checksum = 0;

        std::vector<uint8_t> icmpData(sizeof(icmpHeader) + sizeof(protocol) + data.size());
        size_t offset = 0;

        std::memcpy(icmpData.data() + offset, &icmpHeader, sizeof(icmpHeader));
        offset += sizeof(icmpHeader);

        std::memcpy(icmpData.data() + offset, &protocol, sizeof(protocol));
        offset += sizeof(protocol);

        if (!data.empty()) {
            std::memcpy(icmpData.data() + offset, data.data(), data.size());
        }

        PseudoHeader pseudo {};

        pseudo.source = source;
        pseudo.destination = destination;
        pseudo.length = htonl(sizeof(icmpHeader) + sizeof(protocol) + data.size());
        pseudo.nextHeader = IPPROTO_ICMPV6;

        std::vector<uint8_t> buffer(sizeof(pseudo) + icmpData.size());
        std::memcpy(buffer.data(), &pseudo, sizeof(pseudo));
        std::memcpy(buffer.data() + sizeof(pseudo), icmpData.data(), icmpData.size());

        const uint16_t checksum = calculateChecksum(buffer.data(), buffer.size());

        icmpHeader.checksum = checksum;
    }

    IcmpPacket IcmpPacket::createPacket(const uint8_t icmpType, const uint8_t code, const uint16_t id, const PacketType protocolType, const std::vector<uint8_t> &data) {
        IcmpPacket packet;

        packet.icmpHeader.type = icmpType;
        packet.icmpHeader.code = code;
        packet.icmpHeader.id = htons(id);
        packet.icmpHeader.sequence = htons(0);
        packet.icmpHeader.checksum = 0;

        packet.protocol.type = protocolType;
        packet.protocol.payloadLength = htons(data.size());

        std::vector<uint8_t> checksumBuffer(sizeof(icmpHeader) + sizeof(protocol) + data.size());

        memcpy(checksumBuffer.data(), &packet.icmpHeader, sizeof(icmpHeader));
        memcpy(checksumBuffer.data() + sizeof(icmpHeader), &packet.protocol, sizeof(protocol));

        if (!data.empty()) {
            memcpy(checksumBuffer.data() + sizeof(icmpHeader) + sizeof(protocol), data.data(), data.size());
        }

        packet.icmpHeader.checksum = htons(calculateChecksum(checksumBuffer.data(), checksumBuffer.size()));

        packet.data = data;

        return packet;
    }
} //Packet