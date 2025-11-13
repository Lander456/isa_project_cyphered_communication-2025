//
// Created by tadeas on 2025-10-26.
//

#ifndef PROJEKT_CHANNEL_H
#define PROJEKT_CHANNEL_H

#include <unistd.h>
#include <netdb.h>
#include <chrono>

#include "Packet.h"

#define CHUNK_SIZE 1500
#define TIMEOUT_MS 1000

namespace Channel {
    class Channel {
    public:

        explicit Channel(int family);

        ~Channel();

        ssize_t sendPacket(const void* data, size_t len, const sockaddr* destination) const;

        ssize_t receivePacket(void* buffer, size_t len, sockaddr* source, socklen_t sourceLength) const;

        Packet::IcmpPacket waitForResponse(uint16_t expectedId, sockaddr* source);

        [[nodiscard]] int family() const { return family_; }

        void setRemoteAddress(const sockaddr_storage& address) {remoteAddress_ = address;}
        void setRemoteAddressLength(socklen_t remoteAddressLength) {remoteAddressLength_ = remoteAddressLength;}
        [[nodiscard]] socklen_t getRemoteAddressLength() const {return remoteAddressLength_;}
        [[nodiscard]] sockaddr_storage& getRemoteAddress() {return remoteAddress_;}

        Packet::IcmpPacket listen();

        bool transmissionHandover(pid_t id, uint16_t sequence) const;

        bool sendAck(pid_t id, uint16_t sequenceNum) const;

    private:
        int sockfd_{-1};
        int family_;
        sockaddr_storage remoteAddress_;
        socklen_t remoteAddressLength_;

        static bool verifyChecksum(const Packet::IcmpPacket& packet);
    };
} // Channel

#endif //PROJEKT_CHANNEL_H