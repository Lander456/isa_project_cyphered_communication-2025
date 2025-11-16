CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude

LIBS := -lssl -lcrypto

TARGET := secret

SRCS := src/ArgParser.cpp \
	src/Client.cpp \
	src/Server.cpp \
	src/Packet.cpp \
	src/Channel.cpp \
	src/Cipher.cpp \
	main.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(TARGET) clean

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
