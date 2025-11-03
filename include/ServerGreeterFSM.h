//
// Created by tadeas on 2025-11-01.
//

#ifndef PROJEKT_SERVERGREETERFSM_H
#define PROJEKT_SERVERGREETERFSM_H

enum class serverGreeterFSM {
    INIT,
    LISTENING,
    FORK_RECEIVER,
    SHUTDOWN
};

#endif //PROJEKT_SERVERGREETERFSM_H