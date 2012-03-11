#include "network.h"

struct epoll_event ev, events[MAX_EVENTS];
int server_fd, conn_sock, nfds, epollfd;
short int INTERRUPTED = 0;

KHASH_MAP_INIT_INT(clients, client)
khash_t(clients) *h;

void
    sig_proc_exit(int sig)
{
    INTERRUPTED = 1;
}

static int 
    on_message(http_parser *parser) 
{
    return 0;
}

static int 
    on_path(http_parser *parser, const char *at, size_t len) 
{
    client *client = parser->data;
    client->path = malloc(sizeof(char) * (len+1));

    if (client->path == NULL) {
        error_exit("Malloc");
    }    
    strncpy(client->path, at, len+1);
    client->path[len] = '\0';

    debug_print("HTTP-REQ Path: %s %d\n", client->path, len);

    return 0;
}

static int on_query_string(http_parser *parser, const char *at, size_t len) {
    return 0;
}

static int on_fragment(http_parser *parser, const char *at, size_t len) {
    return 0;
}

static int on_header_field(http_parser *parser, const char *at, size_t len) {
    return 0;
}

static int on_header_value(http_parser *parser, const char *at, size_t len) {
    return 0;
}

static int on_headers_complete(http_parser *parser) {
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t len) {
    return 0;
}

int on_message_complete(http_parser *parser) {
    return 0;
}

http_parser_settings parser_settings = 
{ 
    on_message, 
    on_path, 
    on_query_string, 
    NULL, 
    NULL, 
    on_header_field,
    on_header_value, 
    on_headers_complete,
    on_body, 
    on_message_complete
};

void 
    handle_write(client *cli, char* resp) 
{
    struct epoll_event ev;
    int s;

    cli->buffer = malloc(sizeof(char) * strlen(resp));
    
    strcpy((char *)cli->buffer, resp);
    cli->buffer[strlen(resp)] = '\0';
    
    free(resp);
    
    ev.events = EPOLLOUT;
    ev.data.fd = cli->fd;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, cli->fd, &ev) == -1) {
        //no problem!!!! do_write will sent as many as needed
        printf("try again!!!!!!!!");
        while (epoll_ctl(epollfd, EPOLL_CTL_MOD, cli->fd, &ev) == -1) {
            usleep(500);
        }
    }

}

void 
    do_write(client *cli, struct epoll_event *ev) 
{
    int sent, size;
    char *p;
    
    sent = send(cli->fd, cli->buffer, (int)strlen(cli->buffer), 0);
    if (sent < 0 && errno != EAGAIN) {
        printf("Client disconnected\n");
    }
        
    epoll_ctl(epollfd, EPOLL_CTL_DEL, cli->fd, ev);
    close(cli->fd);

    if (cli->buffer != NULL) {
        free(cli->buffer);
        cli->buffer = NULL;
    }
}

int 
    handle_read(client *cli, struct epoll_event *ev) 
{
    size_t len = 4096;
    char *p;
    ssize_t received;

    //allocate space for data
    cli->buffer = (char*)malloc( (size_t)(sizeof(char) * 4096) );
    received = recv(ev->data.fd, cli->buffer, len, 0);

    if (received <= 0) 
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK) 
        {
            if (received == 0) 
            {
                //if error, remove from epoll and close socket
                printf("Client received error: disconnected\n");
            } else 
            {
                printf("Some other error %d\n", errno);
            }

            //error on server_fd.. don't know yet...
            if (ev->data.fd != server_fd) {
                epoll_ctl(epollfd, EPOLL_CTL_DEL, ev->data.fd, ev);
                close(ev->data.fd);
        
                if (cli->buffer != NULL) {
                    free(cli->buffer);
                    cli->buffer = NULL;
                }
            }
        }
    }
    
    return received;
}

