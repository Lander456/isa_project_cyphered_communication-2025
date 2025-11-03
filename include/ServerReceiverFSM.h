//
// Created by tadeas on 2025-11-01.
//

#ifndef PROJEKT_SERVERRECEIVERFSM_H
#define PROJEKT_SERVERRECEIVERFSM_H
enum class serverReceiverFSM {
    INIT,
    LISTENING,
    ACKING,
    SHUTDOWN
};
#endif //PROJEKT_SERVERRECEIVERFSM_H