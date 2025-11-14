//
// Created by tadeas on 2025-10-16.
//

#ifndef SERVER_H
#define SERVER_H

#include <fstream>

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
        const pid_t pid_;

        void listen();

        static void forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId);

    };

    class Receiver {
    public:
        Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId);

        void run();

    private:
        const int family_;
        ReceiverFSM state_;
        Channel::Channel commsChannel_;
        const pid_t commsId_;
        std::ofstream outputFile_;

        bool awaitTransmissionWindow();

    };
} // Server

#endif //SERVER_H