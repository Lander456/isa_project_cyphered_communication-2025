//
// Created by tadeas on 2025-10-26.
//

#include <stdexcept>
#include "../include/Channel.h"

#include <cstring>
#include <iostream>

#include "../include/Packet.h"

namespace Channel {
    Channel::Channel(const int family)
    : family_(family), remoteAddress_({}), remoteAddressLength_(sizeof(remoteAddress_)) {
        if (family_ == AF_INET) {
            sockfd_ = socket(family_, SOCK_RAW, IPPROTO_ICMP);
        } else {
            sockfd_ = socket(family_, SOCK_RAW, IPPROTO_ICMPV6);
        }
        if (sockfd_ < 0) {
            perror("sockfd");
            exit(1);
        }

        timeval tv{};

        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    Channel::~Channel() {
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
    }

    ssize_t Channel::sendPacket(const void *data, const size_t len, const sockaddr *destination) const {
        return sendto(sockfd_, data, len, 0, destination, remoteAddressLength_);
    }

    ssize_t Channel::receivePacket(void *buffer, const size_t len, sockaddr *source, socklen_t sourceLength) const {
        return recvfrom(sockfd_, buffer, len, 0, source, &sourceLength);
    }

    Packet::IcmpPacket Channel::waitForResponse(const uint16_t expectedId, sockaddr* source) {
        auto start = std::chrono::steady_clock::now();

        uint8_t buffer[1500];

        while (true) {
            auto now = std::chrono::steady_clock::now();
            int timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

            if (timePassed >= TIMEOUT_MS) {
                return {};
            }

            ssize_t received = recvfrom(sockfd_, buffer, 1500, 0, source, &remoteAddressLength_);
            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                return {};
            }

            auto packet = Packet::IcmpPacket::parse(buffer, received);

            if (htons(packet.icmpHeader.id) == expectedId) {

                Packet::IcmpPacket::swapByteOrder(packet);
                return packet;
            }
        }
    }

    bool Channel::verifyChecksum(const Packet::IcmpPacket& packet) {
        std::vector<uint8_t> buffer(sizeof(packet.icmpHeader) + sizeof(packet.protocol) + packet.data.size());
        std::memcpy(buffer.data(), &packet.icmpHeader, sizeof(packet.icmpHeader));
        buffer[2] = 0;
        buffer[3] = 0;
        std::memcpy(buffer.data() + sizeof(packet.icmpHeader), &packet.protocol, sizeof(packet.protocol));
        std::memcpy(buffer.data() + sizeof(packet.icmpHeader) + sizeof(packet.protocol), packet.data.data(), packet.data.size());

        return Packet::IcmpPacket::calculateChecksum(buffer.data(), buffer.size()) == htons(packet.icmpHeader.checksum);
    }

    Packet::IcmpPacket Channel::listen() {
        uint8_t buffer[1500];

        while(true) {
            const ssize_t received = recvfrom(sockfd_, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr *>(&remoteAddress_), &remoteAddressLength_);

            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("recv");
                exit(1);
            }

            Packet::IcmpPacket packet = Packet::IcmpPacket::parse(buffer, received);

            if (packet.icmpHeader.checksum != 0) {
                Packet::IcmpPacket::swapByteOrder(packet);
                return packet;
            }
        }
        return {};
    }

    bool Channel::transmissionHandover(const pid_t id) const {
        const std::vector<uint8_t> emptyData;
        const auto packet = Packet::IcmpPacket::createPacket(8, 0, id, PacketType::TRANSMISSION_HANDOVER, emptyData);
        const auto serializedPacket = packet.serialize();
        return sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr *>(&remoteAddress_));
    }
} // Channel