#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define BLOCK_SIZE (1024 * 10)
#define END_OF_LIST "END_OF_LIST"
#define END_OF_FILE "END_OF_FILE"

int main()
{
    // Tạo Socket UDP
    int sender_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Hết 3 giây không nhận được gói nào thì coi như mất kết nối
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sender_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Thiết lập địa chỉ và cổng của server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char command[1024];
    char song_name[1024];
    // Buffer nhận phải đủ lớn cho một block, nếu không datagram sẽ bị cắt
    char *buffer = malloc(BLOCK_SIZE + 1);
    if (buffer == NULL)
    {
        perror("malloc");
        close(sender_socket);
        exit(EXIT_FAILURE);
    }

    printf("Commands: GETLIST | DOWNLOAD <ten_bai_hat> | QUIT\n");
    while (1)
    {
        printf("\n> ");
        fflush(stdout);
        memset(command, 0, sizeof(command));
        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }
        command[strcspn(command, "\r\n")] = 0; // Bỏ ký tự xuống dòng

        if (command[0] == 0)
        {
            continue;
        }
        if (strcmp(command, "QUIT") == 0)
        {
            break;
        }

        // Gửi yêu cầu đến server
        if (sendto(sender_socket, command, strlen(command), 0, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("sendto");
            break;
        }

        struct sockaddr_in server_addr;
        socklen_t server_addr_len;

        if (strcmp(command, "GETLIST") == 0)
        {
            printf("--- Danh sach bai hat ---\n");
            while (1)
            {
                server_addr_len = sizeof(server_addr);
                ssize_t bytes_received = recvfrom(sender_socket, buffer, BLOCK_SIZE, 0,
                                                 (struct sockaddr *)&server_addr, &server_addr_len);
                if (bytes_received == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        printf("Timeout: khong nhan duoc phan hoi tu server\n");
                    }
                    else
                    {
                        perror("recvfrom");
                    }
                    break;
                }
                buffer[bytes_received] = 0;

                if (strcmp(buffer, END_OF_LIST) == 0)
                {
                    break;
                }
                if (strncmp(buffer, "ERROR ", 6) == 0)
                {
                    printf("Server error: %s\n", buffer + 6);
                    break;
                }
                printf("%s\n", buffer);
            }
            printf("-------------------------\n");
        }
        else if (strncmp(command, "DOWNLOAD ", 9) == 0)
        {
            // Sao chép tên file ra biến riêng, vì buffer nhận sẽ bị ghi đè
            snprintf(song_name, sizeof(song_name), "%s", command + 9);
            if (song_name[0] == 0)
            {
                printf("Thieu ten bai hat\n");
                continue;
            }

            FILE *file = fopen(song_name, "wb");
            if (file == NULL)
            {
                perror("fopen");
                continue;
            }

            size_t total = 0;
            int block_count = 0;
            int ok = 0;
            while (1)
            {
                server_addr_len = sizeof(server_addr);
                ssize_t bytes_received = recvfrom(sender_socket, buffer, BLOCK_SIZE, 0,
                                                 (struct sockaddr *)&server_addr, &server_addr_len);
                if (bytes_received == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        printf("Timeout: mat ket noi khi dang tai\n");
                    }
                    else
                    {
                        perror("recvfrom");
                    }
                    break;
                }

                // Kiểm tra gói điều khiển TRƯỚC khi ghi, để không lẫn vào file audio
                if (bytes_received == (ssize_t)strlen(END_OF_FILE) &&
                    memcmp(buffer, END_OF_FILE, bytes_received) == 0)
                {
                    ok = 1;
                    break;
                }
                if (bytes_received > 6 && memcmp(buffer, "ERROR ", 6) == 0)
                {
                    buffer[bytes_received] = 0;
                    printf("Server error: %s\n", buffer + 6);
                    break;
                }

                fwrite(buffer, 1, bytes_received, file);
                total += bytes_received;
                block_count++;
            }
            fclose(file);

            if (ok)
            {
                printf("Download complete: %s (%d block, %zu bytes)\n", song_name, block_count, total);
            }
            else
            {
                remove(song_name); // File tải dở, bỏ đi
                printf("Download failed: %s\n", song_name);
            }
        }
        else
        {
            // Lệnh không hợp lệ, in phản hồi lỗi của server
            server_addr_len = sizeof(server_addr);
            ssize_t bytes_received = recvfrom(sender_socket, buffer, BLOCK_SIZE, 0,
                                             (struct sockaddr *)&server_addr, &server_addr_len);
            if (bytes_received > 0)
            {
                buffer[bytes_received] = 0;
                printf("%s\n", buffer);
            }
        }
    }

    free(buffer);
    close(sender_socket);
    return 0;
}
