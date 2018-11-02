// Jakub Zadrozny 290920
#include "traceroute.h"
#include "packet_tools.h"
#include "handler.h"

#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>

extern const int WAIT_FOR_REPLY;
extern const int PACKETS_PER_STEP;
extern int pid;

#define MAXSEQ 500

bool received[MAXSEQ];

int tv_to_ms(struct timeval* tv) {
    return tv -> tv_sec * 1000 + (int) tv -> tv_usec / 1000;
}

bool is_valid_ip_address(char* address) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, address, &(sa.sin_addr));
    return result != 0;
}

int response_type(icmp_packet* packet, int seq_id) {
    if (packet -> icmp_header -> type == ICMP_TIME_EXCEEDED) {
        icmp_packet* rejected;
        if (!parse_packet(packet -> icmp_content, &rejected)) {
            free(rejected);
            return 0;
        }

        int seq_num = ntohs(rejected -> icmp_header -> un.echo.sequence);
        if (rejected -> icmp_header -> un.echo.id == pid &&
            seq_num >= seq_id && received[seq_num] == false) {
            received[seq_num] = true;
            free(rejected);
            return 1;
        }

        free(rejected);
        return 0;
    }

    if (packet -> icmp_header -> type == ICMP_ECHOREPLY) {
        int seq_num = ntohs(packet -> icmp_header -> un.echo.sequence);
        if (packet -> icmp_header -> un.echo.id == pid &&
            seq_num >= seq_id && received[seq_num] == false) {
            received[seq_num] = true;
            return 2;
        }
    }

    return 0;
}

int wait_for_answer(int sockfd, int seq_id, reply* r) {
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_ZERO (&descriptors);
    FD_SET (sockfd, &descriptors);

    r -> received = 0;
    r -> is_target = true;
    r -> rtt = 0;

    struct timeval start, add, end, left, curr;
    gettimeofday(&start, NULL);

    add.tv_sec = WAIT_FOR_REPLY;
    add.tv_usec = 0;
    timeradd(&start, &add, &end);
    timersub(&end, &start, &left);

    while (timerisset(&left)) {
        int ready = select (sockfd+1, &descriptors, NULL, NULL, &left);
        if (ready < 0) {
            handle_error("select error", true);
        } else if (ready == 0) {
            return r -> received;
        }

        while (true) {
            icmp_packet* packet;
            int res = receive_packet(sockfd, &packet);
            if(res < 0) {
                if(errno == EWOULDBLOCK) {
                    break;
                } else {
                    handle_error("socket error", true);
                }
            }

            int rtype = response_type(packet, seq_id);
            if(rtype > 0) {
                strcpy(r -> address[r -> received], packet -> source_ip);
                timersub(&(packet -> received), &start, &curr);
                r -> rtt += tv_to_ms(&curr);
                r -> is_target = r -> is_target & (rtype == 2);
                r -> received += 1;
            }

            free(packet);
            if(r -> received == PACKETS_PER_STEP) {
                return r -> received;
            }
        }

        gettimeofday(&curr, NULL);
        timersub(&end, &curr, &left);
    }

    return -1;
}

void print_reply(reply* r, int ttl) {
    if(r -> received > 0) {
        printf("%d: %s", ttl, r -> address[0]);
        for(int i = 1; i < r -> received; i++) {
            bool unique = true;
            for(int j = 0; j < i; j++) {
                unique = unique & strcmp(r -> address[i], r -> address[j]);
                if(!unique)
                    break;
            }
            if(unique)
                printf(" %s", r -> address[i]);
        }
        if(r -> received == PACKETS_PER_STEP) {
            int ms = (int) r -> rtt / r -> received;
            printf(" %dms\n", ms);
        } else {
            printf(" ???\n");
        }
    } else {
        printf("%d: *\n", ttl);
    }
}
