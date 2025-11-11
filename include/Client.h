//
// Created by tadeas on 2025-10-16.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <string>
#include <vector>
#include <random>
#include <unordered_map>

#include "ClientFSM.h"
#include "Packet.h"
#include "Channel.h"

namespace Client {
    class Client {
    private:

        struct ResolvedAddr {
            sockaddr_storage addr;
            socklen_t addr_len;
            int family;
        };

        enum class packetSendingGrowth {
            EXPONENTIAL_GROWTH,
            STEADY_GROWTH,
        };

        ClientFSM state_;
        bool transmitted_;
        std::vector<ResolvedAddr> resolvedAddrs_;
        std::mt19937 rng_;
        std::uniform_int_distribution<int> dist_;
        std::string hostnameArg_;
        Channel::Channel* commsChannel_ = nullptr;
        size_t sequenceNum_ = 0;
        std::unordered_map<size_t, Packet::IcmpPacket> packetsWaitingForAck_;
        std::string inputFile_;
        packetSendingGrowth sendingGrowth_;

        void run();

        /**
         * method for resolving a given hostname's addresses (both IPv4 and IPv6)
         * @param hostname hostname gotten from the user as an argument
         * @return vector of resolved addresses for that hostname
         */
        static std::vector<ResolvedAddr> resolveHostname(const std::string &hostname);

    public:
        explicit Client(std::string hostNameArg);

        void openConn(const ResolvedAddr &resolved_addr);

    };
} // client

#endif //CLIENT_H