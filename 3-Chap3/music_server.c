#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BLOCK_SIZE (1024 * 10)
#define DIR_PATH "./server_playlist"
#define END_OF_LIST "END_OF_LIST"
#define END_OF_FILE "END_OF_FILE"

// Gửi một thông báo lỗi về cho client
void send_error(int sock, struct sockaddr_in *client, socklen_t client_len, const char *msg)
{
    char error_msg[1024];
    int len = snprintf(error_msg, sizeof(error_msg), "ERROR %s", msg);
    sendto(sock, error_msg, len, 0, (struct sockaddr *)client, client_len);
}

int main()
{
    // Tạo Socket UDP
    int sender_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ và cổng cho socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = INADDR_ANY;
    // Thiết lập cấu trúc lưu địa chỉ socket của client
    struct sockaddr_in client_socket;
    socklen_t client_socket_len = sizeof(client_socket);

    // Liên kết socket với địa chỉ và cổng
    if (bind(sender_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(sender_socket);
        exit(EXIT_FAILURE);
    }

    printf("UDP Music Server is listening on port %d...\n", ntohs(addr.sin_port));
    printf("Playlist directory: %s\n", DIR_PATH);

    char buffer[1024];
    char *data = malloc(BLOCK_SIZE);
    if (data == NULL)
    {
        perror("malloc");
        close(sender_socket);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Nhận yêu cầu từ client
        memset(buffer, 0, sizeof(buffer));
        int ret = recvfrom(sender_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_socket, &client_socket_len);
        if (ret == -1)
        {
            perror("recvfrom");
            continue;
        }
        buffer[ret] = 0;

        // Bỏ ký tự xuống dòng ở cuối (telnet gửi \r\n)
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\r' || buffer[len - 1] == '\n'))
        {
            buffer[--len] = 0;
        }

        printf("Received command from %s:%d -> %s\n",
               inet_ntoa(client_socket.sin_addr), ntohs(client_socket.sin_port), buffer);

        if (strcmp(buffer, "GETLIST") == 0)
        {
            DIR *dir = opendir(DIR_PATH);
            if (dir == NULL)
            {
                int err = errno; // Lưu lại ngay, vì perror có thể làm thay đổi errno
                perror("opendir");
                send_error(sender_socket, &client_socket, client_socket_len, strerror(err));
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_name[0] == '.')
                {
                    continue; // bỏ qua ".", ".." và file ẩn
                }

                // Chỉ liệt kê file thường, bỏ qua thư mục con
                char full_path[1024];
                struct stat st;
                snprintf(full_path, sizeof(full_path), "%s/%s", DIR_PATH, entry->d_name);
                if (stat(full_path, &st) == -1 || !S_ISREG(st.st_mode))
                {
                    continue;
                }

                // Gửi từng tên bài hát đến client
                char line[1024];
                int line_len = snprintf(line, sizeof(line), "%s", entry->d_name);
                if (sendto(sender_socket, line, line_len, 0, (struct sockaddr *)&client_socket, client_socket_len) == -1)
                {
                    perror("sendto");
                    break;
                }
            }
            closedir(dir);

            sendto(sender_socket, END_OF_LIST, strlen(END_OF_LIST), 0, (struct sockaddr *)&client_socket, client_socket_len);
        }
        else if (strncmp(buffer, "DOWNLOAD ", 9) == 0)
        {
            char *song_name = buffer + 9;

            // Chặn truy cập ra ngoài thư mục playlist
            if (song_name[0] == 0 || strstr(song_name, "..") != NULL || strchr(song_name, '/') != NULL)
            {
                send_error(sender_socket, &client_socket, client_socket_len, "Invalid song name");
                continue;
            }

            char song_path[1024];
            snprintf(song_path, sizeof(song_path), "%s/%s", DIR_PATH, song_name);
            FILE *file = fopen(song_path, "rb");
            if (file == NULL)
            {
                int err = errno; // Lưu lại ngay, vì perror có thể làm thay đổi errno
                perror("fopen");
                send_error(sender_socket, &client_socket, client_socket_len, strerror(err));
                continue;
            }

            printf("Sending song: %s\n", song_name);
            size_t bytes_read;
            size_t total = 0;
            int block_count = 0;
            while ((bytes_read = fread(data, 1, BLOCK_SIZE, file)) > 0)
            {
                ssize_t bytes_sent = sendto(sender_socket, data, bytes_read, 0, (struct sockaddr *)&client_socket, client_socket_len);
                if (bytes_sent == -1)
                {
                    perror("sendto");
                    break;
                }
                total += bytes_read;
                block_count++;
            }
            fclose(file);

            sendto(sender_socket, END_OF_FILE, strlen(END_OF_FILE), 0, (struct sockaddr *)&client_socket, client_socket_len);
            printf("Sent %d block(s), %zu bytes\n", block_count, total);
        }
        else
        {
            printf("Unknown command: %s\n", buffer);
            char msg[1100];
            snprintf(msg, sizeof(msg), "Unknown command: %s", buffer);
            send_error(sender_socket, &client_socket, client_socket_len, msg);
        }
    }

    free(data);
    close(sender_socket);
    return 0;
}
