#define _GNU_SOURCE
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "types.h"
#include "utils.h"
#include "khash.h"

#ifndef __STATIC_H_
#define __STATIC_H_

typedef struct {
    size_t size;
    int fd;
    char *filename;
} cached_file;

void 
    handle_static(rio_client *cli, char *path);

void
    init_static_server(void);

void 
    destroy_static_server(void);

char*
    extension(char *value);

const char*
    mime_type(char *value);

#else
#endif
