#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "packets.h"
#include "handler.h"

extern int sockfd;

unsigned char       packet[2 * MAX_DATA];
struct sockaddr_in  sender;
struct sockaddr_in  addr;
socklen_t           addr_len = sizeof(addr);
socklen_t           sender_len = sizeof(sender);

void resend (int start, int bytes) {
    int len = sprintf((char*) packet, "GET %d %d\n", start, bytes);
    if (sendto(sockfd, packet, len, 0, (struct sockaddr*) &addr, addr_len) < 0) {
        handle_error("socket error", true);
    }
}

int fetch  (unsigned char **buf, int *start, int *size) {
    bzero(&sender, sender_len);
    ssize_t dg_len = recvfrom(sockfd, packet, 2*MAX_DATA, MSG_DONTWAIT,
        (struct sockaddr*) &sender, &sender_len);
    if(dg_len < 0) {
        if(errno == EWOULDBLOCK) {
            return -1;
        } else {
            handle_error("socket error", true);
        }
    }

    if (sender.sin_addr.s_addr != addr.sin_addr.s_addr || sender.sin_port != addr.sin_port) {
        return 0;
    }

    *buf = packet;
    sscanf((char*) packet, "%*s %d %d", start, size);
    return (int) dg_len;
}
