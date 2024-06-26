#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <vector>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
#define PORT 8080

void set_nonblocking(int sock) {
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
}

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];
    char buffer[BUFFER_SIZE] = {0};

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定套接字到地址
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 创建epoll实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // 注册server_fd上的读事件
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd) {
                // 新连接到达
                struct sockaddr_in client_address;
                socklen_t addrlen = sizeof(client_address);
                if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address,
                                        (socklen_t*)&addrlen)) < 0) {
                    perror("accept");
                    continue;
                }
                
                set_nonblocking(new_socket);
                
                // 将新连接的socket加入epoll监控
                event.data.fd = new_socket;
                event.events = EPOLLIN | EPOLLET;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event) < 0) {
                    perror("epoll_ctl: add");
                    close(new_socket);
                }
            } else {
                // 处理已连接的socket上的数据
                valread = read(events[i].data.fd, buffer, BUFFER_SIZE);
                if (valread <= 0) {
                    // 如果读取失败或连接关闭，则从epoll中删除并关闭该socket
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &event);
                    close(events[i].data.fd);
                } else {
                    // 在这里处理接收到的数据，例如保存到文件
                    std::cout << "Received data on descriptor " << events[i].data.fd << std::endl;
                    // 注意：此处省略了处理数据并保存到文件的逻辑
                }
            }
        }
    }

    close(server_fd);
    return 0;
}