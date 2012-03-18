#include <msgpack.h>
#include <zmq.h>
#include "types.h"

#ifndef __DISPATCH_H_
#define __DISPATCH_H_

char*
    dispatch(client *cli, char *path);

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
