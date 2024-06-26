#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>

#define BUFFER_SIZE 1024
#define PORT 8080

// 函数原型声明
ssize_t send_full(int socket, const void* buffer, size_t len, int flags);
bool send_file(const char* filename, int sockfd);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <file_to_send>\n";
        return 1;
    }
    
    const char* server_ip = argv[1];
    const char* filename = argv[2];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket creation failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sock);
        return 1;
    }

    if (!send_file(filename, sock)) {
        perror("File sending failed");
    }

    close(sock);
    return 0;
}

bool send_file(const char* filename, int sockfd) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        perror("Failed to open file");
        return false;
    }

    // 发送文件名
    std::string filename_str = filename;
    if (send_full(sockfd, filename_str.c_str(), filename_str.size(), 0) == -1) {
        return false;
    }

    // 发送文件内容
    char buffer[BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        ssize_t bytes_read = file.gcount();
        if (bytes_read == 0) break; // EOF reached
        if (send_full(sockfd, buffer, bytes_read, 0) == -1) {
            return false;
        }
    }

    file.close();
    return true;
}

// 发送全部数据的辅助函数
ssize_t send_full(int socket, const void* buffer, size_t len, int flags) {
    size_t bytes_sent = 0;
    const char* ptr = static_cast<const char*>(buffer);
    while (bytes_sent < len) {
        ssize_t ret = send(socket, ptr + bytes_sent, len - bytes_sent, flags);
        if (ret == -1) return -1;
        bytes_sent += ret;
    }
    return bytes_sent;
}