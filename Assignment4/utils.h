// Jakub Zadrozny 290920
#ifndef UTILS
#define UTILS

#include <stdbool.h>

#define RESOURCE_SIZE 1000

int getDataTillX (int fd, char *buffer, int buffer_size, int timeout);
bool parse_request (int bytes, char *buffer, char *dir, char *file, char *host, bool *close_conn);
void get_type (char *file, char *type);

#endif
