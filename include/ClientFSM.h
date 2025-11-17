//
// Created by tadeas on 2025-11-01.

#ifndef PROJEKT_CLIENTFSM_H
#define PROJEKT_CLIENTFSM_H

/**
 * enum containing all the Client states
 */
enum class ClientFSM {
    INIT,
    SEND_HELLO,
    AWAIT_HELLO_BACK,
    SEND_FILENAME,
    SEND_DATA,
    TRANSMISSION_COMPLETE,
    SHUTDOWN,
};

#endif //PROJEKT_CLIENTFSM_H