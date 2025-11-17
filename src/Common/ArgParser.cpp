//
// Created by Tadeas Topinka (xtopint00) on 2025-10-16.
//

#include <iostream>
#include <cstring>

#include "../../include/ArgParser.h"

namespace argparser {

    ArgParser::ArgParser(int argc, char **argv)
        : argc_(argc), argv_(argv) {}


    Config ArgParser::parse() const {
        Config config = {};
        opterr = 0;
        int arg;
        int optionIndex;
        bool error = false;

        if (argc_ == 1) {
            std::cerr << "No args passed, exiting!" << std::endl;
            error = true;
        }

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
                case '?':
                default:
                    std::cerr << "Unknown args passed, exiting!" << std::endl;
                    exit(1);
            }
        }

        if (optind < argc_) {
            std::cerr << "Unexpected positional argument: " << argv_[optind] << " exiting!" << std::endl;
            error = true;
        }

        if (config.server && (!config.ipHostname.empty() || !config.inputFile.empty())) {
            std::cerr << "Invalid argument combination, exiting!" << std::endl;
            error = true;
        }

        if (error) {
            printHelp(argv_[0]);
            exit(1);
        }

        return config;
    }

    void ArgParser::printHelp(const char *programName) {
        std::cout << "Program usage: " << programName << " -r <file> -s <ip|hostname> [-l]" << std::endl;
    }

    option ArgParser::longOptions[] = {
        { "filepath", required_argument, nullptr, 'r' },
        { "serverIP", required_argument, nullptr, 's' },
        {"becomeServer", no_argument, nullptr, 'l'},
        {nullptr, 0, nullptr, 0}
    };
} //argparser