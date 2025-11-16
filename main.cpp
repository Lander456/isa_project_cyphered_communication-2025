//
// Created by tadeas on 2025-10-16.
//

#include <iostream>
#include "include/ArgParser.h"
#include "include/Client.h"
#include "include/Server.h"

int main(int argc, char **argv) {
    argparser::ArgParser parser(argc, argv);
    argparser::Config config = parser.parse();

    if (config.server) {
        std::cout << "Running in server mode..." << std::endl;
        pid_t pid = fork();

        if (pid == 0) {
            auto greeterIpv6 = Server::Greeter(AF_INET6);
            greeterIpv6.run();
        } else {
            auto greeterIpv4 = Server::Greeter(AF_INET);
            greeterIpv4.run();
        }
    } else {
        std::cout << config.server << std::endl;
        std::cout << "Input filepath: " << config.inputFile << std::endl;
        std::cout << "IP/Hostname: " << config.ipHostname << std::endl;
        auto client = Client::Client(config.ipHostname, config.inputFile);
        client.run();
    }

    return 0;
}