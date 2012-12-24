#include <signal.h>
#include <sys/wait.h>
#include "network.h"

#define WORKERS 1 

static short int INTERRUPTED = 0;

void
    sig_proc_exit(int sig)
{
    INTERRUPTED = 1;
}

void
    sig_proc_abort(int sig)
{
    debug_print("Aborting..\n", sig);
    abort();
}

void
    send_command(void *publisher, char *command)
{
    zmq_send(publisher, command, strlen(command), ZMQ_DONTWAIT);
}


int main (int argc, char **args) {

    int status;
    pid_t pid;
    rio_runtime *runtime;

    printf(" _ __  _   ___  \n");
    printf("| '__|| | / _ \\ \n");
    printf("| |   | || (_) |\n");
    printf("|_|   |_| \\___/ \n");

    runtime = malloc(sizeof(rio_runtime));
    if (runtime == NULL) {
        error_exit("malloc error");
    }

    runtime->server_fd = socket_bind();
    runtime->workers = malloc(WORKERS * sizeof(rio_worker*));
    if (runtime->workers == NULL) {
        error_exit("malloc error");
    } 

    //this will be configurable
    for (int i = 0; i < WORKERS; i++) {
        *(runtime->workers + i) = malloc(sizeof(rio_worker));
    }

    signal(SIGINT, sig_proc_exit);
    signal(SIGABRT, sig_proc_abort);

    for (int i = 0; i < WORKERS; i++) {
        pid_t pid;
        switch (pid = fork()) {
            case -1:
                //handle error
                break;
            case 0:
                run_worker(i, runtime->workers[i], runtime);
                _exit(0);
            default:
                debug_print("Master after fork\n", runtime);
        }
    }
                
    runtime->zmq_context = zmq_ctx_new();
    runtime->publisher = zmq_socket(runtime->zmq_context,
                                    ZMQ_PUB);
    zmq_bind(runtime->publisher, "ipc:///tmp/rio_master.sock");

    while (1) {
        int rc;
        
        if (INTERRUPTED != 0) {
            debug_print("Sending terminate!\n", runtime);
            send_command(runtime->publisher, "terminate");
            //terminate workers and exit gracefully
            break;
        }

        send_command(runtime->publisher, "tick");
        sleep(5);
    }

    for (int i = 0; i < WORKERS; i++) {
        pid = waitpid(runtime->workers[i]->pid, &status, 0);
    }
    
    debug_print("Freeing workers\n", runtime);
    for (int i = 0; i < WORKERS; i++) {
        free(runtime->workers[i]);
    }
    
    int rc = zmq_close(runtime->publisher);
    debug_print("Master ZMQ Socket close return %d\n", rc);
    
    rc = zmq_ctx_destroy(runtime->zmq_context);
    debug_print("Master ZMQ Context termination return :%d\n", rc);   

    free(runtime->workers);
    close(runtime->server_fd);
    free(runtime);

    exit(0);

}
