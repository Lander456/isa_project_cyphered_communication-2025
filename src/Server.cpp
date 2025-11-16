//
// Created by tadeas on 2025-10-16.
//

#include "../include/Server.h"

#include <iostream>

namespace Server {

    Greeter::Greeter(const int family) : state_(GreeterFSM::LISTENING), commsChannel_(family), family_(family), pid_(getpid()), cipherer_("xtopint00") { }

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
                std::cout << "packet.data.size() = " << packet.data.size() << std::endl;
                forkReceiver(commsChannel_.getRemoteAddress(), packet.icmpHeader.id, packet.data);
            }
        }
    }

    void Greeter::forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId, const std::vector<uint8_t> &iv) {
        pid_t pid = fork();
        if (pid == 0) {
            auto receiver = Receiver(clientAddr, commsId, iv);
            receiver.run();
            exit(0);
        }
    }

    Receiver::Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId, const std::vector<uint8_t> &iv)
        : family_(remoteAddr.ss_family), state_(ReceiverFSM::INIT), commsChannel_(remoteAddr.ss_family), commsId_(commsId), outputFile_(), icmpType_(), cipherer_("xtopint00") {
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
            std::vector<uint8_t> emptyData;
            switch (state_) {
                case ReceiverFSM::INIT: {
                    auto packet = Packet::IcmpPacket::createPacket(icmpType_, 0, commsId_, PacketType::HELLO_REPLY, emptyData);
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

                            auto decryptedData = cipherer_.decrypt(packet.data, iv_);

                            std::string filename(decryptedData.begin(), decryptedData.end());
                            std::cout << filename << std::endl;
                            filename.erase(std::find(filename.begin(), filename.end(), '\0'), filename.end());

                            std::cout << "filename received: " << filename << std::endl;
                            outputFile_.open(filename, std::ios::binary);

                            break;
                        }
                        case PacketType::DATA: {
                            std::cout << "WRITE" << std::endl;
                            if (outputFile_.is_open()) {
                                auto decryptedData = cipherer_.decrypt(packet.data, iv_);
                                outputFile_.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
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
} // Server