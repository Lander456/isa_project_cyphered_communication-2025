//
// Created by tadeas on 2025-11-01.
//

#ifndef PROJEKT_CLIENTFSM_H
#define PROJEKT_CLIENTFSM_H

enum class ClientFSM {
    INIT,
    SEND_HELLO,
    AWAIT_RECEIVE,
    SEND_DATA,
    AWAIT_ACK,
    RETRANSMIT,
    TRANSMIT_DONE,
    SHUTDOWN,
    ERROR
};

#endif //PROJEKT_CLIENTFSM_H