//
// Created by tadeas on 2025-11-10.
//

#ifndef PROJEKT_SERVERFSM_H
#define PROJEKT_SERVERFSM_H

enum class GreeterFSM {
    INIT,
    LISTENING,
    ERROR
};

enum class ReceiverFSM {
    INIT,
    RECEIVING,
    ERROR,
    SHUTDOWN
};

#endif //PROJEKT_SERVERFSM_H