#include "ArgParser.h"
#include <iostream>
#include <getopt.h>
#include <cstring>

namespace argparser {

    int ArgParser::serverFlag = 0;

    ArgParser::ArgParser(int argc, char **argv)
        : argc_(argc), argv_(argv) {}


    static struct option long_options[] = {
        { "filepath", required_argument, nullptr, 'r' },
        { "serverIP", required_argument, nullptr, 's' },
        {"becomeServer", no_argument, &ArgParser::serverFlag, 1},
        {nullptr, 0, nullptr, 0}
    };

    void ArgParser::printHelp(const char **programName) {
        std::cout << "Program usage: " << programName[0] << " -r <file> -s <ip|hostname> [-l]" << std::endl;
    }

    Config ArgParser::parse() const {
        Config config = {};
        int arg;
        int optionIndex;

        while ((arg = getopt_long(argc_, argv_, "r:s:l", long_options, &optionIndex)) != -1) {
            switch (arg) {
                case 'r':
                    config.inputFile = optarg;
                    break;
                case 's':
                    config.ipHostname = optarg;
                    break;
                case 'l':
                    std::cout << "Starting server" << std::endl;
                    break;
                default:
                    std::cerr << "Unknown args passed, exiting!" << std::endl;
                    exit(1);
            }
        }

        return config;
    }
} //argparser