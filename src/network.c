#include "network.h"

struct epoll_event ev, events[MAX_EVENTS];
int conn_sock, nfds, epollfd;
client *clients;

static int on_message(http_parser *parser) {
    return 0;
}

static int on_path(http_parser *parser, const char *at, size_t len) {
    return 0;
}

static int on_query_string(http_parser *parser, const char *at, size_t len) {
    return 0;
}

static int on_url(http_parser *parser, const char *at, size_t len) {
    client *client = parser->data;

    //printf("data: %s\n", at);

    client->path = (char*)malloc( (size_t)(sizeof(char) * len + 1));
    if (client->path == NULL) {
        error_exit("Can not malloc...");
    }

    strncpy(client->path, at, len);
    client->path[len] = '\0';
    //printf("%s %d\n", client->path, len);
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

http_parser_settings parser_settings = { 
                                        on_message, on_path,on_query_string, 
                                        on_url, on_fragment, on_header_field,
                                        on_header_value, on_headers_complete,
                                        on_body, on_message_complete
                                        };

void handle_write(client *cli, char* resp) {
    struct epoll_event ev;
    int s;

    // remove valgrind warning!!!
    memset(&ev, 0, sizeof(ev)); 

    if (cli->last > 0) {
        printf("append to buffer!!!!\n");
    }

    if (cli->buffer != NULL) {
        //free(cli->buffer);
        //cli->buffer = NULL;
        printf("realloc...\n");
        cli->t++;
        s = (int) strlen(cli->buffer);       
        cli->buffer = realloc(cli->buffer,s + (sizeof(char) * strlen(resp)+1));
        strcpy(cli->buffer, strcat(cli->buffer, resp));        
    } else {  
        //allocate buffer for write data
        cli->buffer = malloc(sizeof(char) * strlen(resp)+1);
     
        //printf("handle write: %d %s\n", cli->fd, cli->buffer);
       
        //copy response to client buffer and null terminate content
        strcpy((char *)cli->buffer,resp);
    }

    cli->buffer[strlen(resp)-1] = '\0';
    
    //free response since already copied to user buffer
    free(resp);
    
    //set event to write and event fd to client fd
    ev.events = EPOLLOUT;
    ev.data.fd = cli->fd;
    
    //reset client last
    cli->last = 0;
    
    //copy client to global client array
    memcpy(&clients[cli->fd],cli, sizeof(client));
    
    //add client to write event
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, cli->fd, &ev) == -1) {
        //no problem!!!! do_write will sent as many as needed
        printf("try again!!!!!!!!");
        while (epoll_ctl(epollfd, EPOLL_CTL_ADD, cli->fd, &ev) == -1) {
            usleep(500);
        }
    }

}

void do_write(client *cli, struct epoll_event *ev) {
    int sent, size;
    char *p;

    //printf("do write: %d %s\n", cli->fd, cli->buffer);

    if (cli->last > 0) {
        printf("last > 0");
    }

    //set char pointer to client buffer begin
    p = &cli->buffer[cli->last];
    size = (int)strlen(p);
    do { //send until loop conditions
        printf("will send block?\n");
        sent = send(cli->fd, p , (int)strlen(p), 0);
        if (sent < 0 && errno != EAGAIN) {
            //if error, remove client fd from epoll and close
            printf("Handle error!!!\nClient disconnected!\n");
            epoll_ctl(epollfd, EPOLL_CTL_DEL, cli->fd, ev);
            close(cli->fd);
        }
        //update last and pointer
        cli->last += sent;
        p = &cli->buffer[cli->last];
        //printf("=%d==%d==============================\n", sent, size);
    } while (sent > 0 && errno != EAGAIN && errno != EWOULDBLOCK && cli->last < size);    

    //remove client from write, close and free client buffer
    epoll_ctl(epollfd, EPOLL_CTL_DEL, cli->fd, ev);
    close(cli->fd);
    free(cli->buffer);
    free(cli->path);
    cli->path = NULL;
    cli->buffer = NULL;
    cli->last = 0;
    cli->state = 0;
    cli->t = 0;
    memcpy(&clients[cli->fd], cli, sizeof(client));

}

