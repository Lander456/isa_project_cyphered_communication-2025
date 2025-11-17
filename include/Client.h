//
// Created by Tadeas Topinka (xtopint00) on 2025-10-16.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <atomic>
#include <string>
#include <vector>
#include <random>
#include <fstream>

#include "ClientFSM.h"
#include "Packet.h"
#include "Channel.h"
#include "Cipher.h"

namespace Client {
    class Client {
    private:

        /**
         * data structure used to hold the information about a resolved address
         */
        struct ResolvedAddr {
            sockaddr_storage addr;
            socklen_t addr_len;
            int family;
        };

        /**
         * variable containing the current state of the Client
         */
        ClientFSM state_;


        /**
         * resolved addresses vector, containing all resolved addresses
         */
        std::vector<ResolvedAddr> resolvedAddrs_;


        /**
         * random number used to get a socket for the Client
         */
        std::mt19937 rng_;

        /**
         * uniform distribution used to initialize the random number
         */
        std::uniform_int_distribution<int> dist_;

        /**
         * the hostname that was passed to the program on startup (could also be an IP address)
         */
        std::string hostnameArg_;

        /**
         * Channel instance assigned to this Client, initialized as a nullptr
         */
        Channel::Channel* commsChannel_ = nullptr;

        /**
         * name of the file that will be sent to the server
         */
        std::string inputFile_;

        /**
         * pid of this Client, will be used as a communication ID
         */
        const pid_t pid_;

        /**
         * what type the client will use for its ICMP packets, will ever be only 8 (if IPv4) or 128 (if IPv6)
         */
        uint8_t icmpType_;

        /**
         * filestream used to read the input file
         */
        std::ifstream inputFileStream_;

        /**
         * Cipher instance used by this Client
         */
        Cipher::Cipher cipherer_;

        /**
         * initialization vector used in the communication conducted by this client
         */
        std::vector<uint8_t> iv_;

        /**
         * pointer to an atomic bool determining whether the Client should end itself
         */
        std::atomic<bool>& die_;

        /**
         * method for resolving a given hostname's addresses (both IPv4 and IPv6)
         * @param hostname hostname gotten from the user as an argument
         * @return vector of resolved addresses for that hostname
         */
        static std::vector<ResolvedAddr> resolveHostname(const std::string &hostname);

    public:
        /**
         * Client constructor used to initialize a client instance
         * @param hostNameArg hostname or IP address passed to the program as the destination for sending data
         * @param inputFile filename that will be sent to the server
         * @param die parameter used to pass on the die bool pointer
         */
        Client(std::string hostNameArg, std::string inputFile, std::atomic<bool>& die);

        /**
         * method used to instantiate a new Channel instance
         * @param resolvedAddr address with which this Channel instance will communicate
         */
        void openConn(const ResolvedAddr &resolvedAddr);

        /**
         * run method used to run the Client
         */
        void run();

    };
} // client

#endif //CLIENT_H