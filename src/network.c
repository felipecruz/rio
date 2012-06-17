#include <strings.h>

#include "network.h"
#include "websocket.h"
#include "buffer.h"

int eag = 0;

static int 
    on_message(http_parser *parser) 
{
    return 0;
}

static int 
    on_path(http_parser *parser, const char *at, size_t len) 
{
    rio_client *client = parser->data;
    client->path = malloc(sizeof(char) * (len+1));

    if (client->path == NULL) {
        error_exit("Malloc");
    }    
    strncpy(client->path, at, len);
    client->path[len] = '\0';

    client->method = (unsigned char) parser->method;

    debug_print("HTTP-REQ method: %d\n", (int) client->method);
    debug_print("HTTP-REQ Path: %s %d\n", client->path, (int) len);

    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t len) {
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t len) {
    return 0;
}

int on_headers_complete(http_parser *parser) {
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
    on_header_field,
    on_header_value, 
    on_headers_complete,
    on_body, 
    on_message_complete
};

void 
    handle_write(rio_worker *worker, rio_client *cli) 
{
    struct epoll_event ev;
    int ret;
    khiter_t k;

    debug_print("Handle Write: %d : %s\n", cli->fd, 
                                rio_buffer_get_data(cli->buffer));

    ev.events = EPOLLOUT | EPOLLET;
    ev.data.fd = cli->fd;
    
    if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_MOD, cli->fd, &ev) == -1) {
        debug_print("Error on epoll_ctl_mod on %d\n", 
                                                cli->fd, worker->epoll_fd);
    }
    
    k = kh_put(clients, h, cli->fd , &ret);
    kh_value(h, k) = *cli;
}

void 
    do_write(rio_worker *worker, rio_client *cli, struct epoll_event *ev) 
{
    int ret;
    int sent;
    khiter_t k;

    if (cli->buffer->length == 0) {
        return;
    }
           
    debug_print("Do Write ws(%d) to fd: %d : %s\n", cli->websocket, cli->fd, 
                                            rio_buffer_get_data(cli->buffer));

    do {
        sent = send(cli->fd, 
                    rio_buffer_get_data(cli->buffer), 
                    cli->buffer->length, 
                    MSG_DONTWAIT);
    
        if (sent < 0 && errno != EAGAIN) {
            debug_print("Do Write: send error on fd: %d errno: %d\n", 
                        cli->fd, errno);
            break;
        } else if (sent < 0 && errno == EAGAIN) {
             debug_print("Do Write: EAGAIN on fd: %d\n", 
                        cli->fd);
             break;
        } else if (sent > 0) {
            rio_buffer_adjust(cli->buffer, sent);
        }
    } while (sent > 0 && rio_buffer_get_data(cli->buffer) != NULL);
    
    //upgrade from ws hadnshake to ws connection
    if (cli->websocket == 1)
        cli->websocket = 2;
    
    debug_print("Do Write sent: %d from: %zu\n", sent, 
                                                cli->buffer->length);
 
    // if it's a regular connection and since we don't support chunked responses
    // after write the response we close the connection
    if (cli->websocket == 0) {   
        if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_DEL, cli->fd, ev) == -1) {
                debug_print("Error on epoll_ctl_del on %d\n", cli->fd, 
                                                              worker->epoll_fd);
        }
        
        if (close(cli->fd) == -1) {
            debug_print("Error on close client %d\n", 
                                                cli->fd, worker->epoll_fd);
        }
        
    } else {
        ev->events = EPOLLIN | EPOLLOUT | EPOLLET;

        //add socket to epoll
        if (epoll_ctl(worker->epoll_fd, 
                      EPOLL_CTL_MOD,
                      cli->fd,
                      ev) == -1) {
            error_exit("Could not add conn_sock to epoll");
        }
    }

    if (cli->buffer->content != NULL) {
        rio_buffer_free(&cli->buffer);
    }

    k = kh_put(clients, h, cli->fd , &ret);
    kh_value(h, k) = *cli;

}

int 
    handle_read(rio_worker *worker, rio_client *cli, struct epoll_event *ev) 
{
    size_t len = 4096;
    ssize_t received = 0;
    ssize_t total_received = 0;

    cli->buffer = new_rio_buffer_size(4096);
        
    debug_print("\n ============ INPUT ==============\n"
                "Handle Read from %d websocket(%d)\n", cli->fd, 
                                                       cli->websocket);

    do {
        received = recv(ev->data.fd, cli->buffer->content, len, MSG_DONTWAIT);
        if (received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                if (received == 0) {
                    //if error, remove from epoll and close socket
                    debug_print("Client received error: disconnected"
                                "errno %d\n", errno);
                } else {
                    debug_print("Some other error %d\n", errno);
                }
                //handle_http will take care of this :)
            } else {
                //received += 1;
                debug_print("EAGAIN on recv from fd: %d\n", cli->fd); 
                
                //if EAGAIN, insert on epoll again
                ev->events = EPOLLIN | EPOLLET;
                //add socket to epoll
                if (epoll_ctl(worker->epoll_fd, 
                                EPOLL_CTL_MOD, 
                                cli->fd, 
                                ev) == -1) {
                    error_exit("Could not add conn_sock to epoll");
                }

                eag += 1;
                printf("EAGAIN %d\n", eag);

            }
            break;
        } else if (received == 0) {
            //client disconnected
            break;
        } else {
            total_received += received;
            debug_print("READ AGAIN on %d\n", cli->fd);
        }
    } while (received > 0 && rio_buffer_get_data(cli->buffer) != NULL);
    
    cli->buffer->length = total_received;

    debug_print("Total received %zu\n", total_received);

    return received;
}

