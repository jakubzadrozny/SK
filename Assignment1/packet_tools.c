// Jakub Zadrozny 290920
#include "packet_tools.h"
#include "handler.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>

int pid;
int seq = 0;
struct sockaddr_in sender;
u_int8_t buffer[IP_MAXPACKET];

u_int16_t compute_icmp_checksum (const void *buff, int length) {
	u_int32_t sum;
	const u_int16_t* ptr = buff;
	assert (length % 2 == 0);
	for (sum = 0; length > 0; length -= 2)
		sum += *ptr++;
	sum = (sum >> 16) + (sum & 0xffff);
	return (u_int16_t)(~(sum + (sum >> 16)));
}

int send_icmp(int sockfd, char* target, int ttl) {
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = pid;
    icmp_header.un.echo.sequence = htons(seq);
    icmp_header.checksum = 0;
    icmp_header.checksum = compute_icmp_checksum((u_int16_t*)&icmp_header, sizeof(icmp_header));

    struct sockaddr_in recipient;
    bzero (&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton(AF_INET, target, &recipient.sin_addr);

    if(setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int))) {
		handle_error("socket error", true);
	}

    ssize_t bytes_sent = sendto(sockfd, &icmp_header, sizeof(icmp_header), 0,
        (struct sockaddr*)&recipient, sizeof(recipient));

	if (bytes_sent < 0) {
		handle_error("socket error", true);
	}

	return seq++;
}

ssize_t receive_packet (int sockfd, icmp_packet** packet) {
    socklen_t sender_len = sizeof(sender);
    ssize_t packet_len = recvfrom ( sockfd,
                                    buffer,
                                    IP_MAXPACKET,
                                    MSG_DONTWAIT,
                                    (struct sockaddr*)&sender,
                                    &sender_len);
    if(packet_len > 0) {
        parse_packet(buffer, packet);
    }
    return packet_len;
}

int parse_packet(u_int8_t* buffer, icmp_packet** packet_ptr) {
    *packet_ptr = (icmp_packet*) malloc(sizeof(icmp_packet));
    icmp_packet* packet = *packet_ptr;

    gettimeofday(&(packet -> received), NULL);

    packet -> ip_header = (struct iphdr*) buffer;
    ssize_t header_len = 4 * packet -> ip_header -> ihl;

    packet -> icmp_header = (struct icmphdr*) (buffer + header_len);
    packet -> icmp_content = buffer + header_len + ICMP_HEADER_BYTES;

    inet_ntop(AF_INET, &(packet -> ip_header -> saddr), packet -> source_ip, INET_ADDRSTRLEN);

    return packet -> ip_header -> protocol == IPPROTO_ICMP;
}
