//
// Created by tadeas on 2025-10-16.
//

#include "../include/Server.h"

namespace Server {

    Greeter::Greeter(const int family) : state_(GreeterFSM::LISTENING), commsChannel_(family), family_(family) {
    }

    void Greeter::run() {
        switch (state_) {
            case GreeterFSM::INIT: {

                break;
            }
            case GreeterFSM::LISTENING: {
                while(true) {

                }
                break;
            }
            case GreeterFSM::FORKING: {

                break;
            }
            case GreeterFSM::ERROR: {

                break;
            }
        }
    }

    Packet::IcmpPacket Greeter::listen() {
        while (true) {
            auto packet = commsChannel_.listen();

            if (packet.protocol.type == PacketType::HELLO) {
                forkReceiver(family_, commsChannel_.getRemoteAddress());
            }
        }
    }

    pid_t Greeter::forkReceiver(int family, sockaddr_storage clientAddr) {
    }

    Receiver::Receiver(int family)
        : family_(family), state_(ReceiverFSM::RECEIVING) {

    }

} // Server