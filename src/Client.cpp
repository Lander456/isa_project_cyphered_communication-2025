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

#include "../include/Client.h"

namespace Client {

    Client::Client(std::string hostNameArg, std::string inputFile)
        : state_(ClientFSM::INIT), transmitted_(false), rng_(std::random_device{}()), dist_(std::uniform_int_distribution<int>(1024, 65535)), hostnameArg_(std::move(hostNameArg)), inputFile_(std::move(inputFile)), pid_(getpid()), icmpType_(0), inputFileStream_(inputFile_, std::ios::binary), cipherer_("xtopint00"), iv_(16) {
        if (!inputFileStream_.is_open()) {
            std::cerr << "ERROR: Failed to open " << inputFile_ << std::endl;
            exit(1);
        }
    }

    void Client::run() {
        bool run = true;
        int currentAddr = 0;

        while (run) {
            std::vector<uint8_t> emptyData;
            switch (state_) {
                case ClientFSM::INIT: {
                    resolvedAddrs_ = resolveHostname(hostnameArg_);
                    state_ = ClientFSM::SEND_HELLO;
                    break;
                }
                case ClientFSM::SEND_HELLO: {

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
                        inputFileStream_.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE * sizeof(uint8_t));
                        std::streamsize bytesRead = inputFileStream_.gcount();

                        if (bytesRead <= 0) {
                            state_ = ClientFSM::TRANSMIT_DONE;
                            break;
                        }

                        buffer.resize(bytesRead);

                        buffer = cipherer_.encrypt(buffer, iv_);

                        auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::DATA, buffer);
                        auto serializedPacket = packet.serialize();

                        commsChannel_->sendPacket(serializedPacket.data(), serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                        usleep(100);
                    }

                    break;
                }

                case ClientFSM::SHUTDOWN: {
                    //TODO graceful shutdown
                    run = false;
                    break;
                }
                case ClientFSM::TRANSMIT_DONE: {

                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::GOODBYE, emptyData);
                    auto serializedPacket = packet.serialize();

                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));

                    state_ = ClientFSM::SHUTDOWN;
                    break;
                }
                case ClientFSM::ERROR: {
                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, pid_, PacketType::ERROR, emptyData);
                    auto serializedPacket = packet.serialize();

                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size(), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));

                    state_ = ClientFSM::SHUTDOWN;
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
            exit(EXIT_FAILURE);
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
        commsChannel_ = new Channel::Channel(resolvedAddr.family);
        commsChannel_->setRemoteAddress(resolvedAddr.addr);
        commsChannel_->setRemoteAddressLength(resolvedAddr.addr_len);
    }
} // client