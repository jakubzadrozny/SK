// Jakub Zadrozny 290920
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "packet_tools.h"
#include "traceroute.h"
#include "handler.h"

const int WAIT_FOR_REPLY = 1;
const int PACKETS_PER_STEP = 3;
#define MAX_TTL 30

extern int pid;

int main(int argc, char **argv) {
    char* target = argv[1];
    if(argc < 2 || !is_valid_ip_address(target)) {
        handle_error("Target is not a valid IPv4 address.\nEnter a valid address to proceed.", false);
    }

    pid = getpid();
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        handle_error("socket error", true);
	}

    reply r;
    for(int ttl=1; ttl <= MAX_TTL; ttl++) {
        int seq_id = send_icmp(sockfd, target, ttl);
        for(int j=1; j < PACKETS_PER_STEP; j++) {
            send_icmp(sockfd, target, ttl);
        }

        int received = wait_for_answer(sockfd, seq_id, &r);
        print_reply(&r, ttl);
        if(received > 0 && r.is_target) {
            break;
        }
    }

    return 0;
}
