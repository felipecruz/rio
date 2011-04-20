#include "network.h"

struct epoll_event ev, events[MAX_EVENTS];
int conn_sock, nfds, epollfd;

char *default_response = 
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\n";
char *body = "<html><head><title>Test</title></head><body><h1>Response</h1><p>Response Test</p></body></html>"; 

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
    
    //allocate buffer for write data
    cli->buffer = malloc(sizeof(char) * strlen(resp)+1);
    
    //copy response to client buffer and null terminate content
    strcpy((char *)cli->buffer,resp);
    cli->buffer[strlen(resp)] = '\0';
    
    //set event to write and event fd to client fd
    ev.events = EPOLLOUT;
    ev.data.fd = cli->fd;
    
    //reset client last
    cli->last = 0;
    
    //copy client to global client array
    memcpy(&clients[cli->fd],cli, sizeof(client));
    
    //add client to write event
    epoll_ctl(epollfd, EPOLL_CTL_ADD, cli->fd, &ev);
}

void do_write(client *cli, struct epoll_event *ev) {
    int sent;
    char *p;

    //set char pointer to client buffer begin
    p = &cli->buffer[cli->last];

    do { //send until loop conditions
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
    } while (sent != 0 && errno != EAGAIN && errno != EWOULDBLOCK && sent < (int)strlen(p));    

    //remove client from write, close and free client buffer
    epoll_ctl(epollfd, EPOLL_CTL_DEL, cli->fd, ev);
    close(cli->fd);
    free(cli->buffer);
}

int handle_read(client *cli, struct epoll_event *ev) {
    size_t len = 4096;
    char *p;
    ssize_t received;

    //allocate space for data
    cli->buffer = (char*)malloc(sizeof(char) * 4096);
    p = cli->buffer;

    do { //read until loop conditions
        received = recv(ev->data.fd, p, len, 0);
        if (received < 0 && errno != EAGAIN) {
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
    if (event.events & EPOLLIN) {
        //create http parser
        http_parser *parser = malloc(sizeof(http_parser));
        http_parser_init(parser, HTTP_REQUEST);
        
        //handle read
        int received = handle_read(cli, &event);
        
        //set parser data
        parser->data = (void*)cli->buffer;    
        
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
        //generate response
        sprintf(resp,default_response,strlen(body));
        
        //remove read events from fd
        epoll_ctl(epollfd, EPOLL_CTL_DEL, event.data.fd, &event);

        //write response
        handle_write(cli, strcat(resp,body));
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
        //clients[i].buffer = malloc(sizeof(char) * 4096);
        clients[i].last = 0;
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
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
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
                ev.data.fd = conn_soick;

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
                handle_http(events[n], &cli);
            }
        }
    }
    //#TODO: clean exit!!!
    close(s_fd);
    exit(0);
}
