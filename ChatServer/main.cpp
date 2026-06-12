#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>		//                  提供 close()
#include <arpa/inet.h>	//	arpa / inet.h   提供 htons(), inet_addr() 等 IP / 端口转换函数	
#include <sys/socket.h> //  sys / socket.h  提供 socket(), bind(), listen(), accept(), recv()

using namespace std;

int main() {
	const int PORT = 8888;

	//AF_INET       使用 IPv4
	//SOCK_STREAM   使用 TCP
	//0             使用默认协议
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd == -1) {
		cout << "create server error" << endl;
		return 1;
	}

	//创建一个 IPv4 地址结构体
	//把里面的内容全部清零，避免有脏数据
	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	//sin_family          地址类型，AF_INET 表示 IPv4
	//sin_addr.s_addr     监听哪个 IP，INADDR_ANY 表示本机所有网卡
	//sin_port            监听哪个端口，htons(PORT) 表示把端口转成网络字节序
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	//使用bind() 把 server_fd 这个 socket 绑定到 8888 端口
	int ret = bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));

	if (ret == -1) {
		cout << "bind error" << endl;
		close(server_fd);
		return 1;
	}

	//listen() 的作用就是：让这个 socket 从普通 socket 变成监听 socket，开始等待客户端连接
	//这里的 5 叫 backlog，可以先理解成：
	//    客户端连接排队队列的长度
	//    比如某一瞬间有多个客户端同时连接服务器，服务器还没来得及 accept()，这些连接就会先排队。5 表示队列长度给 5 个左右。可以设置成6，7，8...
	//	  
	ret = listen(server_fd, 5);

	if (ret == -1) {
		cout << "listen error" << endl;
		close(server_fd);
		return 1;
	}

	cout << "server listening on port " << PORT << endl;

	

	//server_fd 继续负责监听新的客户端
	//client_fd 负责和这一个客户端聊天
	//循环接受多个客户端

	// ---------------------------------------------------------------------
	//当前服务端是阻塞式单客户端处理模型。
	//当服务端进入某个客户端的 recv() 后，无法继续 accept 或处理其他客户端。
	//因此多个客户端同时连接时，后连接的客户端必须等待前一个客户端断开。
	//这就是后续引入 select 的原因。
	// ---------------------------------------------------------------------
	while (1) {
		//client_addr 用来保存连接进来的客户端 IP 和端口
		//client_len 表示 client_addr 这个结构体的大小
		sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

		if (client_fd == -1) {
			cout << "accept error" << endl;
			continue;
		}

		cout << "client connected" << endl;
		//打印客户端 IP
		cout << "client ip: " << inet_ntoa(client_addr.sin_addr) << endl;

		//准备缓冲区
		//不用vector的写法
		//char buffer[1024];
		//memset(buffer, 0, sizeof(buffer));

		vector<char> buffer(1024);

		//recv(): 客户端连接后，服务器接收客户端发来的一句话并打印出来
		//这里 sizeof(buffer) - 1 是为了给字符串结尾的 '\0' 留一个位置。
		//循环接收消息
		while (1) {
			int bytes_received = recv(client_fd, buffer.data(), buffer.size() - 1, 0);

			//详情见：notes/recv缓冲区与字符串结束符学习笔记.txt
			if (bytes_received > 0) {
				buffer[bytes_received] = '\0';
				cout << "message: " << buffer.data() << endl;
			}
			else if (bytes_received == 0) {
				//TCP 空消息与 recv 返回值短笔记.txt
				cout << "client disconnected" << endl;
				break;
			}
			else {
				cout << "receive message failed" << endl;
				break;
			}
		}
		close(client_fd);
	}
	
	close(server_fd);
	
	return 0;
}