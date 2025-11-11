cmake_minimum_required(VERSION 4.0)
project(projekt)

set(CMAKE_CXX_STANDARD 20)

add_executable(projekt
        src/ArgParser.cpp
        include/ArgParser.h
        src/Client.cpp
        include/Client.h
        src/Server.cpp
        include/Server.h
        main.cpp
        src/Packet.cpp
        include/Packet.h
        include/Common.h
        src/Channel.cpp
        include/Channel.h
        include/ClientFSM.h
        include/ServerFSM.h
)
