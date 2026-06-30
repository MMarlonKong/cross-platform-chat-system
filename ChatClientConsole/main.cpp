#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include "../Common/Protocol.h"
using namespace std;

int main()
{
    // <1>: 准备ip地址和端口，"127.0.0.1"表示本机
    const char* SERVER_IP = "127.0.0.1";
    const int SERVER_PORT = 8888;

    // <2>: 创建客户端 socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd == -1) {
        cout << "create client socket error" << endl;
        return 1;
    }

    // <3>: 准备想要连接的服务器的地址
    //客户端不需要 bind()，因为它不需要固定监听端口。
    //客户端要准备的是“我要连接谁”。

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    //这里和服务端不同：
    //    服务端用 INADDR_ANY：表示监听本机所有网卡
    //    客户端用 inet_addr(SERVER_IP)：表示要连接指定服务器 IP

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // <4>: connect 连接服务器
    int ret = connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr));

    if (ret == -1) {
        cout << "connect server error" << endl;
        close(client_fd);
        return 1;
    }

    cout << "connected to server" << endl;

    // 连接成功后，用户输入名称（不能为空）
    string nickname;

    while (true) {
        cout << "请输入昵称: ";
        getline(cin, nickname);

        if (!nickname.empty()) {
            break;
        }

        cout << "昵称不能为空" << endl;
    }

    //把昵称发送给服务端
    string login_msg = "login|" + nickname;

    if (!sendMessage(client_fd, login_msg)) {
        cout << "send nickname failed" << endl;
        close(client_fd);
        return 1;
    }
    
    //<5>: send 发送消息
    string message;
    

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client_fd, &read_fds);

        int max_fd = client_fd;

        int ret = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (ret == -1) {
            cout << "select error" << endl;
            break;
        }

        // 1. 键盘有输入：读取用户输入并 send
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            getline(cin, message);

            if (message == "quit") {
                cout << "退出发送消息" << endl;
                break;
            }

            if (message.empty()) {
                cout << "请输入至少一个字符文本" << endl;
                continue;
            }

            string chat_msg = "chat|" + message;

            if (!sendMessage(client_fd, chat_msg)) {
                cout << "send message failed" << endl;
                break;
            }

            cout << "sent message: " << message << endl;
        }

        // 2. 服务器 socket 有数据：接收广播消息
        if (FD_ISSET(client_fd, &read_fds)) {
            string received_msg;

            if (recvMessage(client_fd, received_msg)) {
                cout << "received message: " << received_msg << endl;
            }
            else {
                cout << "server disconnected or receive message failed" << endl;
                break;
            }
        }
    }


    close(client_fd);
    return 0;
}