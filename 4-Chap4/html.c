#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>

void Append(char** output, const char* str)
{
    char* tmp = *output;
    int oldlen = tmp == NULL ? 0 : strlen(tmp);
    int newlen = oldlen + strlen(str) + 1;
    tmp = realloc(tmp, newlen);
    tmp[newlen - 1] = 0;
    sprintf(tmp + oldlen, "%s", str);
    *output = tmp;
}

void send_error(int client, int code, const char* msg){
    char* body = NULL;
    char line[128];
    snprintf(line, sizeof(line), "<h1>%d %s</h1>", code, msg);
    Append(&body, line);

    char* header = NULL;
    snprintf(line, sizeof(line), "HTTP/1.1 %d %s\r\n", code, msg);
    Append(&header, line);
    Append(&header, "Content-Type: text/html; charset=utf-8\r\n");
    Append(&header, "Connection: close\r\n");
    snprintf(line, sizeof(line), "Content-Length: %zu\r\n\r\n", strlen(body));
    Append(&header, line);

    send(client, header, strlen(header), 0);
    send(client, body, strlen(body), 0);

    free(header);
    free(body);
}

void list_directory(int client, const char* path, const char* uri){
    DIR* dir = opendir(path);
    if (dir == NULL){
        send_error(client, 404, "Not Found");
        return;
    }

    char* body = NULL;
    char line[1536];

    snprintf(line, sizeof(line), "<h1>Index of %s</h1><ul>", uri);
    Append(&body, line);

    int uri_has_slash = uri[strlen(uri) - 1] == '/';

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char href[1100];
        snprintf(href, sizeof(href), "%s%s%s", uri, uri_has_slash ? "" : "/", entry->d_name);

        if (entry->d_type == DT_DIR)
            snprintf(line, sizeof(line), "<li>[DIR] <a href=\"%s\">%s/</a></li>", href, entry->d_name);
        else
            snprintf(line, sizeof(line), "<li>[FILE] <a href=\"%s\">%s</a></li>", href, entry->d_name);

        Append(&body, line);
    }
    closedir(dir);

    Append(&body, "</ul>");

    char* header = NULL;
    Append(&header, "HTTP/1.1 200 OK\r\n");
    Append(&header, "Content-Type: text/html; charset=utf-8\r\n");
    Append(&header, "Connection: close\r\n");
    snprintf(line, sizeof(line), "Content-Length: %zu\r\n\r\n", strlen(body));
    Append(&header, line);

    send(client, header, strlen(header), 0);
    send(client, body, strlen(body), 0);

    free(header);
    free(body);
}

void send_file(int client, const char* path){
    FILE* fp = fopen(path, "rb");
    if (fp == NULL){
        send_error(client, 404, "Not Found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    const char* filename = strrchr(path, '/');
    filename = filename != NULL ? filename + 1 : path;

    char* header = NULL;
    char line[256];
    Append(&header, "HTTP/1.1 200 OK\r\n");
    Append(&header, "Content-Type: application/octet-stream\r\n");
    snprintf(line, sizeof(line), "Content-Disposition: attachment; filename=\"%s\"\r\n", filename);
    Append(&header, line);
    snprintf(line, sizeof(line), "Content-Length: %ld\r\n", filesize);
    Append(&header, line);
    Append(&header, "Connection: close\r\n\r\n");

    send(client, header, strlen(header), 0);
    free(header);

    char filebuf[4096];
    size_t n;
    while ((n = fread(filebuf, 1, sizeof(filebuf), fp)) > 0){
        send(client, filebuf, n, 0);
    }

    fclose(fp);
}

int main(){
    int listener = socket(
        AF_INET, 
        SOCK_STREAM,
        IPPROTO_TCP
    );

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    if (bind(
            listener, 
            (struct sockaddr*)&addr, 
            sizeof(addr)
        ) == -1
    ){
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5) == -1){
        perror("listen() failed");
        return 1;
    }

    printf("Server is listening on http://localhost:8080/\n");

    char buffer[2048];

    while (0 == 0){
        int client = accept(
            listener, 
            NULL, 
            NULL
        );

        if (client == -1){
            perror("accept() failed");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int byte_received = recv(
            client, 
            buffer, 
            sizeof(buffer) - 1, 
            0
        );

        if (byte_received <= 0){
            close(client);
            continue;
        }

        char method[16] = {0};
        char uri[1024] = {0};
        sscanf(buffer, "%15s %1023s", method, uri);

        printf("%s %s\n", method, uri);

        if (strcmp(method, "GET") != 0){
            send_error(client, 400, "Bad Request");
            close(client);
            continue;
        }

        if (strstr(uri, "..") != NULL){
            send_error(client, 404, "Not Found");
            close(client);
            continue;
        }

        char path[1040];
        if (strcmp(uri, "/") == 0)
            snprintf(path, sizeof(path), ".");
        else
            snprintf(path, sizeof(path), ".%s", uri);

        struct stat st;
        if (stat(path, &st) == -1){
            send_error(client, 404, "Not Found");
        } else if (S_ISDIR(st.st_mode)){
            list_directory(client, path, uri);
        } else {
            send_file(client, path);
        }

        close(client);
    }

}
