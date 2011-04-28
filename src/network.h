#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include "../http-parser/http_parser.h"
#include "utils.h"

#define MAX_EVENTS 1000
#ifndef __NETWORK_H
#define __NETWORK_H

enum methods { POST, GET };

typedef struct {
    int fd;
    char *buffer;
    int last;
    char *path;
    int state;
    int t;
    int offsets[100];
} client;

int run(int argc, char** args);

void handle_write(client *cli, char* resp);

#else
#endif
