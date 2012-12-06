#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include "khash.h"

#ifndef __TYPES_H
#define __TYPES_H
typedef struct {
    int epoll_fd;
    char name[10];
    void *zmq_context;
    void *master;
    pid_t pid;
} rio_worker;

typedef struct {
    int server_fd;
    void *zmq_context;
    void *publisher;
    rio_worker **workers;
} rio_runtime;

typedef struct {
    int where;
    int read_count;
    uint8_t *content;
    size_t length;
} rio_buffer;

typedef struct {
    int fd;
    int keep_alive;
    int websocket;
    int eagain;
    char *path;
    unsigned char method;
    rio_buffer *buffer;
} rio_client;

KHASH_MAP_INIT_INT(clients, rio_client)
khash_t(clients) *h;
#else
#endif
