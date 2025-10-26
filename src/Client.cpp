//
// Created by tadeas on 2025-10-16.
//

#include <cstdint>
#include <unistd.h>
#include "../include/Packet.h"
#include "../include/Client.h"

namespace Client {

    Client::Client()
        : rng_(std::random_device{}()), dist_(std::uniform_int_distribution<int>(1024, 65535)), sockfd_(-1), family_(-1) {
    }

    std::vector<Client::ResolvedAddr> Client::resolveHostname(const std::string &hostname) {
        std::vector<Client::ResolvedAddr> result;

        struct addrinfo hints = {};
        struct addrinfo *res;

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_RAW;

        int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);

        if (status != 0) {
            std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
            exit(EXIT_FAILURE);
        }

        for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
            ResolvedAddr addr{};

            if (p->ai_addrlen <= sizeof(sockaddr_storage)) {
                std::memcpy(&addr.addr, p->ai_addr, p->ai_addrlen);
                addr.addr_len = p->ai_addrlen;
                addr.family = p->ai_family;

                result.push_back(addr);
            }
        }
        freeaddrinfo(res);

        return result;
    }

    void Client::openConn(const ResolvedAddr &resolved_addr) {
        family_ = resolved_addr.family;
        if (resolved_addr.family == AF_INET) {
            sockfd_ = socket(resolved_addr.family, SOCK_RAW, IPPROTO_ICMP);
        } else {
            sockfd_ = socket(resolved_addr.family, SOCK_RAW, IPPROTO_ICMPV6);
        }

        if (sockfd_ == -1) {
            std::cerr << "ERROR: failed to create socket" << std::endl;
            exit(2);
        }
    }

    void Client::closeConn() const {
        close(sockfd_);
    }

    void Client::sendPacket(const IcmpPacket* packet, const ResolvedAddr &resolved_addr) {
        if (sendto(sockfd_, packet, sizeof(*packet), 0, reinterpret_cast<const sockaddr*>(&resolved_addr.addr), resolved_addr.addr_len) == -1) {
            perror("failed to send packet");
            closeConn();
        }
    }

    void Client::awaitConfirm(uint16_t sequenceNum) {
        uint8_t packetBuffer[sizeof(IcmpPacket)];
        sockaddr_storage srcAddr{};
        socklen_t srcAddrLen = sizeof(srcAddr);

        auto received = recvfrom(sockfd_, packetBuffer, sizeof(packetBuffer), 0, reinterpret_cast<sockaddr*>(&srcAddr), &srcAddrLen);
        if (received == -1) {
            perror("receive failed");
            return;
        }

        int ipHeaderLen = 0;

        if (family_ == AF_INET) {
            ipHeaderLen = (packetBuffer[0] & 0x0F) * 4;
        }

        auto* icmpHeader = reinterpret_cast<IcmpHeader*>(packetBuffer + ipHeaderLen);

        if (icmpHeader->sequenceNum == sequenceNum) {

        }
    }



} // client