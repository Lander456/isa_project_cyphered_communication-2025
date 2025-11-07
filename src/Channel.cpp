//
// Created by tadeas on 2025-10-26.
//

#include <stdexcept>
#include "../include/Channel.h"

#include "../include/Packet.h"

namespace Channel {
    Channel::Channel(const int family)
    : family_(family), remoteAddress_({}) {
        if (family_ == AF_INET) {
            sockfd_ = socket(family_, SOCK_RAW, IPPROTO_ICMP);
        } else {
            sockfd_ = socket(family_, SOCK_DGRAM, IPPROTO_ICMPV6);
        }
        if (sockfd_ < 0) {
            throw std::runtime_error("Failed to create a socket");
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
                } else {
                    return {};
                }
            }

            auto packet = Packet::IcmpPacket::parse(buffer, received);

            if (packet.header.id == expectedId) {
                return packet;
            }
        }
    }
} // Channel