void
    handle_websocket_connection(rio_worker *worker, rio_client *cli,
                                struct epoll_event *event)
{
    int ret;
    size_t size = 4096;
    khiter_t k;
    enum ws_frame_type frame_type = WS_INCOMPLETE_FRAME;
    struct handshake hs;

    nullhandshake(&hs);

    char *temp = rio_buffer_get_data(cli->buffer);

    frame_type = ws_parse_handshake(temp,
                                    cli->buffer->length,
                                    &hs);

    if (frame_type == WS_INCOMPLETE_FRAME ||
        frame_type == WS_ERROR_FRAME) {
        debug_print("Websocket frame error or incomplete\n", cli->fd);
        rio_buffer_free(&cli->buffer);
        close(cli->fd);
        epoll_ctl(worker->epoll_fd, EPOLL_CTL_DEL, cli->fd, event);
        return;
    }
    
    rio_buffer_free(&cli->buffer);
    cli->buffer = new_rio_buffer_size(sizeof(char) * 4096);

    ws_get_handshake_answer(&hs, 
                            cli->buffer->content,
                            &size);

    cli->websocket = 1;
    cli->buffer->length = size;

    handle_write(worker, cli);

    k = kh_put(clients, h, cli->fd , &ret);
    kh_value(h, k) = *cli;
}
    
uint8_t _fake_single_frame[] = {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f};

