#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>      // close()
#include <arpa/inet.h>   // htons(), inet_ntoa(), htonl(), ntohl()
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), recv(), send()
#include <sys/select.h>  // select(), fd_set, FD_SET, FD_ISSET
#include <map>
#include <string>
#include <cstdint>
#include <cstdlib>
#include "../Common/Protocol.h"

using namespace std;

// recvAll() 用来解决 TCP 半包问题。
// recv() 不保证一次就能读满指定长度，所以这里循环读取，直到收满 length 字节。
// 
int main(int argc, char* argv[]) {
    int PORT = 8888;

    if (argc >= 2) {
        PORT = atoi(argv[1]);
    }   

    // 创建 TCP 监听 socket。
    // AF_INET 使用 IPv4。
    // SOCK_STREAM 表示面向连接的字节流 socket，通常对应 TCP。
    // 第三个参数 0 表示使用默认协议。
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        cout << "create server error" << endl;
        return 1;
    }

    // 允许服务端重启后快速复用同一个端口，避免频繁出现 bind error。
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        cout << "setsockopt error" << endl;
        close(server_fd);
        return 1;
    }

    // 准备服务器地址信息。
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // 监听本机所有网卡
    server_addr.sin_port = htons(PORT);         // 端口转换为网络字节序

    // bind()：把 server_fd 绑定到指定 IP 和端口。
    int ret = bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));

    if (ret == -1) {
        cout << "bind error" << endl;
        close(server_fd);
        return 1;
    }

    // listen()：让 socket 进入监听状态，等待客户端连接。
    // 第二个参数 backlog 表示连接等待队列长度。
    ret = listen(server_fd, 5);

    if (ret == -1) {
        cout << "listen error" << endl;
        close(server_fd);
        return 1;
    }

    cout << "server listening on port " << PORT << endl;

    // 保存所有已连接客户端的 socket。
    vector<int> clients;

    // 保存客户端昵称：client_fd -> nickname。
    map<int, string> nicknames;

    // select 模型：
    // 同时监听 server_fd 和所有 client_fd。
    // server_fd 可读：有新客户端连接，可以 accept()。
    // client_fd 可读：已有客户端发来消息或断开连接，可以 recvMessage()。
    while (true) {
        // fd_set 是 select 使用的文件描述符集合。
        // read_fds 保存本轮要监听“可读事件”的 fd。
        fd_set read_fds;

        // 每轮 select 前都要清空集合，再重新加入要监听的 fd。
        FD_ZERO(&read_fds);

        // 监听 socket 负责接收新客户端连接。
        FD_SET(server_fd, &read_fds);

        // select 的第一个参数需要传“最大 fd + 1”。
        // 先假设 server_fd 是当前最大 fd，后面再和所有 client_fd 比较。
        int max_fd = server_fd;

        // 把所有客户端 socket 加入 read_fds，并更新 max_fd。
        // 第一轮 clients 为空，因此只监听 server_fd。
        for (int client_fd : clients) {
            FD_SET(client_fd, &read_fds);

            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
        }

        // select() 会阻塞等待，直到某个 fd 变为可读。
        // 最后一个参数传 nullptr 表示没有超时时间，会一直等待。
        // 关于为什么需要 max_fd + 1，见笔记：为什么select不直接检查read_fds所有fd.txt
        ret = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (ret == -1) {
            cout << "select error" << endl;
            break;
        }

        // 处理新客户端连接。
        // 对监听 socket 来说，“可读”表示有新连接已经准备好，accept() 不会阻塞。
        if (FD_ISSET(server_fd, &read_fds)) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            // accept() 从连接队列取出一个新客户端，
            // 返回专门用于和该客户端通信的 client_fd。
            int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

            if (client_fd == -1) {
                cout << "accept error" << endl;
            }
            else {
                // 保存新客户端，后续 select 才能继续监听它。
                clients.push_back(client_fd);

                cout << "client connected" << endl;
                cout << "client ip: " << inet_ntoa(client_addr.sin_addr) << endl;
                cout << "online clients: " << clients.size() << endl;
            }
        }

        // 处理已有客户端消息。
        // 哪个 client_fd 可读，就对哪个客户端 recvMessage()。
        for (auto it = clients.begin(); it != clients.end(); ) {
            int client_fd = *it;

            if (FD_ISSET(client_fd, &read_fds)) {
                string text;

                if (recvMessage(client_fd, text)) {
                    string broadcast_msg;

                    // 协议格式：
                    // login|nickname 表示登录消息。
                    // chat|message 表示聊天消息。
                    if (text.rfind("login|", 0) == 0) {
                        string nickname = text.substr(6);

                        if (nickname.empty()) {
                            cout << "empty nickname ignored" << endl;
                            ++it;
                            continue;
                        }

                        nicknames[client_fd] = nickname;
                        broadcast_msg = nickname + " joined the chat";
                        cout << broadcast_msg << endl;
                    }
                    else if (text.rfind("chat|", 0) == 0) {
                        string content = text.substr(5);

                        if (nicknames.find(client_fd) == nicknames.end()) {
                            cout << "chat message from unauthenticated client ignored" << endl;
                            ++it;
                            continue;
                        }

                        if (content.empty()) {
                            cout << "empty chat message ignored" << endl;
                            ++it;
                            continue;
                        }

                        broadcast_msg = nicknames[client_fd] + ": " + content;
                        cout << broadcast_msg << endl;
                    }
                    else {
                        cout << "invalid message ignored: " << text << endl;
                        ++it;
                        continue;
                    }

                    // 群聊广播：使用 sendMessage() 转发给除发送者以外的所有在线客户端。
                    for (int other_fd : clients) {
                        if (other_fd != client_fd) {
                            sendMessage(other_fd, broadcast_msg);
                        }
                    }

                    ++it;
                }
                else {
                    // recvMessage() 返回 false：
                    // 可能是客户端断开，也可能是消息长度非法或接收失败。
                    auto name_it = nicknames.find(client_fd);

                    if (name_it != nicknames.end()) {
                        string leave_msg = name_it->second + " left the chat";

                        cout << leave_msg << endl;

                        for (int other_fd : clients) {
                            if (other_fd != client_fd) {
                                sendMessage(other_fd, leave_msg);
                            }
                        }

                        nicknames.erase(name_it);
                    }
                    else {
                        cout << "unauthenticated client disconnected" << endl;
                    }

                    close(client_fd);
                    it = clients.erase(it);

                    cout << "online clients: " << clients.size() << endl;
                }
            }
            else {
                ++it;
            }
        }
    }

    // 程序退出前关闭所有客户端 socket。
    for (int client_fd : clients) {
        close(client_fd);
    }

    close(server_fd);

    return 0;
}