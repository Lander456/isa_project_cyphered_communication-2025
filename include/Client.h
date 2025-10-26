//
// Created by tadeas on 2025-10-16.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include "Packet.h"

namespace Client {
    class Client {
    private:

        struct ResolvedAddr {
            sockaddr_storage addr;
            socklen_t addr_len;
            int family;
        };

        std::vector<ResolvedAddr> resolvedAddrs_;
        int sockfd_;
        int family_;
        std::mt19937 rng_;
        std::uniform_int_distribution<int> dist_;

        /**
         * method for resolving a given hostname's addresses (both IPv4 and IPv6)
         * @param hostname hostname gotten from the user as an argument
         * @return vector of resolved addresses for that hostname
         */
        static std::vector<ResolvedAddr> resolveHostname(const std::string &hostname);

    public:
        Client();

        void openConn(const ResolvedAddr &resolved_addr);

        void closeConn() const;

        void sendPacket(const Packet::IcmpPacket* packet, const ResolvedAddr &resolved_addr);


    };
} // client

#endif //CLIENT_H