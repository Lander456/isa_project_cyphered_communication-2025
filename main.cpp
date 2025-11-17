//
// Created by tadeas on 2025-10-16.
//

#include <iostream>
#include <csignal>

#include "include/ArgParser.h"
#include "include/Client.h"
#include "include/Server.h"

std::atomic<bool> globalDeathSignal(false);

void signalHandler(int) {
    globalDeathSignal.store(true, std::memory_order_relaxed);
}

int main(int argc, char **argv) {

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    argparser::ArgParser parser(argc, argv);
    argparser::Config config = parser.parse();

    if (config.server) {
        pid_t pid = fork();

        if (pid == 0) {
            auto greeterIpv6 = Server::Greeter(AF_INET6, globalDeathSignal);
            greeterIpv6.run();
            _exit(0);
        } else {
            auto greeterIpv4 = Server::Greeter(AF_INET, globalDeathSignal);
            greeterIpv4.run();
        }
    } else {
        auto client = Client::Client(config.ipHostname, config.inputFile, globalDeathSignal);
        client.run();
    }

    return 0;
}