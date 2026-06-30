#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstddef>
#include <string>

bool sendAll(int fd, const char* data, std::size_t length);
bool recvAll(int fd, char* buffer, std::size_t length);

bool sendMessage(int fd, const std::string& message);
bool recvMessage(int fd, std::string& message);

#endif