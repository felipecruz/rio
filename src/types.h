#ifndef __TYPES_H
#define __TYPES_H
typedef struct {
    int fd;
    int keep_alive;
    char *buffer;
    char *path;
    unsigned char method;
} client;
#else
#endif

