#include "utils.h"
#include "handler.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

int getDataTillX (int fd, char *buffer, int buffer_size, int timeout) {
	struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

	bool found = false;
	char *buff_ptr = buffer;
	int total_bytes_read = 0;

	while (total_bytes_read < buffer_size) {
		fd_set descriptors;
		FD_ZERO (&descriptors);
		FD_SET (fd, &descriptors);
		int ready = select(fd+1, &descriptors, NULL, NULL, &tv);
		if (!ready) return 0;

		int bytes_read = recv(fd, buff_ptr, buffer_size - total_bytes_read, 0);
		if (bytes_read <= 0) return bytes_read;
		char* new_buff_ptr = buff_ptr + bytes_read;
		for (; buff_ptr < new_buff_ptr; buff_ptr++) {
            if (buff_ptr != buffer && *(buff_ptr - 1) == '\r' && *buff_ptr == '\n') {
				found = true;
            }
        }

		total_bytes_read += bytes_read;
		if (found)
			return total_bytes_read;
	}
	return 0;
}

bool parse_request (int bytes, char *buffer, char *dir, char *file, char *host, bool *close_conn) {
    *close_conn = false;

    char arg[RESOURCE_SIZE];
    char val[RESOURCE_SIZE];
    if (sscanf(buffer, "GET %s", arg) < 1) {
        return false;
    }

    char *arg_ptr = arg;
    char *slash = arg;
    for(; *arg_ptr != '\0'; arg_ptr++) {
        if (*arg_ptr == '/') {
            slash = arg_ptr;
        }
    }
    strncpy(dir, arg, slash - arg + 1);
    dir[slash - arg + 1] = '\0';
    strncpy(file, slash + 1, arg_ptr - slash - 1);
    file[arg_ptr - slash - 1] = '\0';

    bool host_present = false;
    char *buffer_end = buffer + bytes;

    for (char *buffer_ptr = buffer; buffer_ptr < buffer_end; buffer_ptr++) {
        if (*buffer_ptr == '\n') {
            if (sscanf(buffer_ptr + 1, "%s %s", arg, val) == 2) {
                if (strcmp(arg, "Host:") == 0) {
                    char *val_ptr = val;
                    for (; *val_ptr != '\0' && *val_ptr != ':'; val_ptr++) { }
                    int len = val_ptr - val;
                    strncpy(host, val, len);
                    host[len] = '\0';
                    host_present = true;
                } else if (strcmp(arg, "Connection:") == 0) {
                    if (strcmp(val, "close") == 0) {
                        *close_conn = true;
                    }
                }
            }
        }
    }

    return host_present;
}

void get_type (char *file, char *type) {
    char *dot = strrchr(file, 46) + 1;
    if (!dot) {
        strcpy(type, "application/octet-stream");
        return;
    }

    if (strcmp(dot, "txt") == 0) {
        strcpy(type, "text/plain");
    } else if (strcmp(dot, "html") == 0) {
        strcpy(type, "text/html");
    } else if (strcmp(dot, "css") == 0) {
        strcpy(type, "text/css");
    } else if (strcmp(dot, "js") == 0) {
        strcpy(type, "text/javascript");
    } else if (strcmp(dot, "jpg") == 0) {
        strcpy(type, "image/jpeg");
    } else if (strcmp(dot, "jpeg") == 0) {
        strcpy(type, "image/jpeg");
    } else if (strcmp(dot, "png") == 0) {
        strcpy(type, "image/png");
    } else if (strcmp(dot, "pdf") == 0) {
        strcpy(type, "application/pdf");
    } else {
        strcpy(type, "application/octet-stream");
    }
}
