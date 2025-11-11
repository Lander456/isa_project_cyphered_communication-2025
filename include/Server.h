//
// Created by tadeas on 2025-10-16.
//

#ifndef SERVER_H
#define SERVER_H

#include "Channel.h"
#include "ServerFSM.h"

namespace Server {
    class Greeter {
    public:
        explicit Greeter(int family);

        void run();

    private:
        GreeterFSM state_;
        Channel::Channel commsChannel_;
        const int family_;

        Packet::IcmpPacket listen();

        pid_t forkReceiver(int family, sockaddr_storage clientAddr);

    };

    class Receiver {
    public:
        explicit Receiver(int family);

        void run();

    private:
        const int family_;
        ReceiverFSM state_;

    };
} // Server

#endif //SERVER_H