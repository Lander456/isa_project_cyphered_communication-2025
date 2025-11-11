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

#include "../include/Client.h"

namespace Client {

    Client::Client(std::string hostNameArg)
        : state_(ClientFSM::INIT), transmitted_(false), rng_(std::random_device{}()), dist_(std::uniform_int_distribution<int>(1024, 65535)), hostnameArg_(std::move(hostNameArg)), sendingGrowth_(packetSendingGrowth::EXPONENTIAL_GROWTH) {}

    void Client::run() {
        bool run = true;
        int retransmitCount = 0;
        int currentAddr = 0;
        int sendAmt = 1;

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
                    auto packet = Packet::IcmpPacket::createPacket(8, 0, getpid(), sequenceNum_, PacketType::HELLO, emptyData);
                    auto serializedPacket = packet.serialize();
                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));

                    Packet::IcmpPacket parsedResponse = commsChannel_->waitForResponse(getpid(), reinterpret_cast<sockaddr *>(&commsChannel_->getRemoteAddress()));

                    if (parsedResponse.icmpHeader.checksum == 0) {
                        currentAddr++;
                        break;
                    }

                    if (parsedResponse.protocol.type == PacketType::HELLO) {
                        state_ = ClientFSM::AWAIT_RECEIVE;
                        sequenceNum_++;
                    } else {
                        state_ = ClientFSM::ERROR;
                    }
                    break;
                }
                case ClientFSM::AWAIT_RECEIVE: {
                    auto receivedPacket = commsChannel_->waitForResponse(getpid(), reinterpret_cast<sockaddr *>(&commsChannel_->getRemoteAddress()));

                    if (receivedPacket.icmpHeader.type == 8 && receivedPacket.icmpHeader.code == 0 && receivedPacket.protocol.type == PacketType::RECEIVING) {
                        auto packet = Packet::IcmpPacket::createPacket(0, 0, getpid(), receivedPacket.icmpHeader.sequence, PacketType::ACK, emptyData);
                        auto serializedPacket = packet.serialize();
                        commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                        state_ = ClientFSM::SEND_DATA;
                    } else {
                        state_ = ClientFSM::ERROR;
                    }
                    break;
                }
                case ClientFSM::SEND_DATA: {
                    std::ifstream file(inputFile_, std::ios::binary);
                    if (!file.is_open()) {
                        std::cerr << "ERROR: Failed to open " << inputFile_ << std::endl;
                        exit(1);
                    }

                    std::vector<uint8_t> buffer(CHUNK_SIZE);

                    for (int i = 0; i < sendAmt; i++) {
                        file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE * sizeof(uint8_t));
                        std::streamsize bytesRead = file.gcount();

                        if (bytesRead <= 0) {
                            transmitted_ = true;
                            state_ = ClientFSM::AWAIT_ACK;
                            break;
                        }

                        auto packet = Packet::IcmpPacket::createPacket(8, 0, getpid(), sequenceNum_, PacketType::DATA, buffer);
                        auto serializedPacket = packet.serialize();

                        commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                        packetsWaitingForAck_.insert({sequenceNum_, packet});
                        sequenceNum_++;
                    }

                    state_ = ClientFSM::AWAIT_ACK;

                    break;
                }

                case ClientFSM::AWAIT_ACK: {
                    while (!packetsWaitingForAck_.empty()) {
                        auto packet = commsChannel_->waitForResponse(getpid(), reinterpret_cast<sockaddr *>(&commsChannel_->getRemoteAddress()));

                        if (packet.icmpHeader.checksum == 0) {
                            continue;
                        }

                        if (packet.protocol.type == PacketType::ACK && packetsWaitingForAck_.contains(packet.icmpHeader.sequence)) {
                            packetsWaitingForAck_.erase(packet.icmpHeader.sequence);

                        } else if (packet.protocol.type == PacketType::TRANSMISSION_HANDOVER) {
                            break;
                        }
                    }

                    if (!packetsWaitingForAck_.empty()) {

                        sendAmt = sendAmt/2;

                        if (sendAmt == 0) {
                            sendAmt++;
                        }

                        if (retransmitCount < 3) {
                            state_ = ClientFSM::RETRANSMIT;
                        } else {
                            state_ = ClientFSM::ERROR;
                        }
                    } else {
                        if (sendingGrowth_ == packetSendingGrowth::EXPONENTIAL_GROWTH) {
                            sendAmt *= 2;
                        } else {
                            sendAmt += 2;
                        }

                        if (transmitted_) {
                            state_ = ClientFSM::TRANSMIT_DONE;
                        } else {
                            state_ = ClientFSM::SEND_DATA;
                        }
                    }

                    retransmitCount = 0;
                    break;
                }

                case ClientFSM::RETRANSMIT: {

                    if (sendingGrowth_ == packetSendingGrowth::EXPONENTIAL_GROWTH) {
                        sendingGrowth_ = packetSendingGrowth::STEADY_GROWTH;
                    }

                    for (auto &packet: packetsWaitingForAck_ | std::views::values) {
                        auto serializedPacket = packet.serialize();

                        commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));
                    }

                    retransmitCount++;
                    state_ = ClientFSM::AWAIT_ACK;
                    break;
                }

                case ClientFSM::SHUTDOWN: {
                    //TODO graceful shutdown
                    run = false;
                    break;
                }
                case ClientFSM::TRANSMIT_DONE: {

                    auto packet = Packet::IcmpPacket::createPacket(8, 0, getpid(), sequenceNum_, PacketType::GOODBYE, emptyData);
                    sequenceNum_++;
                    auto serializedPacket = packet.serialize();

                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));

                    state_ = ClientFSM::SHUTDOWN;
                    break;
                }
                case ClientFSM::ERROR: {
                    auto packet = Packet::IcmpPacket::createPacket(8, 0, getpid(), sequenceNum_, PacketType::ERROR, emptyData);
                    sequenceNum_++;
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

    void Client::openConn(const ResolvedAddr &resolved_addr) {
        commsChannel_ = new Channel::Channel(resolved_addr.family);
        commsChannel_->setRemoteAddress(resolved_addr.addr);
        commsChannel_->setRemoteAddressLength(resolved_addr.addr_len);
    }
} // client