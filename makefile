CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -pedantic

LIBS := -lssl -lcrypto

TARGET := secret

SRCS := src/Common/ArgParser.cpp \
	src/App/Client.cpp \
	src/App/Server.cpp \
	src/Net/Packet.cpp \
	src/Net/Channel.cpp \
	src/Common/Cipher.cpp \
	main.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(TARGET) clean

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
