//
// Created by tadeas on 2025-10-26.
//

#include <stdexcept>
#include "../include/Channel.h"

namespace Channel {
    Channel::Channel(const int family, const int protocol)
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

    ssize_t Channel::sendPacket(const void *data, size_t len, const sockaddr *destination, const socklen_t destinationLength) const {
        return sendto(sockfd_, data, len, 0, destination, destinationLength);
    }

    ssize_t Channel::receivePacket(void *buffer, size_t len, sockaddr *source, socklen_t *sourceLength) const {
        return recvfrom(sockfd_, buffer, len, 0, source, sourceLength);
    }
} // Channel