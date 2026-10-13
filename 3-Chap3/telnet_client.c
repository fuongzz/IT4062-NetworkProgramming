#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define SOCKADDR_IN struct sockaddr_in
#define SOCKADDR struct sockaddr

int main(int argc, char* argv[])
{
    // Địa chỉ và cổng của telnet server
    char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int server_port = (argc > 2) ? atoi(argv[2]) : 8888;

    // Tạo Socket TCP
    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = inet_addr(server_ip);

    // Kết nối đến server
    if (connect(client_socket, (SOCKADDR*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("Connected to %s:%d\n", server_ip, server_port);

    char buffer[1024];

    // Nhận lời chào từ server
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';
    printf("%s", buffer);

    // Nhập lệnh từ bàn phím
    printf("Command: ");
    fflush(stdout);
    memset(buffer, 0, sizeof(buffer));
    if (fgets(buffer, sizeof(buffer) - 1, stdin) == NULL) {
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Loại bỏ ký tự xuống dòng ở cuối lệnh
    while (strlen(buffer) > 0 &&
           (buffer[strlen(buffer) - 1] == '\r' || buffer[strlen(buffer) - 1] == '\n')) {
        buffer[strlen(buffer) - 1] = 0;
    }
    if (strlen(buffer) == 0) {
        printf("Empty command\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Gửi lệnh đến server
    ssize_t bytes_sent = send(client_socket, buffer, strlen(buffer), 0);
    if (bytes_sent == -1) {
        perror("send");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("Sent: %ld (Bytes)\n", (long)bytes_sent);

    // Nhận kết quả thực thi lệnh cho đến khi server đóng kết nối
    printf("Result: \n");
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
    if (bytes_received == -1) {
        perror("recv");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    close(client_socket);
    return 0;
}
