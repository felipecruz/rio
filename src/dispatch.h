#include "network.h"

#ifndef __DISPATCH_H_
#define __DISPATCH_H_

char*
    dispatch(client *cli, const char *path);

void
    init_dispatcher(void);
#else
#endif

#if TEST
int
    init_dispatch_test_suite(void);

void
    test_is_filename(void);

#endif
