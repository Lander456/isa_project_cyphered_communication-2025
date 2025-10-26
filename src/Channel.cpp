//
// Created by tadeas on 2025-10-26.
//

#include "../include/Channel.h"

#include <stdexcept>

namespace Channel {
    Channel::Channel(int family, int protocol)
    : family_(family) {
        sockfd_ = socket(family, SOCK_RAW, protocol);
        if (sockfd_ < 0) {
            throw std::runtime_error("Failed to create a socket");
        }
    }

    Channel::~Channel() {
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
    }

    ssize_t Channel::sendPacket(const void *data, size_t len, const sockaddr *destination, socklen_t detinationLength) {
        return sendto(sockfd_, data, len, 0, destination, detinationLength);
    }

    ssize_t Channel::receivePacket(void *buffer, size_t len, sockaddr *source, socklen_t *sourceLength) {
        return recvfrom(sockfd_, buffer, len, 0, source, sourceLength);
    }

} // Channel