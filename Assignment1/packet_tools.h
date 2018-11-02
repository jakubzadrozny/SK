// Jakub Zadrozny 290920
#ifndef PACKET_TOOLS
#define PACKET_TOOLS

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#define IP_ADDRLEN 16
#define ICMP_HEADER_BYTES 8

typedef struct icmp_packet {
    char source_ip[IP_ADDRLEN];
    struct iphdr* ip_header;
    struct icmphdr* icmp_header;
    u_int8_t* icmp_content;
    struct timeval received;
} icmp_packet;

u_int16_t compute_icmp_checksum (const void *buff, int length);
int send_icmp(int sockfd, char* target, int ttl);
int parse_packet(u_int8_t* buffer, icmp_packet** packet_ptr);
ssize_t receive_packet (int sockfd, icmp_packet** packet);

#endif
