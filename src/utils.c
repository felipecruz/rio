#include "utils.h"

void error_exit(const char *message) {
    perror(message);
    exit(-1);
}
