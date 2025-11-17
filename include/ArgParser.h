#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <string>
#include <getopt.h>

namespace argparser {

    /**
     * Config struct is used to store the running configuration of the program
     */
    struct Config {
        std::string inputFile;
        std::string ipHostname;
        bool server;
    };

    class ArgParser {
    public:
        /**
         * serverFlag is set based on whether the -l argument has been passed
         */
        static int serverFlag;

        /**
         * argparser class constructor
         * @param argc argument count from command line
         * @param argv argument values from command line
         */
        ArgParser(int argc, char** argv);

        /**
         * method used to parse the command line args passed to the program
         * @return an instance of the Config struct containing information about the running configuration
         */
        [[nodiscard]] Config parse() const;

        /**
         * method used to print the help message, invoked whenever incomplete or incorrect arguments are detected,
         * before exiting the program
         * @param programName program name to be printed in the help message
         */
        static void printHelp(const char* programName);

    private:
        int argc_;
        char** argv_;
        static option longOptions[];
    };
} //argparser
#endif //ARGPARSER_H
