#include <unistd.h>
#include "khash.h"

#ifndef __TYPES_H
#define __TYPES_H

typedef struct {
    int epoll_fd;
    char name[10];
    pid_t pid;
    void *zmq_context;
    void *master;
} rio_worker;

typedef struct {
    int server_fd;
    rio_worker **workers;
    void *zmq_context;
    void *publisher;
} rio_runtime;

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

