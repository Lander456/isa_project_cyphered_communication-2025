cmake_minimum_required(VERSION 4.0)
project(projekt)

set(CMAKE_CXX_STANDARD 20)

add_executable(projekt
        argparser/ArgParser.cpp
        argparser/ArgParser.h
        client/client.cpp
        client/client.h
        server/server.cpp
        server/server.h)
