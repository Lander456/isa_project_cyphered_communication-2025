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

            if (packet.protocol.type == PacketType::HELLO && packet.icmpHeader.sequence == 0) {
                std::cout << "forking" << std::endl;
                forkReceiver(commsChannel_.getRemoteAddress(), packet.icmpHeader.id, packet.icmpHeader.sequence);
            }
        }
    }

    void Greeter::forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId, const uint16_t commsSequence) {
        pid_t pid = fork();
        if (pid == 0) {
            auto receiver = Receiver(clientAddr, commsId, commsSequence);
            receiver.run();
            exit(0);
        }
    }

    Receiver::Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId, const uint16_t commsSequence)
        : family_(remoteAddr.ss_family), pid_(getpid()), state_(ReceiverFSM::INIT), commsChannel_(remoteAddr.ss_family), commsId_(commsId), commsSequence_(commsSequence) {
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
                    commsSequence_++;
                    auto packet = Packet::IcmpPacket::createPacket(8, 0, commsId_, commsSequence_, PacketType::HELLO, emptyData);
                    auto serializedPacket = packet.serialize();
                    commsChannel_.sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_.getRemoteAddress()));
                    state_ = ReceiverFSM::RECEIVING;
                    break;
                }
                case ReceiverFSM::RECEIVING: {
                    auto receivingPacket = Packet::IcmpPacket::createPacket(8, 0, commsId_, commsSequence_, PacketType::RECEIVING, emptyData);
                    auto serializedPacket = receivingPacket.serialize();
                    commsChannel_.sendPacket(&serializedPacket[0], serializedPacket.size() * sizeof(uint8_t), reinterpret_cast<const sockaddr*>(&commsChannel_.getRemoteAddress()));
                    auto packet = commsChannel_.waitForResponse(commsId_, reinterpret_cast<sockaddr*>(&commsChannel_.getRemoteAddress()));
                    ackSequences_.push_back(packet.icmpHeader.sequence);
                    switch (packet.protocol.type) {
                        case PacketType::DATA: {
                            dataSequence_.push_back(packet.protocol.dataSequence);
                            break;
                        }
                        case PacketType::TRANSMISSION_HANDOVER: {
                            state_ = ReceiverFSM::ACKING;
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
                case ReceiverFSM::ACKING: {
                    if (!dataSequence_.empty()) {
                        std::sort(dataSequence_.begin(), dataSequence_.end());

                        for (size_t i = 0; i < dataSequence_.size(); i++) {
                            //TODO writing
                        }

                        for (auto sequenceNum = ackSequences_.begin(); sequenceNum != ackSequences_.end();) {

                            if (!commsChannel_.sendAck(commsId_, *sequenceNum)) {
                                perror("sendAck");
                                exit(1);
                            }

                            sequenceNum = ackSequences_.erase(sequenceNum);
                        }
                    }
                    if (!commsChannel_.transmissionHandover(commsId_, commsSequence_)) {
                        perror("handover send");
                        exit(1);
                    }

                    state_ = ReceiverFSM::RECEIVING;
                    break;
                }
                case ReceiverFSM::ERROR: {

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