cmake_minimum_required(VERSION 4.0)
project(projekt)

set(CMAKE_CXX_STANDARD 20)

add_executable(projekt
        src/Common/ArgParser.cpp
        include/ArgParser.h
        src/App/Client.cpp
        include/Client.h
        src/App/Server.cpp
        include/Server.h
        main.cpp
        src/Net/Packet.cpp
        include/Packet.h
        src/Net/Channel.cpp
        include/Channel.h
        include/ClientFSM.h
        include/ServerFSM.h
        src/Common/Cipher.cpp
        include/Cipher.h
)
