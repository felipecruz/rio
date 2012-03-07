#define _GNU_SOURCE
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "utils.h"
#include "network.h"
#include "khash.h"
#include "CUnit/CUnit.h"

#ifndef __STATIC_H_
#define __STATIC_H_

typedef struct {
    size_t size;
    int fd;
} cached_file;

void 
    handle_static(client *cli, char *path);

void
    init_static_server(void);

void 
    destroy_static_server(void);

char*
    extension(char *value);

char*
    mime_type(char *value);

#else
#endif

#if TEST
int
    init_static_test_suite(void);

void
    test_file_extension(void);

void
    test_mime_types(void);

#endif

