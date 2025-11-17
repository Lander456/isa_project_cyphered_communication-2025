//
// Created by Tadeas Topinka (xtopint00) on 2025-11-10.
//

#ifndef PROJEKT_SERVERFSM_H
#define PROJEKT_SERVERFSM_H

/**
 * enum containing all the Greeter states
 */
enum class GreeterFSM {
    LISTENING,
    SHUTDOWN
};

/**
 * enum containing all the Receiver states
 */
enum class ReceiverFSM {
    INIT,
    RECEIVING,
    SHUTDOWN
};

#endif //PROJEKT_SERVERFSM_H