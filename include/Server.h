//
// Created by Tadeas Topinka (xtopint00) on 2025-10-16.
//

#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <fstream>

#include "Channel.h"
#include "Cipher.h"
#include "ServerFSM.h"

namespace Server {
    class Greeter {
    public:
        /**
         * Greeter constructor
         * @param family what family the greeter will be communicating over (IPv4 or IPv6)
         * @param die used to pass a pointer to the die bool
         */
        Greeter(int family, std::atomic<bool>& die);

        /**
         * method used to run the Greeter
         */
        void run();

    private:
        /**
         * used to store the current state of the Greeter
         */
        GreeterFSM state_;

        /**
         * Channel instance assigned to this Greeter
         */
        Channel::Channel commsChannel_;

        /**
         * the IP family this Greeter is communicating over
         */
        const int family_;

        /**
         * pid of this Greeter instance
         */
        const pid_t pid_;


        /**
         * Cipher instance assigned to this Greeter
         */
        Cipher::Cipher cipherer_;


        /**
         * die bool, indicating whether this Greeter should end itself
         */
        std::atomic<bool>& die_;

        /**
         * method used to fork a new Receiver instance to facilitate further communication with the client
         * @param clientAddr the Client's address
         * @param commsId ID used for the communication (Client's PID)
         * @param iv intialization vector used for the communication
         * @param die used to pass the die bool pointer
         */
        static void forkReceiver(const sockaddr_storage &clientAddr, const pid_t &commsId, const std::vector<uint8_t> &iv, std::atomic<bool>& die);

    };

    class Receiver {
    public:
        /**
         * Receiver constructor
         * @param remoteAddr address that the receiver will be getting data from
         * @param commsId ID used for the communication (remote's PID)
         * @param iv initialization vector used for the communication
         * @param die used to pass the die bool pointer
         */
        Receiver(const sockaddr_storage &remoteAddr, const pid_t &commsId, const std::vector<uint8_t> &iv, std::atomic<bool>& die);

        /**
         * method used to run the Receiver
         */
        void run();

    private:
        /**
         * the IP family this Receiver instance is communicating over
         */
        const int family_;

        /**
         * attribute containing the current state of this Receiver instance
         */
        ReceiverFSM state_;

        /**
         * Channel instance assigned to this Receiver
         */
        Channel::Channel commsChannel_;

        /**
         * ID used for the communication this Receiver is handling
         */
        const pid_t commsId_;

        /**
         * filestream used to write data into the output file
         */
        std::ofstream outputFile_;

        /**
         * what type the Receiver will use for its ICMP packets, will ever be only 8 (if IPv4) or 128 (if IPv6)
         */
        uint8_t icmpType_;

        /**
         * Cipher instance assigned to this Receiver
         */
        Cipher::Cipher cipherer_;

        /**
         * initialization vector used for this communication
         */
        std::vector<uint8_t> iv_;

        /**
         * name of the file this Receiver is getting from the client
         */
        std::string filename_;

        /**
         * die bool indicating whether the receiver should end itself
         */
        std::atomic<bool>& die_;

    };
} // Server

#endif //SERVER_H