#include "Protocol.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdint>
#include <vector>

bool sendAll(int fd, const char* data, std::size_t length)
{
    std::size_t total_sent = 0;

    while (total_sent < length) {
        ssize_t bytes_sent = send(
            fd,
            data + total_sent,
            length - total_sent,
            0
        );

        if (bytes_sent <= 0) {
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

bool recvAll(int fd, char* buffer, std::size_t length)
{
    std::size_t total_received = 0;

    while (total_received < length) {
        ssize_t bytes_received = recv(
            fd,
            buffer + total_received,
            length - total_received,
            0
        );

        if (bytes_received <= 0) {
            return false;
        }

        total_received += bytes_received;
    }

    return true;
}

// 按照当前协议发送一条完整消息：
// [4字节长度][消息正文]
bool sendMessage(int fd, const std::string& message)
{
    uint32_t length = message.size();
    uint32_t network_length = htonl(length);

    // 先发送 4 字节长度头。
    if (!sendAll(fd, reinterpret_cast<const char*>(&network_length), sizeof(network_length))) {
        return false;
    }

    // 再发送消息正文。
    if (!sendAll(fd, message.c_str(), message.size())) {
        return false;
    }

    return true;
}

// 按照当前协议接收一条完整消息：
// [4字节长度][消息正文]
bool recvMessage(int fd, std::string& message)
{
    uint32_t network_length = 0;

    // 先读取 4 字节长度头。
    if (!recvAll(fd, reinterpret_cast<char*>(&network_length), sizeof(network_length))) {
        return false;
    }

    // 网络字节序转主机字节序。
    uint32_t length = ntohl(network_length);

    // 简单防御：拒绝空消息和过大的消息。
    if (length == 0 || length > 4096) {
        return false;
    }

    std::vector<char> buffer(length);

    // 再根据长度读取完整消息正文。
    if (!recvAll(fd, buffer.data(), length)) {
        return false;
    }

    message.assign(buffer.begin(), buffer.end());

    return true;
}