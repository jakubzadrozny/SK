// Jakub Zadrozny 290920
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "packets.h"
#include "handler.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

#define ADDRESS "156.17.4.30"
#define WINDOW 3000
#define TIMEOUT 200000
#define MAX_DATA 1000
#define MIL 1000000

unsigned char   buffer  [WINDOW][MAX_DATA];
bool            received[WINDOW];
int             timeout [WINDOW];

int size;
int bytes_received;
int filefd;
int sockfd;

extern struct sockaddr_in addr;

void read_packets (int lfr) {
    unsigned char *buf;
    int start, len, num, pos, result;
    while(true) {
        result = fetch(&buf, &start, &len);

        if (result < 0) {
            return;
        } else if (result == 0) {
            continue;
        }

        num = start / MAX_DATA;
        pos = num % WINDOW;
        if (num < lfr || received[pos]) {
            continue;
        }
        // pakiety z przyszlosci?

        memcpy(buffer[pos], &buf[result - len], len);
        received[pos] = true;
        
        bytes_received += len;
        printf("%d%% done\n", (int) 100 * bytes_received / size);
    }
}

int get_wait (int lfr, int sub) {
    int wait = TIMEOUT;
    for (size_t i = 0; i < WINDOW; i++) {
        int start = (int) (lfr + i) * MAX_DATA;
        int bytes = min(MAX_DATA, size - start);
        if (bytes <= 0) {
            break;
        }

        int pos = (lfr + i) % WINDOW;
        if (received[pos]) {
            continue;
        }
        timeout[pos] -= sub;
        if (timeout[pos] <= 0) {
            resend(start, bytes);
            timeout[pos] = TIMEOUT;
        }

        wait = min(wait, timeout[pos]);
    }
    return wait;
}

void write_data (int lfr) {
    int pos = lfr % WINDOW;
    int len = min(MAX_DATA, size - lfr * MAX_DATA);

    if (write(filefd, buffer[pos], len) < 0) {
        handle_error("file error", true);
    }

    received[pos] = false;
    timeout[pos] = 0;
}

int main (int argc, char *argv[]) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        handle_error("socket error", true);
	}

    if (argc < 4) {
        handle_error("not enough arguments", false);
    }

    size = atoi(argv[3]);
    int         port = atoi(argv[1]);
    const char *name = argv[2];
    if (!size || !port) {
        handle_error("both port and size have to be positive", false);
    }

    filefd = open(name, O_WRONLY | O_CREAT | O_TRUNC);
    if (filefd < 0) {
        handle_error("file error", true);
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ADDRESS, &(addr.sin_addr));

    get_wait(0, 0);

    int lfr = 0;
    int elapsed = 0;
    while (lfr * MAX_DATA < size) {
        if (received[lfr % WINDOW]) {
            write_data(lfr++);
        } else {
            int t = get_wait(lfr, elapsed);
            struct timeval left, start, end, duration;
            left.tv_sec = t / MIL;
            left.tv_usec = t % MIL;

            fd_set descriptors;
            FD_ZERO (&descriptors);
            FD_SET (sockfd, &descriptors);

            gettimeofday(&start, NULL);

            int ready = select (sockfd+1, &descriptors, NULL, NULL, &left);

            gettimeofday(&end, NULL);
            timersub(&end, &start, &duration);

            elapsed = duration.tv_sec * MIL + duration.tv_usec;

            if (ready > 0) {
                read_packets(lfr);
            } else if (ready < 0) {
                handle_error("socket error", true);
            }
        }
    }
}
