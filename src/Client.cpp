//
// Created by tadeas on 2025-10-16.
//

#include <cstdint>
#include <utility>
#include <unistd.h>
#include <fstream>
#include <ranges>

#include "../include/Client.h"

namespace Client {

    Client::Client(std::string hostNameArg)
        : state_(ClientFSM::INIT), rng_(std::random_device{}()), dist_(std::uniform_int_distribution<int>(1024, 65535)), hostnameArg_(std::move(hostNameArg)), sendingGrowth_(packetSendingGrowth::EXPONENTIAL_GROWTH)
    {
        run();
    }

    void Client::run() {
        bool run = true;
        uint8_t response[1500];
        int retransmitCount = 0;
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
                    int currentAddr = 0;
                    openConn(resolvedAddrs_[currentAddr]);
                    auto packet = Packet::IcmpPacket::createPacket(8, 0, getpid(), sequenceNum_, PacketType::HELLO, emptyData);
                    auto serializedPacket = packet.serialize();
                    commsChannel_->sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_->getRemoteAddress()));

                    Packet::IcmpPacket parsedResponse = commsChannel_->waitForResponse(getpid(), reinterpret_cast<sockaddr *>(&commsChannel_->getRemoteAddress()), commsChannel_->getRemoteAddressLength());

                    if (parsedResponse.header.checksum == 0) {
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
                    if (commsChannel_->receivePacket(&response, sizeof(response), reinterpret_cast<sockaddr*>(&commsChannel_->getRemoteAddress()), sizeof(commsChannel_->getRemoteAddress())) < 0) {
                        perror("socket");
                        exit(1);
                    }

                    auto parsedResponse = Packet::IcmpPacket::parse(response, sizeof(response));

                    if (parsedResponse.header.type == 0 && parsedResponse.header.code == 0 && parsedResponse.protocol.type == PacketType::RECEIVING) {
                        auto packet = Packet::IcmpPacket::createPacket(0, 0, getpid(), parsedResponse.header.sequence, PacketType::ACK, emptyData);
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
                            state_ = ClientFSM::TRANSMIT_DONE;
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
                        auto packet = commsChannel_->waitForResponse(getpid(), reinterpret_cast<sockaddr *>(&commsChannel_->getRemoteAddress()), commsChannel_->getRemoteAddressLength());
                        if (packet.header.sequence == 0) {
                            break;
                        }
                        if (packet.protocol.type == PacketType::ACK && packetsWaitingForAck_.contains(packet.header.sequence)) {
                            packetsWaitingForAck_.erase(packet.header.sequence);
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
        commsChannel_ = new Channel::Channel(resolved_addr.family);
        commsChannel_->setRemoteAddress(resolved_addr.addr);
        commsChannel_->setRemoteAddressLength(resolved_addr.addr_len);
    }
} // client