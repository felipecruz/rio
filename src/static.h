#include <aio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <aio.h>
#include "utils.h"
#include "network.h"


void handle_static(client *cli, const char *path);
