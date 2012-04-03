#include <sys/types.h>
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
    void* content;
    size_t length;
    int where;
    int read_count;
} rio_buffer;

typedef struct {
    int fd;
    int keep_alive;
    char *path;
    unsigned char method;
    rio_buffer *buffer;
} rio_client;

KHASH_MAP_INIT_INT(clients, rio_client)
khash_t(clients) *h;
#else
#endif
