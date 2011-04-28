#include "static.h"

char *default_static_response = 
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %d\r\n\n%s\n\n";

typedef struct {
    client *cli;
    struct aiocb *cb;
} cliaio;


void aio_read_completion_handler(union sigval event) {
    cliaio *cl;
    client *cli;
    struct aiocb *cb;
    int s = 0;
    char *buff;

    cl = (cliaio*)event.sival_ptr;
    cli = cl->cli;
    cb = cl->cb;

    s = (int)cb->aio_nbytes;
    ((char*)cb->aio_buf)[s] = '\0';

    buff = malloc(93 + (sizeof(char) * s));
    
    sprintf(buff, default_static_response, s, (char*)cb->aio_buf);
    //memcpy(buff, (void*)cb->aio_buf, s);
    //sprintf(
    s = (int)strlen(buff);
    buff[s] = '\0';
    //printf("Response: %s\n", buff);

    handle_write(cli, (char*)buff);

    //printf("posix aio completes.. free..\n");

    close(cb->aio_fildes);
    free(cl);
    free((void*)cb->aio_buf);
    free(cb);

    return;
}

void handle_static(client *cli, const char *path) {

    int ret;
    struct stat st;
    size_t size;
    struct aiocb *cb;
    FILE *file = NULL;
    cliaio *cl;


    cl = malloc(sizeof(cliaio));;
    if (cl == NULL) {
        error_exit("Can not malloc..");
    }
    bzero(&st, sizeof(st));
    stat(path, &st);
    size = st.st_size;
    
    //printf("Opening: %s",&path[1]);

    file = fopen(path, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        //#TODO handle 404
    }

    cb = malloc(sizeof(struct aiocb));
    if (cb == NULL) {
        error_exit("Can not malloc..");
    }
    bzero(cb,sizeof(struct aiocb));

    cb->aio_buf = malloc((size_t)(size * sizeof(char)) + 10);
    cb->aio_fildes = fileno(file);
    cb->aio_nbytes = size;
    cb->aio_offset = 0;
    cb->aio_sigevent.sigev_notify = SIGEV_THREAD; //SIGEV_SIGNAL;
    //cb->aio_sigevent.sigev_signo = SIGUSR1;
    cb->aio_sigevent.sigev_notify_function = aio_read_completion_handler;
    cb->aio_sigevent.sigev_notify_attributes = NULL;

    cl->cli = cli;
    cl->cb = cb;

    cb->aio_sigevent.sigev_value.sival_ptr = cl;

    ret = aio_read(cb);

    if (ret < 0)
        error_exit("aio_read");

    return;
}
