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
        const pid_t pid_;

        void listen();

        static void forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId, uint16_t commsSequence);

    };

    class Receiver {
    public:
        Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId, uint16_t commsSequence);

        void run();

    private:
        const int family_;
        const pid_t pid_;
        ReceiverFSM state_;
        Channel::Channel commsChannel_;
        const pid_t commsId_;
        std::vector<uint32_t> dataSequence_;
        uint16_t commsSequence_;
        std::vector<uint16_t> ackSequences_;
        uint32_t expectedSequenceNum_ = 0;

        bool sendAck(uint16_t sequenceNum);

    };
} // Server

#endif //SERVER_H