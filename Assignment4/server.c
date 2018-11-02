#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "handler.h"
#include "utils.h"

#define BUFFER_SIZE 10000000
#define RESOURCE_SIZE 1000
#define TIMEOUT 1

char buffer[BUFFER_SIZE];

int respond (char *path, char *dir, char *file, char *host, int port) {
    char full[RESOURCE_SIZE];
    char real[RESOURCE_SIZE];
    bzero(full, RESOURCE_SIZE);

    strcat(full, path);
    strcat(full, host);
    strcat(full, dir);
    strcat(full, file);

    if (!realpath(full, real)) {
        strcpy(buffer, "HTTP/1.1 404 Not Found\r\nContent-Length: 41\r\nContent-Type: text/plain\r\n\r\n");
        strcat(buffer, "The website you requested does not exist.");
        return strlen(buffer);
    }

    char domain_full[RESOURCE_SIZE];
    char domain_real[RESOURCE_SIZE];
    bzero(domain_full, RESOURCE_SIZE);

    strcat(domain_full, path);
    strcat(domain_full, host);
    realpath(domain_full, domain_real);
    if(strstr(real, domain_real) != real) {
        strcpy(buffer, "HTTP/1.1 403 Forbidden\r\nContent-Length: 45\r\nContent-Type: text/plain\r\n\r\n");
        strcat(buffer, "This website is not part of specified domain.");
        return strlen(buffer);
    }

    if (opendir(real)) {
        char port_str[10];
        sprintf(port_str, "%d", port);

        strcpy(buffer, "HTTP/1.1 301 Moved Permanently\r\nLocation: http://");
        strcat(buffer, host);
        strcat(buffer, ":");
        strcat(buffer, port_str);
        strcat(buffer, dir);

        int dirlen = strlen(dir);
        if (dir[dirlen-1] == '/') {
            strcat(buffer, "index.html");
        } else {
            strcat(buffer, "/index.html");
        }
        strcat(buffer, "\r\n\r\n");

        return strlen(buffer);
    }

    char type[RESOURCE_SIZE];
    bzero(type, RESOURCE_SIZE);
    get_type(file, type);

    int fd = open(real, O_RDONLY);
    if (fd < 0) {
        handle_error("file error", true);
    }
    int length = read(fd, buffer + RESOURCE_SIZE, BUFFER_SIZE - RESOURCE_SIZE);
    if (length < 0) {
        handle_error("file error", true);
    }
    char length_str[10];
    sprintf(length_str, "%d", length);

    strcpy(buffer, "HTTP/1.1 200 OK\r\nContent-Type: ");
    strcat(buffer, type);
    strcat(buffer, "\r\n");
    strcat(buffer, "Content-Length: ");
    strcat(buffer, length_str);
    strcat(buffer, "\r\n\r\n");

    int header = strlen(buffer);
    memmove(buffer + header, buffer + RESOURCE_SIZE, length);

    return header + length;
}


int main (int argc, char *argv[]) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        handle_error("socket error", true);
    }

    if (argc < 3) {
        handle_error("not enough arguments", false);
    }

    int port = atoi(argv[1]);
    if (port < 1025) {
        handle_error("port has to be at least 1025", false);
    }

    char *path = realpath(argv[2], NULL);
    if (!path || !opendir(path)) {
        handle_error("directory error", true);
    }
    strcat(path, "/");

    struct sockaddr_in server_address;
	bzero (&server_address, sizeof(server_address));
	server_address.sin_family      = AF_INET;
	server_address.sin_port        = htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind (sockfd, (struct sockaddr *)&server_address, sizeof(server_address))) {
        handle_error("bind error", true);
    }
	if (listen (sockfd, 64)) {
        handle_error("listen error", true);
    }

    while (true) {
        int conn_sockfd = accept(sockfd, NULL, NULL);
        if (conn_sockfd < 0) {
            handle_error("socket error", true);
        }

        while (true) {
            int bytes = getDataTillX(conn_sockfd, buffer, BUFFER_SIZE, TIMEOUT);
            if (bytes < 0) {
                handle_error("socket error", true);
            } else if (bytes == 0) {
                break;
            }

            char dir[RESOURCE_SIZE];
            char file[RESOURCE_SIZE];
            char host[RESOURCE_SIZE];
            bzero(dir, RESOURCE_SIZE);
            bzero(file, RESOURCE_SIZE);
            bzero(host, RESOURCE_SIZE);
            bool close_conn;

            int length;
            if (!parse_request(bytes, buffer, dir, file, host, &close_conn)) {
                strcpy(buffer, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 53\r\nContent-Type: text/plain\r\n\r\n");
                strcat(buffer, "Your requested was not understandable for the server.");
                length = strlen(buffer);
            } else {
                length = respond(path, dir, file, host, port);
            }

            int sent = send(conn_sockfd, buffer, length, 0);
            if (sent < 0) {
                handle_error("socket error", true);
            }

            if (close_conn) {
                break;
            }
        }

        close(conn_sockfd);
    }
}
