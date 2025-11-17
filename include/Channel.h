//
// Created by tadeas on 2025-10-26.
//

#ifndef PROJEKT_CHANNEL_H
#define PROJEKT_CHANNEL_H

#include <unistd.h>
#include <netdb.h>
#include <chrono>

#include "Packet.h"

#define CHUNK_SIZE 1400
#define TIMEOUT_MS 2000

namespace Channel {
    class Channel {
    public:

        /**
         * Channel class constructor
         * @param family indicates what family the channel will support, either IPv6 or IPv4
         */
        explicit Channel(int family);

        /**
         * Channel destructor
         */
        ~Channel();

        /**
         * method used to send a single serialized packet to the destination
         * @param data serialized packet
         * @param len length of the serialized packet
         * @param destination destination address
         * @return ssize_t indicating the amount of bytes sent
         */
        ssize_t sendPacket(const void* data, size_t len, const sockaddr* destination) const;

        /**
         * method used to receive a packet from a source
         * @param buffer buffer to write the received data into
         * @param len length of the received data
         * @param source variable to store the source address in
         * @param sourceLength length of the source address
         * @return ssize_t indicating the amount of bytes received
         */
        ssize_t receivePacket(void* buffer, size_t len, sockaddr* source, socklen_t sourceLength) const;

        /**
         * method use to have the channel wait for a response from a source, utilizes a timeout to let the caller know
         * that the channel has been waiting for too long
         * @param expectedId communication ID that the Channel is expecting a message from
         * @param source variable that will be filled with the source address
         * @return either an empty instance of IcmpPacket, if an error or timeout occured, a parsed packet in all other
         * cases
         */
        Packet::IcmpPacket waitForResponse(uint16_t expectedId, sockaddr* source);

        /**
         * method used to set the remoteAddress_ of the Channel instance to a certain value
         * @param address value that the instance's remoteAddress_ will be set to
         */
        void setRemoteAddress(const sockaddr_storage& address) {remoteAddress_ = address;}

        /**
         * method used to set the remoteAddressLength_ of the Channel instance to a certain value
         * @param remoteAddressLength value that the instance's remoteAddressLength_ will be set to
         */
        void setRemoteAddressLength(socklen_t remoteAddressLength) {remoteAddressLength_ = remoteAddressLength;}

        /**
         * method used to get the remoteAddressLength_ of the Channel instance
         * @return remoteAddressLength_ of the Channel instance
         */
        [[nodiscard]] socklen_t getRemoteAddressLength() const {return remoteAddressLength_;}

        /**
         * method used to get the remoteAddress_ of the Channel instance
         * @return remoteAddress_ of the Channel instance
         */
        [[nodiscard]] sockaddr_storage& getRemoteAddress() {return remoteAddress_;}

        /**
         * method used to have the Channel listen for any communication, created specifically for the Greeter
         * @return parsed IcmpPacket that has been heard on the interface, empty IcmpPacket instance in case of an error
         */
        Packet::IcmpPacket listen();

    private:
        /**
         * socket file descriptor used by this Channel instance
         */
        int sockfd_{-1};

        /**
         * family this Channel instance is operating on (IPv4/IPv6)
         */
        int family_;

        /**
         * remote IP address this Channel instance is communicating with
         */
        sockaddr_storage remoteAddress_;

        /**
         * length of the remote IP address this Channel instance is communicating with
         */
        socklen_t remoteAddressLength_;
    };
} // Channel

#endif //PROJEKT_CHANNEL_H