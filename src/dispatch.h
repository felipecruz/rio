#include <zmq.h>
#include <msgpack.h>
#include <sys/epoll.h>
#include "types.h"

#ifndef __DISPATCH_H_
#define __DISPATCH_H_

#define DISPATCH_FINISHED 1
#define DISPATCH_LATER 2
#define DISPATCH_AGAIN 3

int
    dispatch(rio_client *cli, char *path);

void
    dispatch_responses(rio_worker *worker);

void
    init_dispatcher(void);

void
    destroy_dispatcher(void);

int
    is_filename(char *path);
#else
#endif

#if TEST
int
    init_dispatch_test_suite(void);

void
    test_is_filename(void);

#endif
