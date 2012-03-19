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

#define MAX_EVENTS 128
#ifndef __NETWORK_H
#define __NETWORK_H
int 
    run(int argc, char **args);
#else
#endif
