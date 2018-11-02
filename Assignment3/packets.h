// Jakub Zadrozny 290920
#ifndef PACKETS
#define PACKETS

#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_DATA 1000

void resend (int start, int bytes);
int  fetch  (unsigned char **buf, int *start, int *size);

#endif
