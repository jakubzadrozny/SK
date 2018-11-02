// Jakub Zadrozny 290920
#ifndef TRACEROUTE
#define TRACEROUTE

#include <stdbool.h>

typedef struct reply {
    int received;
    bool is_target;
    char address[3][16];
    int rtt;
} reply;

bool is_valid_ip_address(char* address);
int wait_for_answer(int sockfd, int seq_id, reply* r);
void print_reply(reply* r, int ttl);

#endif
