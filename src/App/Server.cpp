//
// Created by Tadeas Topinka (xtopint00) on 2025-10-16.
//

#include <algorithm>
#include <csignal>
#include <filesystem>
#include <iostream>

#include "../../include/Server.h"

namespace Server {

    Greeter::Greeter(const int family, std::atomic<bool>& die)
    : state_(GreeterFSM::LISTENING), commsChannel_(family), family_(family), pid_(getpid()), cipherer_("xtopint00"), die_(die) { }

    void Greeter::run() {
        bool run = true;
        while (run) {
            if (die_.load(std::memory_order_relaxed)) {
                state_ = GreeterFSM::SHUTDOWN;
            }
            switch (state_) {
                case GreeterFSM::LISTENING: {
                    auto packet = commsChannel_.listen();
                    if (packet.protocol.type == PacketType::HELLO && packet.icmpHeader.type != 0) {
                        forkReceiver(commsChannel_.getRemoteAddress(), packet.icmpHeader.id, packet.data, die_);
                    }
                    break;
                }
                case GreeterFSM::SHUTDOWN: {
                    run = false;
                    break;
                }
            }
        }
    }

    void Greeter::forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId, const std::vector<uint8_t> &iv, std::atomic<bool>& die) {
        pid_t pid = fork();
        if (pid == 0) {
            auto receiver = Receiver(clientAddr, commsId, iv, die);
            receiver.run();
            exit(0);
        }
    }

    Receiver::Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId, const std::vector<uint8_t> &iv, std::atomic<bool>& die)
        : family_(remoteAddr.ss_family), state_(ReceiverFSM::INIT), commsChannel_(remoteAddr.ss_family), commsId_(commsId), outputFile_(), icmpType_(), cipherer_("xtopint00"), die_(die){
        commsChannel_.setRemoteAddress(remoteAddr);
        iv_ = iv;
        switch (family_) {
            case AF_INET: {
                commsChannel_.setRemoteAddressLength(sizeof(sockaddr_in));
                icmpType_ = 8;
                break;
            }
            case AF_INET6: {
                commsChannel_.setRemoteAddressLength(sizeof(sockaddr_in6));
                icmpType_ = 128;
                break;
            }
            default: {
                perror("unknown family");
            }
        }
    }

    void Receiver::run() {
        bool run = true;

        while (run) {
            if (die_.load(std::memory_order_relaxed)) {
                state_ = ReceiverFSM::SHUTDOWN;
            }
            switch (state_) {
                case ReceiverFSM::INIT: {
                    std::vector<uint8_t> emptyData;
                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, commsId_, PacketType::HELLO_REPLY, emptyData);
                    auto serializedPacket = packet.serialize();
                    commsChannel_.sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_.getRemoteAddress()));

                    state_ = ReceiverFSM::RECEIVING;
                    break;
                }
                case ReceiverFSM::RECEIVING: {

                    auto packet = commsChannel_.waitForResponse(commsId_, reinterpret_cast<sockaddr*>(&commsChannel_.getRemoteAddress()));
                    if (packet.icmpHeader.type == 0) {
                        continue;
                    }

                    switch (packet.protocol.type) {
                        case PacketType::FILENAME: {

                            auto decryptedData = cipherer_.decrypt(packet.data, iv_);

                            filename_.assign(decryptedData.begin(), decryptedData.end());
                            filename_.erase(std::find(filename_.begin(), filename_.end(), '\0'), filename_.end());

                            outputFile_.open(filename_, std::ios::binary);

                            break;
                        }
                        case PacketType::DATA: {
                            if (outputFile_.is_open()) {
                                auto decryptedData = cipherer_.decrypt(packet.data, iv_);
                                outputFile_.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
                                outputFile_.flush();
                            } else {
                                std::cerr << "ERROR: Failed to write to file!" << std::endl;
                                state_ = ReceiverFSM::SHUTDOWN;
                            }
                            break;
                        }
                        case PacketType::TRANSMISSION_COMPLETE: {
                            outputFile_.close();
                            if (outputFile_.is_open()) {
                                std::cerr << "ERROR: Failed to close file!" << std::endl;
                                exit(1);
                            }
                            state_ = ReceiverFSM::SHUTDOWN;
                            break;
                        }
                        case PacketType::GOODBYE: {
                            state_ = ReceiverFSM::SHUTDOWN;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }

                case ReceiverFSM::SHUTDOWN: {

                    if (outputFile_.is_open()) {
                        outputFile_.close();
                        std::filesystem::remove(filename_);
                    }

                    run = false;
                    break;
                }
            }
        }
    }
} // Server