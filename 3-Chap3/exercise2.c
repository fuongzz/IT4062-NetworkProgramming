#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#define PORT 8081
#define BUF_SIZE 4096

// Xử lý lệnh "ls": tự liệt kê thư mục hiện tại bằng opendir/readdir/closedir
void handle_ls(int client_socket) {
    char response[BUF_SIZE];
    response[0] = '\0';

    DIR *dir = opendir(".");
    if (dir == NULL) {
        char *err = "Cannot open directory\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Bỏ qua thư mục hiện tại "." và thư mục cha ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Ghép tên file/thư mục vào chuỗi kết quả, cách nhau bởi khoảng trắng
        strncat(response, entry->d_name, sizeof(response) - strlen(response) - 2);
        strncat(response, " ", sizeof(response) - strlen(response) - 1);
    }
    closedir(dir);

    strncat(response, "\n", sizeof(response) - strlen(response) - 1);
    send(client_socket, response, strlen(response), 0);
}

// Xử lý lệnh "ifconfig": thực thi lệnh hệ thống và gửi toàn bộ output về client
void handle_ifconfig(int client_socket) {
    FILE *fp = popen("ifconfig", "r");
    if (fp == NULL) {
        char *err = "Cannot execute command\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        send(client_socket, line, strlen(line), 0);
    }
    pclose(fp);
}

// Xử lý một client: nhận command, phân tích, gọi hàm xử lý tương ứng
void handle_client(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received == -1) perror("recv");
        return;
    }
    buffer[bytes_received] = '\0';

    // Loại bỏ ký tự xuống dòng '\n' hoặc '\r' do client (nc) gửi kèm
    buffer[strcspn(buffer, "\r\n")] = '\0';

    printf("Received command: %s\n", buffer);

    if (strcmp(buffer, "ls") == 0) {
        handle_ls(client_socket);
    } else if (strcmp(buffer, "ifconfig") == 0) {
        handle_ifconfig(client_socket);
    } else {
        char *msg = "Invalid command\n";
        send(client_socket, msg, strlen(msg), 0);
    }
}

int main(){
    // Tạo Socket TCP
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ và cổng cho server socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Liên kết socket với địa chỉ và cổng
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listener);
        exit(EXIT_FAILURE);
    }
    // Lắng nghe kết nối từ client
    if (listen(listener, 5) == -1) {
        perror("listen");
        close(listener);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // Vòng lặp chính: lần lượt chấp nhận và xử lý từng client
    while (1) {
        int client_socket = accept(listener, NULL, NULL);
        if (client_socket == -1) {
            perror("accept");
            continue; // tiếp tục chờ client khác thay vì thoát chương trình
        }
        printf("Client connected!: %d\n", client_socket);

        handle_client(client_socket);

        close(client_socket);
        printf("Client disconnected: %d\n", client_socket);
    }

    close(listener);
    return 0;
}
