#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <string>
#include <getopt.h>

namespace argparser {

    struct Config {
        std::string inputFile;
        std::string ipHostname;
        bool server;
    };

    class ArgParser {
    public:
        static int serverFlag;

        /**
         * argparser class constructor
         * @param argc argument count from command line
         * @param argv argument values from command line
         */
        ArgParser(int argc, char** argv);

        /**
         * method used to parse the command line args passed to the program
         * @return
         */
        [[nodiscard]] Config parse() const;

        static void printHelp(const char* programName);

    private:
        int argc_;
        char** argv_;
        static struct option longOptions[];
    };
} //argparser
#endif //ARGPARSER_H
