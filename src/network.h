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
#include "types.h"
#include "utils.h"
#include "dispatch.h"
#include "static.h"
#include "buffer.h"

#define MAX_EVENTS 4096
#ifndef __NETWORK_H
#define __NETWORK_H
int 
    socket_bind(void);

void
    run_worker(int id, rio_worker* worker, rio_runtime *runtime);

void
    run_master(rio_runtime *runtime);

int
    remove_and_close(rio_client *client,
                     rio_worker *worker,
                     struct epoll_event *event);
#else
#endif