void handle_http(struct epoll_event event, client *cli) {
    size_t n;
    char buf[4096], resp[1024];
    int chosen;
    char *response;
    int l = 0;
    if (event.events & EPOLLIN) {
        //create http parser
        http_parser *parser = malloc(sizeof(http_parser));
        http_parser_init(parser, HTTP_REQUEST);
        
        //handle read
        int received = handle_read(cli, &event);
        
        //set parser data
        parser->data = (void*)cli;    
        
        //execute http parsing
        n = http_parser_execute(parser, &parser_settings, cli->buffer, received);
        if (parser->upgrade) {
            //#TODO: what to do?
        } else if (n != received || received == 0) {
            printf("Error parsing, closing socket...\n");
            
            //delete fd from epoll and close
            epoll_ctl(epollfd, EPOLL_CTL_DEL, event.data.fd, &event);
            close(event.data.fd);

            if (cli->buffer != NULL) {
                free(cli->buffer);
                cli->buffer == NULL;
            }

            if (cli->path != NULL) {
                free(cli->path);
                cli->path == NULL;
            }

            return;
        }

        free(cli->buffer);
        response = dispatch(cli, cli->path); 
        
        if (response != NULL) {                
            //write response
            printf("HANDLE WRITE!!!\n");
            handle_write(cli, response);
            //free(response);
        }

        debug_print("Freeing %s\n", cli->path);
        free(cli->path);
        cli->path = NULL;
        free(parser);
        //epoll_ctl(epollfd, EPOLL_CTL_DEL, event.data.fd, &event);
    } else if (event.events & EPOLLOUT) {
        //if socket is ready to write, do it!
        do_write(cli, &event);
    }
}

void 
    init_clients() 
{
   h = kh_init(clients);
}

void
    free_clients()
{
    khiter_t element;
    client *cli;

    debug_print("Closing clients structures\n", h);
    
    for (element = kh_begin(h); element != kh_end(h); ++element) {
        if (kh_exist(h, element)) {
            debug_print("%p\n", kh_val(h, element));
            //free(cli);
            kh_del(clients, h, element);
        }
    }

    kh_destroy(clients, h);
}
int 
    run(int argc, char** args) 
{
    
    client *cli;
    int flags, arg, n, i, ret;
    struct sockaddr_in sin, temp_client;
    unsigned int client_len;
    khiter_t k;

    //bind
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr("0.0.0.0");

    init_clients();
    init_static_server();

    //create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_exit("Could not create socket.");
    }

    //set socket non-blocking
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) == -1) {
        error_exit("Could not set socket non-blocking");
    }

    //set socket options
    arg = 1;
    if (setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
        error_exit("Socket options");
    }

    //bind socket to local addr
    if (bind(server_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        error_exit("bind");
    }

    //listen on this socket
    if (listen(server_fd,MAX_EVENTS) < 0) {
        error_exit("listen");
    }

    //create epoll
    epollfd = epoll_create(MAX_EVENTS);
    if (epollfd == -1) {
        error_exit("epoll_create");
    }

    //configure epoll events and file descriptor
    ev.events = EPOLLIN | EPOLLPRI;
    ev.data.fd = server_fd;
    
    //add listen socket to epoll
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        error_exit("epoll_ctl: listen_sock");
    }

    signal(SIGINT, sig_proc_exit);
 
    for (;;) {
        if (INTERRUPTED != 0) {
            break;
        }
        //poll events
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, 100);
        if (nfds == -1 && errno != EWOULDBLOCK) {
            break;
        }
        for (n = 0; n < nfds; ++n) {
            //if event fd == server fd -> accept new connection
            if (events[n].data.fd == server_fd) {

                client_len = sizeof(temp_client);
                //accept client connection
                conn_sock = accept(server_fd, 
                                  (struct sockaddr *) &temp_client,
                                  &client_len);

                if (conn_sock == -1) {
                    //#TODO what to do?
                    error_exit("Could not accept socket");
                }

                //check sockets flags and set non-blocking after
                if (-1 == (flags = fcntl(conn_sock, F_GETFL, 0))) {
                    flags = 0;
                }
                if (fcntl(conn_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
                    error_exit("Could not set client socket non-blocking");
                }
                
                //set client events
                ev.events = EPOLLIN;
                ev.data.fd = conn_sock;

                //add socket to epoll
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    error_exit("Could not add conn_sock to epoll");
                }

                new(cli, client);
                //store client information
                cli->fd = conn_sock;

                k = kh_put(clients, h, conn_sock , &ret);
                kh_value(h, k) = *cli;
            } 
            //read or write event - handle event
            else {
                //retrieve client info by fd and handle event
                k = kh_get(clients, h, events[n].data.fd);
                *cli = kh_val(h, k);
                handle_http(events[n], cli);
                //free(cli);
            }
        }
    }
    printf("\nCLEAN EXIT!\n");
    
    free_clients();
    destroy_static_server();
    
    close(server_fd);
    exit(0);
}
