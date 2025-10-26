//
// Created by tadeas on 2025-10-16.
//

#include <iostream>
#include "include/ArgParser.h"

int main(int argc, char **argv) {
    argparser::ArgParser parser(argc, argv);
    argparser::Config config = parser.parse();

    if (config.server) {
        std::cout << "Running in server mode..." << std::endl;
    } else {
        std::cout << config.server << std::endl;
        std::cout << "Input filepath: " << config.inputFile << std::endl;
        std::cout << "IP/Hostname: " << config.ipHostname << std::endl;
    }

    return 0;
}