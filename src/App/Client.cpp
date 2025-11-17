//
// Created by tadeas on 2025-10-16.
//

#include <cstdint>
#include <utility>
#include <unistd.h>
#include <fstream>
#include <ranges>
#include <iostream>
#include <cstring>
#include <openssl/rand.h>

#include "../../include/Client.h"

#include <atomic>

namespace Client {

    Client::Client(std::string hostNameArg, std::string inputFile, std::atomic<bool>& die)
        : state_(ClientFSM::INIT), rng_(std::random_device{}()), dist_(std::uniform_int_distribution<int>(1024, 65535)), hostnameArg_(std::move(hostNameArg)), inputFile_(std::move(inputFile)), pid_(getpid()), icmpType_(0), inputFileStream_(inputFile_, std::ios::binary), cipherer_("xtopint00"), iv_(16), die_(die) {
        if (!inputFileStream_.is_open()) {
            std::cerr << "ERROR: Failed to open " << inputFile_ << std::endl;
            exit(1);
        }
    }

    void Client::run() {
        bool run = true;
        size_t currentAddr = 0;

        while (run) {

            if (die_.load(std::memory_order_relaxed)) {
                state_ = ClientFSM::SHUTDOWN;
            }

            switch (state_) {
                case ClientFSM::INIT: {
                    resolvedAddrs_ = resolveHostname(hostnameArg_);
                    state_ = ClientFSM::SEND_HELLO;
                    break;
                }
                case ClientFSM::SEND_HELLO: {

                    if (currentAddr >= resolvedAddrs_.size()) {
                        std::cout << "ERROR: failed to connect to host, please check whether the hostname you entered is correct" << std::endl;
                        exit(0);
                    }

                    openConn(resolvedAddrs_[currentAddr]);
                    if (resolvedAddrs_[currentAddr].family == AF_INET) {
                        icmpType_ = 8;
                    } else {
                        icmpType_ = 128;
                    }

                    RAND_bytes(iv_.data(), iv_.size());

                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::HELLO, iv_);
                    auto serializedPacket = packet.serialize();
                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));

                    state_ = ClientFSM::AWAIT_HELLO_BACK;
                    break;
                }
                case ClientFSM::AWAIT_HELLO_BACK: {
                    Packet::IcmpPacket parsedResponse = commsChannel_->waitForResponse(pid_, reinterpret_cast<sockaddr *>(&commsChannel_->getRemoteAddress()));

                    if (parsedResponse.icmpHeader.type == 0 && parsedResponse.icmpHeader.checksum != 0) {
                        break;
                    }
                    if (parsedResponse.icmpHeader.checksum == 0) {
                        currentAddr++;
                        state_ = ClientFSM::SEND_HELLO;
                        break;
                    }
                    if (parsedResponse.protocol.type == PacketType::HELLO_REPLY) {
                        state_ = ClientFSM::SEND_FILENAME;
                    }
                    break;
                }
                case ClientFSM::SEND_FILENAME: {
                    if (die_.load()) {
                        state_ = ClientFSM::SHUTDOWN;
                        break;
                    }
                    std::vector<uint8_t> filename(inputFile_.begin(), inputFile_.end());

                    filename = cipherer_.encrypt(filename, iv_);

                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::FILENAME, filename);

                    auto serializedPacket = packet.serialize();
                    commsChannel_->sendPacket(serializedPacket.data(), serializedPacket.size(), reinterpret_cast<sockaddr*>(&commsChannel_->getRemoteAddress()));

                    usleep(100);
                    state_ = ClientFSM::SEND_DATA;

                    break;
                }
                case ClientFSM::SEND_DATA: {

                    std::vector<uint8_t> buffer(CHUNK_SIZE);

                    while (true) {
                        if (die_.load(std::memory_order_relaxed)) {
                            state_ = ClientFSM::SHUTDOWN;
                            break;
                        }
                        inputFileStream_.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE * sizeof(uint8_t));
                        std::streamsize bytesRead = inputFileStream_.gcount();

                        if (bytesRead <= 0) {
                            state_ = ClientFSM::TRANSMISSION_COMPLETE;
                            break;
                        }

                        buffer.resize(bytesRead);

                        buffer = cipherer_.encrypt(buffer, iv_);

                        auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::DATA, buffer);
                        auto serializedPacket = packet.serialize();

                        commsChannel_->sendPacket(serializedPacket.data(), serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                        usleep(100);
                        if (errno == EINTR) {
                            state_ = ClientFSM::SHUTDOWN;
                            break;
                        }
                    }
                    break;
                }

                case ClientFSM::TRANSMISSION_COMPLETE: {
                    std::vector<uint8_t> emptyData;
                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::TRANSMISSION_COMPLETE, emptyData);
                    auto serializedPacket = packet.serialize();
                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                    state_ = ClientFSM::SHUTDOWN;
                    break;
                }

                case ClientFSM::SHUTDOWN: {
                    std::vector<uint8_t> emptyData;
                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::GOODBYE, emptyData);
                    auto serializedPacket = packet.serialize();

                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                    delete commsChannel_;
                    run = false;
                    break;
                }
            }
        }
    }

    std::vector<Client::ResolvedAddr> Client::resolveHostname(const std::string &hostname) {
        std::vector<ResolvedAddr> result;

        addrinfo hints = {};
        addrinfo *res;

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_RAW;

        int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);

        if (status != 0) {
            std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
            exit(1);
        }

        for (const addrinfo *p = res; p != nullptr; p = p->ai_next) {
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

    void Client::openConn(const ResolvedAddr &resolvedAddr) {
        delete commsChannel_;
        commsChannel_ = new Channel::Channel(resolvedAddr.family);
        commsChannel_->setRemoteAddress(resolvedAddr.addr);
        commsChannel_->setRemoteAddressLength(resolvedAddr.addr_len);
    }
} // client