#include "../include/ArgParser.h"
#include <iostream>
#include <cstring>

namespace argparser {

    ArgParser::ArgParser(int argc, char **argv)
        : argc_(argc), argv_(argv) {}


    Config ArgParser::parse() const {
        Config config = {};
        int arg;
        int optionIndex;

        while ((arg = getopt_long(argc_, argv_, "r:s:l", longOptions, &optionIndex)) != -1) {
            switch (arg) {
                case 'r':
                    config.inputFile = optarg;
                    break;
                case 's':
                    config.ipHostname = optarg;
                    break;
                case 'l':
                    config.server = true;
                    break;
                default:
                    std::cerr << "Unknown args passed, exiting!" << std::endl;
                    exit(1);
            }
        }

        if (config.server && (!config.ipHostname.empty() || !config.inputFile.empty())) {
            std::cerr << "Invalid argument combination, exiting!" << std::endl;
            printHelp("secret");
            exit(1);
        }

        return config;
    }

    void ArgParser::printHelp(const char *programName) {
        std::cout << "Program usage: " << programName << " -r <file> -s <ip|hostname> [-l]" << std::endl;
    }

    struct option ArgParser::longOptions[] = {
        { "filepath", required_argument, nullptr, 'r' },
        { "serverIP", required_argument, nullptr, 's' },
        {"becomeServer", no_argument, nullptr, 'l'},
        {nullptr, 0, nullptr, 0}
    };
} //argparser