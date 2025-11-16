//
// Created by tadeas on 2025-10-16.
//

#ifndef SERVER_H
#define SERVER_H

#include <fstream>

#include "Channel.h"
#include "Cipher.h"
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
        Cipher::Cipher cipherer_;

        void listen();

        static void forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId, const std::vector<uint8_t> &iv);

    };

    class Receiver {
    public:
        Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId, const std::vector<uint8_t> &iv);

        void run();

    private:
        const int family_;
        ReceiverFSM state_;
        Channel::Channel commsChannel_;
        const pid_t commsId_;
        std::ofstream outputFile_;
        uint8_t icmpType_;
        Cipher::Cipher cipherer_;
        std::vector<uint8_t> iv_;

    };
} // Server

#endif //SERVER_H