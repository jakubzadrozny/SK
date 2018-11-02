// Jakub Zadrozny 290920
#include "handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void handle_error(char* msg, bool is_errno) {
    fprintf(stderr, "%s\n", msg);
    if (is_errno) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
    exit(EXIT_FAILURE);
}