int handle_read(client *cli, struct epoll_event *ev) {
    size_t len = 4096;
    char *p;
    ssize_t received;
    
    cli->state = 1;
    if (cli->buffer != NULL) {
        //free(cli->buffer);
        //printf("Buff not null %s\n", cli->buffer);
    }

    //allocate space for data
    cli->buffer = (char*)malloc( (size_t)(sizeof(char) * 4096) );
    p = cli->buffer;

    do { //read until loop conditions
        received = recv(ev->data.fd, p, len, 0);
        if (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            //if error, remove from epoll and close socket
            printf("Handle error!!!\nClient disconnected!\n");
            epoll_ctl(epollfd, EPOLL_CTL_DEL, ev->data.fd, ev);
            close(ev->data.fd);
        }
        p = &cli->buffer[received];
    } while (received >= len && errno != EAGAIN && errno != EWOULDBLOCK);
    
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
        } else if (n != received) {
            printf("Error parsing, closing socket...\n");
            //delete fd from epoll and close
            epoll_ctl(epollfd, EPOLL_CTL_DEL, event.data.fd, &event);
            close(event.data.fd);
        }

        response = dispatch(cli, cli->path); 
       
        //printf("%s\n=====================================\n", cli->buffer);
        //remove read events from fd
        epoll_ctl(epollfd, EPOLL_CTL_DEL, event.data.fd, &event);
        
        l = (int)strlen(response);
        if (l > 2) {                
            //write response
            handle_write(cli, response);
        }
        free(cli->path);
        cli->path = NULL;
        free(parser);
        //free(cli->buffer);
        free(response);
        printf("XX\n");
    } else if (event.events & EPOLLOUT) {
        //if socket is ready to write, do it!
        do_write(cli, &event);
    }
}

void init_clients() {
    int i = 0;
    clients = calloc(MAX_EVENTS, sizeof(client));
    for(i = 0; i < MAX_EVENTS; i++) {
        clients[i].fd = 0;
        clients[i].buffer = NULL;
        clients[i].last = 0;
        clients[i].state = 0;
        //bzero(clients[i].offsets, sizeof(int) * 100);
    }
}

int run(int argc, char** args) {
    int flags, arg;
    int s_fd, n, i;
    struct sockaddr_in sin, temp_client;
    unsigned int c_len;
    client cli;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr("0.0.0.0");

    init_clients();

    //create socket
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_exit("Could not create socket.");
    }

    //set socket non-blocking
    if (fcntl(s_fd, F_SETFL, O_NONBLOCK) == -1) {
        error_exit("Could not set socket non-blocking");
    }

    //set socket options
    arg = 1;
    if (setsockopt (s_fd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
        error_exit("Socket options");
    }

    //bind socket to local addr
    if (bind(s_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        error_exit("bind");
    }

    //listen on this socket
    if (listen(s_fd,MAX_EVENTS) < 0) {
        error_exit("listen");
    }

    //create epoll
    epollfd = epoll_create(MAX_EVENTS);
    if (epollfd == -1) {
        error_exit("epoll_create");
    }

    //configure epoll events and file descriptor
    ev.events = EPOLLIN | EPOLLPRI;
    ev.data.fd = s_fd;
    
    //add listen socket to epoll
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, s_fd, &ev) == -1) {
        error_exit("epoll_ctl: listen_sock");
    }
 
    for (;;) {
        //poll events
        printf("will epoll block?\n");
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, 100);
        if (nfds == -1 && errno != EWOULDBLOCK) {
            error_exit("epoll_pwait");
        }
        for (n = 0; n < nfds; ++n) {
            //if event fd == server fd -> accept new connection
            if (events[n].data.fd == s_fd) {

                c_len = sizeof(temp_client);
                //accept client connection
                conn_sock = accept(s_fd, (struct sockaddr *) &temp_client, &c_len);
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

                //store client information
                clients[conn_sock].fd = conn_sock;
                clients[conn_sock].last = 0;
            } 
            //read or write event - handle event
            else {
                //retrieve client info by fd and handle event
                cli = clients[events[n].data.fd];
                printf("Event: %d\n", events[n].events);
                if (cli.state > 0 && events[n].events == 1) {
                    printf("Event: %d\n", events[n].events);
                    /*ev.events = EPOLLIN;
                    ev.data.fd = cli.fd;
                    printf("client sent another request but response wasn't sent");
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1) {
                        error_exit("Could not add conn_sock to epoll");
                    }*/
                } else {
                    handle_http(events[n], &cli);
                }
            }
        }
    }
    //#TODO: clean exit!!!
    close(s_fd);
    exit(0);
}
