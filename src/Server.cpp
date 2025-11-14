//
// Created by tadeas on 2025-10-16.
//

#include "../include/Server.h"

#include <iostream>

namespace Server {

    Greeter::Greeter(const int family) : state_(GreeterFSM::LISTENING), commsChannel_(family), family_(family), pid_(getpid()) { }

    void Greeter::run() {
        switch (state_) {
            case GreeterFSM::INIT: {
                state_ = GreeterFSM::LISTENING;
                break;
            }
            case GreeterFSM::LISTENING: {
                listen();
            }
            case GreeterFSM::ERROR: {

                break;
            }
        }
    }

    void Greeter::listen() {
        while (true) {
            auto packet = commsChannel_.listen();

            if (packet.protocol.type == PacketType::HELLO && packet.icmpHeader.type != 0) {
                std::cout << "forking" << std::endl;
                forkReceiver(commsChannel_.getRemoteAddress(), packet.icmpHeader.id);
            }
        }
    }

    void Greeter::forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId) {
        pid_t pid = fork();
        if (pid == 0) {
            auto receiver = Receiver(clientAddr, commsId);
            receiver.run();
            exit(0);
        }
    }

    Receiver::Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId)
        : family_(remoteAddr.ss_family), state_(ReceiverFSM::INIT), commsChannel_(remoteAddr.ss_family), commsId_(commsId), outputFile_() {
        commsChannel_.setRemoteAddress(remoteAddr);
        switch (family_) {
            case AF_INET: {
                commsChannel_.setRemoteAddressLength(sizeof(sockaddr_in));
                break;
            }
            case AF_INET6: {
                commsChannel_.setRemoteAddressLength(sizeof(sockaddr_in6));
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
            std::vector<uint8_t> emptyData;
            switch (state_) {
                case ReceiverFSM::INIT: {
                    auto packet = Packet::IcmpPacket::createPacket(8, 0, commsId_, PacketType::HELLO_REPLY, emptyData);
                    auto serializedPacket = packet.serialize();
                    commsChannel_.sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_.getRemoteAddress()));

                    state_ = ReceiverFSM::RECEIVING;
                    break;
                }
                case ReceiverFSM::RECEIVING: {
                    std::cout << "RECEIVING" << std::endl;

                    auto packet = commsChannel_.waitForResponse(commsId_, reinterpret_cast<sockaddr*>(&commsChannel_.getRemoteAddress()));
                    if (packet.icmpHeader.type == 0) {
                        continue;
                    }

                    switch (packet.protocol.type) {
                        case PacketType::FILENAME: {
                            std::cout << "FILENAME" << std::endl;
                            std::string filename(packet.data.begin(), packet.data.end());
                            filename.erase(std::find(filename.begin(), filename.end(), '\0'), filename.end());

                            std::cout << "filename received: " << filename << std::endl;
                            std::cout << "Before open, state = " << outputFile_.rdstate() << std::endl;
                            outputFile_.open(filename, std::ios::binary);
                            std::cout << "After open, state = " << outputFile_.rdstate() << std::endl;

                            break;
                        }
                        case PacketType::DATA: {
                            std::cout << "WRITE" << std::endl;
                            if (outputFile_.is_open()) {
                                outputFile_.write(reinterpret_cast<const char*>(packet.data.data()), packet.data.size());
                                outputFile_.flush();
                            } else {
                                std::cout << "FUCK" << std::endl;
                            }
                            break;
                        }
                        case PacketType::GOODBYE: {
                            outputFile_.close();
                            if (outputFile_.is_open()) {
                                std::cout << "FUCK" << std::endl;
                            }
                            state_ = ReceiverFSM::SHUTDOWN;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }
                case ReceiverFSM::ERROR: {
                    state_ = ReceiverFSM::SHUTDOWN;
                    break;
                }

                case ReceiverFSM::SHUTDOWN: {
                    run = false;
                    break;
                }
            }
        }
    }

    bool Receiver::awaitTransmissionWindow() {
        auto start = std::chrono::steady_clock::now();
        while (true) {
            auto now = std::chrono::steady_clock::now();
            int timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

            auto packet = commsChannel_.waitForResponse(commsId_, reinterpret_cast<sockaddr*>(&commsChannel_.getRemoteAddress()));

            if (timePassed > TIMEOUT_MS || packet.icmpHeader.checksum == 0) {
                return false;
            }

            if (packet.icmpHeader.type == 0 && packet.icmpHeader.checksum != 0) {
                continue;
            }

            if (packet.protocol.type == PacketType::TRANSMISSION_HANDOVER) {
                return true;
            }
        }
    }
} // Server