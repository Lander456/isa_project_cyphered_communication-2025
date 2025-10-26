//
// Created by tadeas on 2025-10-26.
//

#ifndef PROJEKT_CHANNEL_H
#define PROJEKT_CHANNEL_H

#include <unistd.h>
#include <netdb.h>

namespace Channel {
    class Channel {
    private:
        int sockfd_{-1};
        int family_;
    public:
        Channel(int family, int protocol);
        ~Channel();

        ssize_t sendPacket(const void* data, size_t len, const sockaddr* destination, socklen_t detinationLength);
        ssize_t receivePacket(void* buffer, size_t len, sockaddr* source, socklen_t* sourceLength);

        [[nodiscard]] int family() const { return family_; }
    };
} // Channel

#endif //PROJEKT_CHANNEL_H