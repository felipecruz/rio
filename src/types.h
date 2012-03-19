#include "khash.h"

#ifndef __TYPES_H
#define __TYPES_H

int server_fd;
int epollfd;

typedef struct {
    int fd;
    int keep_alive;
    char *buffer;
    char *path;
    unsigned char method;
} client;

KHASH_MAP_INIT_INT(clients, client)
khash_t(clients) *h;
#else
#endif