void 
    handle_ws(rio_worker *worker, struct epoll_event event, rio_client *cli)
{
    if (event.events & EPOLLIN) {
       int received = handle_read(worker, cli, &event);

       if (received <= 0) {
            return;
       }

       enum ws_frame_type type = \
            ws_parse_input_frame(rio_buffer_get_data(cli->buffer),
                                 cli->buffer->length,
                                 NULL,
                                 0);
            
            debug_print("Websocket frame type %d\n", (int) type);

            if (type == WS_CLOSING_FRAME) {
                //TODO send a closing frame reply?
                rio_buffer_free(&cli->buffer);
                close(cli->fd);
                return;
            }

            rio_buffer_free(&cli->buffer);
            cli->buffer = new_rio_buffer();
            rio_buffer_copy_data(cli->buffer, _fake_single_frame, 7); 
            handle_write(worker, cli);

    } else if (event.events & EPOLLOUT) {
        if (cli->buffer != NULL) {
            do_write(worker, cli, &event);
        }
    }
}
void 
    handle_http(rio_worker *worker, struct epoll_event event, rio_client *cli)
{
    int response;

    size_t n;
    
    if (event.events & EPOLLIN) {
        //handle read
        int received = handle_read(worker, cli, &event);
    
        //create http parser
        http_parser *parser = malloc(sizeof(http_parser));

        if (!parser) {
          error_exit("malloc error: http_parser");
        }

        http_parser_init(parser, HTTP_REQUEST);
        
        //set parser data
        parser->data = (void*)cli;    
        
        //execute http parsing only if data was read
        if (received > 0) {
            debug_print("Execute http parsing client: %d\n", cli->fd);
            n = http_parser_execute(parser, 
                                    &parser_settings, 
                                    rio_buffer_get_data(cli->buffer),
                                    received);
        }

        if (parser->upgrade) {
            handle_websocket_connection(worker, cli, &event);
            goto end_clean_path;

        } else if (received == 0) { // client disconnected!
            debug_print("Client %d Disconnected!\n", cli->fd);
            epoll_ctl(worker->epoll_fd, EPOLL_CTL_DEL, event.data.fd, &event);
            close(cli->fd);
            rio_buffer_free(&cli->buffer); 
            free(parser);
            return;
        } else if (n != received) {
            debug_print("Error parsing, closing socket n:%zu received:%d\n", 
                        n, received);
            
            //delete fd from epoll and close
            epoll_ctl(worker->epoll_fd, EPOLL_CTL_DEL, event.data.fd, &event);
            close(cli->fd);

            rio_buffer_free(&cli->buffer); 
            free(parser);
            return;
        }

        rio_buffer_free(&cli->buffer);
        response = dispatch(cli, cli->path); 
        
        if (response != DISPATCH_FINISHED) {                
            //write response
            debug_print("Async Dispatch to fd: %d\n", cli->fd);
        }
        
        end_clean_path:

        debug_print("Freeing %s\n", cli->path);
        free(cli->path);
        cli->path = NULL;
        free(parser);

    } else if (event.events & EPOLLOUT) {
        //if socket is ready to write, do it!
        do_write(worker, cli, &event);
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

    debug_print("Closing clients structures\n", h);
    
    for (element = kh_begin(h); element != kh_end(h); ++element) {
        if (kh_exist(h, element)) {
            debug_print("%d\n", ((rio_client) kh_val(h, element)).fd);
            kh_del(clients, h, element);
        }
    }

    kh_destroy(clients, h);
}

int 
    socket_bind()
{
    int server_fd;
    int arg;
    struct sockaddr_in sin;

    //bind
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr("0.0.0.0");

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
    if (listen(server_fd, MAX_EVENTS) < 0) {
        error_exit("listen");
    }

    return server_fd;
}

void
    run_master(rio_runtime *runtime);

void
    accept_incoming_connection(rio_runtime *runtime, rio_worker *worker)
{
    int new_connection_socket;
    int flags;
    int ret;
    
    unsigned int client_len;
    
    struct epoll_event ev;
    struct sockaddr_in temp_client;
    
    rio_client cli;
    khiter_t k;
    
    client_len = sizeof(temp_client);

    //accept client connection
    new_connection_socket = accept(runtime->server_fd, 
                                  (struct sockaddr *) &temp_client,
                                   &client_len);

    if (new_connection_socket == -1) {
        //#TODO what to do?
        error_exit("Could not accept socket");
    }

    //check sockets flags and set non-blocking after
    if (-1 == (flags = fcntl(new_connection_socket, F_GETFL, 0))) {
        flags = 0;
    }
    if (fcntl(new_connection_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        error_exit("Could not set client socket non-blocking");
    }
    
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = new_connection_socket;

    //add socket to epoll
    if (epoll_ctl(worker->epoll_fd, 
                  EPOLL_CTL_ADD, 
                  new_connection_socket, 
                  &ev) == -1) {
        error_exit("Could not add conn_sock to epoll");
    }

    //store client information
    cli.fd = new_connection_socket;
    cli.websocket = 0;
    cli.buffer = NULL;

    k = kh_put(clients, h, new_connection_socket , &ret);
    kh_value(h, k) = cli;
    
    debug_print("New Client: %d\n", cli.fd);
}

void
    run_worker(int id, rio_worker* worker, rio_runtime *runtime)
{
    int size_epoll_events;
    int rc;
    
    struct epoll_event ev, events[MAX_EVENTS];
    
    khiter_t k;
    rio_client cli;

    sprintf(worker->name, "worker %d", id);
    debug_print("Identifying worker as %s pid %d\n", worker->name,
                                                     getpid());

    init_clients();
    init_dispatcher();
    init_static_server();

    worker->zmq_context = zmq_init(1);
    worker->master = zmq_socket(worker->zmq_context, ZMQ_SUB);

    zmq_setsockopt(worker->master, ZMQ_SUBSCRIBE, "", strlen(""));
    zmq_connect(worker->master, "ipc:///tmp/rio_master.sock");

    //create epoll
    worker->epoll_fd = epoll_create(MAX_EVENTS);
    if (worker->epoll_fd == -1) {
        error_exit("epoll_create");
    }

    //configure epoll events and file descriptor
    ev.events = EPOLLIN | EPOLLPRI | EPOLLET;
    ev.data.fd = runtime->server_fd;
    
    //add listen socket to epoll
    if (epoll_ctl(worker->epoll_fd, 
                  EPOLL_CTL_ADD, 
                  runtime->server_fd, 
                  &ev) == -1) {
        error_exit("epoll_ctl: listen_sock");
    }
 
    while (1) {
        //poll events
        size_epoll_events = epoll_wait(
                        worker->epoll_fd, events, MAX_EVENTS, 100);

        if (size_epoll_events == -1 && errno != EWOULDBLOCK) {
            break;
        }

        for (int n = 0; n < size_epoll_events; ++n) {
            //if event fd == server fd -> accept new connection
            if (events[n].data.fd == runtime->server_fd) {
                accept_incoming_connection(runtime, worker);            
            } else { //handle in out readyness :)
                //retrieve client info by fd and handle event
                k = kh_get(clients, h, events[n].data.fd);
                cli = kh_val(h, k);

                if (cli.websocket < 2) {
                    handle_http(worker, events[n], &cli);
                } else {
                    handle_ws(worker, events[n], &cli);
                }
            }
        }
        //dispatch responses
        dispatch_responses(worker);

        //look for master messages
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        rc = zmq_recv(worker->master, &msg, ZMQ_NOBLOCK);
        if (rc == 0) {
            debug_print("Worker %d Received %s from master\n", 
                                            id,
                                            (char *) zmq_msg_data(&msg));
        }

        if (strcmp((char *) zmq_msg_data(&msg), "terminate") == 0) {
            zmq_msg_close(&msg);
            break;
        }
        
        zmq_msg_close(&msg);

    }
    debug_print("\nWorker terminating gracefully\n", worker);

    rc = zmq_close(worker->master);
    debug_print("Worker ZMQ Socket close return %d\n", rc);
    
    rc = zmq_term(worker->zmq_context);
    debug_print("Worker ZMQ Context termination return :%d\n", rc);   
    
    free_clients();
    destroy_static_server();
    destroy_dispatcher();
    close(worker->epoll_fd);
}
