//
// Created by tadeas on 2025-10-21.

#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <netinet/in.h>
#include <netinet/ip.h>

/**
 * enum containing all the different packet types
 */
enum class PacketType : uint8_t {
    HELLO = 200,
    DATA = 201,
    GOODBYE = 202,
    HELLO_REPLY = 203,
    FILENAME = 204,
    TRANSMISSION_COMPLETE = 205
};

#pragma pack(push, 1)
namespace Packet {

    /**
     * data structure used to describe the icmpHeader
     */
    struct icmpHeader_t {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t id;
        uint16_t sequence;
    };

    /**
     * data structure used to describe the custom data transfer protocol
     */
    struct protocolInfo {
        PacketType type;
        uint16_t payloadLength;
    };

    /**
     * data structure used to describe the pseudo header used for checksum calculation when communicating over IPv6
     */
    struct PseudoHeader {
        in6_addr source;
        in6_addr destination;
        uint32_t length;
        uint8_t zeros[3];
        uint8_t nextHeader;
    };
#pragma pack(pop)

    class IcmpPacket {
    public:

        /**
         * attribute used to store the ip header of a packet
         */
        ip ipHeader{};

        /**
         * attribute used to store the icmpHeader of a packet
         */
        icmpHeader_t icmpHeader{};

        /**
         * attribute used to store the custom protocol header of a packet
         */
        protocolInfo protocol{};

        /**
         * attribute used to store the data carried by the packet
         */
        std::vector<uint8_t> data;

        /**
         * method used to get the checksum for IPv4 packets
         */
        void getChecksumIPv4();

        /**
         * method used to get the checksum for IPv6 packets
         * @param source source IP address, used in constructing the pseudo header
         * @param destination destination ip address, used in constructing the pseudo header
         */
        void getChecksumIPv6(const in6_addr &source, const in6_addr &destination);

        /**
         * method used to serialize the packet and prepare it to be sent off
         * @return vector of bytes with the serialized packet in them
         */
        [[nodiscard]] std::vector<uint8_t> serialize() const;

        /**
         * method used to parse data, that have been received
         * @param data received data
         * @param length length of the received data
         * @return IcmpPacket instance with the parsed packet, if an error occurs while parsing, returns an empty instance
         */
        static IcmpPacket parse(const uint8_t* data, size_t length);

        /**
         * method used to create a new IcmpPacket instance
         * @param icmpType the icmp type of the packet
         * @param code the icmp code of the packet
         * @param id communication ID of the packet
         * @param protocolType protocol type of the packet
         * @param data data to be carried by the packet
         * @return IcmpPacket instance with the created packet, if an error occurs while creating, return an empty instance
         */
        static IcmpPacket createPacket(uint8_t icmpType, uint8_t code, uint16_t id, PacketType protocolType, const std::vector<uint8_t> &data);

        /**
         * method used to calculate the checksum for the passed data
         * @param data serialized data
         * @param length length of the serialized data
         * @return calculated checksum
         */
        static uint16_t calculateChecksum(const void* data, size_t length);

        /**
         * method used to swap the byte order of a received packet
         * @param packet packet that will have its byte order swapped
         */
        static void swapByteOrder(IcmpPacket &packet);
    };
} //Packet
#endif //PACKET_H