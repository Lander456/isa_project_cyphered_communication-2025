#pragma once
#include <string>
#include <getopt.h>

#ifndef ARGPARSER_H
#define ARGPARSER_H

namespace argparser {

    struct Config {
        std::string inputFile;
        std::string ipHostname;
        bool server;
    };

    class ArgParser {
    public:
        static int serverFlag;
        ArgParser(int argc, char** argv);
        Config parse() const;
        static void printHelp(const char** programName);

    private:
        int argc_;
        char** argv_;
        static struct option long_options[];
    };
} //argparser
#endif //ARGPARSER_H
