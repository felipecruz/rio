#include <strings.h>
#include "dispatch.h"
#include "static.h"

char *default_response = 
    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
    "Server: rio 0.0.1\r\n"
    "Content-Length: %d\r\n\n%s";

char *body = 
    "<html><head><title>Test</title></head><body><h1>Response</h1>"
    "<p>Response Test</p></body></html>";

void *context; 
void *publisher; 

void
    init_dispatcher(void)
{
    debug_print("Initializing zmq context and publisher\n", context);
    
    context = zmq_init(1);
    
    publisher = zmq_socket(context, ZMQ_REQ);
    zmq_bind(publisher, "tcp://127.0.0.1:5555");
}

void
    destroy_dispatcher(void)
{
    int ret;

    ret = zmq_close(publisher);
    debug_print("Socket close return %d\n", ret);
    
    ret = zmq_term(context);
    debug_print("Context termination return :%d\n", ret);
}

void
    dispatch_responses( )
{
    
    int rc;
    int fd;
    
    size_t offset = 0;
    
    client cli;
    zmq_msg_t msg;

    zmq_pollitem_t items[] = { {publisher, 0, ZMQ_POLLIN, 0} };
    zmq_poll (items, 1, 0);

    zmq_msg_init(&msg);
    rc = zmq_recv(publisher, &msg, 0);

    if (zmq_msg_size(&msg) > 0) {
        struct epoll_event ev;
        khiter_t k;
        msgpack_unpacked result;
        msgpack_unpacker pac;
    
        /* deserializes these objects using msgpack_unpacker. */
        msgpack_unpacker_init(&pac, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

        /* feeds the buffer. */
        msgpack_unpacker_reserve_buffer(&pac, zmq_msg_size(&msg));
        memcpy(msgpack_unpacker_buffer(&pac), zmq_msg_data(&msg), zmq_msg_size(&msg));
        msgpack_unpacker_buffer_consumed(&pac, zmq_msg_size(&msg));

        /* now starts streaming deserialization. */
        msgpack_unpacked_init(&result);

        msgpack_unpacker_next(&pac, &result);
        
        msgpack_object _fd;
        msgpack_object _content;

        _fd = result.data.via.array.ptr[0];
        _content = result.data.via.array.ptr[1];
        
        fd = (int) _fd.via.u64;

        k = kh_get(clients, h, fd);
        cli = kh_val(h, k);

        cli.buffer = malloc(sizeof(char) * 1024);

        sprintf(cli.buffer, 
                default_response, 
                _content.via.raw.size, 
                _content.via.raw.ptr);

        msgpack_unpacked_destroy(&result);
        msgpack_unpacker_destroy(&pac);

        bzero(&ev, sizeof(struct epoll_event));

        ev.events = EPOLLOUT;
        ev.data.fd = cli.fd;
        
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, cli.fd, &ev) == -1) {
            //no problem!!!! do_write will sent as many as needed
            printf("try again!!!!!!!!");
            while (epoll_ctl(epollfd, EPOLL_CTL_MOD, cli.fd, &ev) == -1) {
                //usleep(500);
            }
        }
        
        k = kh_put(clients, h, cli.fd , &rc);
        kh_value(h, k) = cli;

    }
}

int 
    dispatch(client *cli, char *path) 
{   
    char *buf = NULL;
    
    debug_print("Request URI %s\n", path);

    if (is_filename(path)) {
        handle_static(cli, path);
        return DISPATCH_FINISHED;
    }
    
    int rc;
    zmq_msg_t msg;
    
    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    msgpack_pack_array(pk, 3);
    msgpack_pack_int(pk, cli->fd);
    msgpack_pack_int(pk, (int) cli->method);
    msgpack_pack_raw(pk, strlen(path));
    msgpack_pack_raw_body(pk, path, strlen(path));

    zmq_msg_init_size(&msg, buffer->size);
    memcpy(zmq_msg_data(&msg), buffer->data, buffer->size);

    rc = zmq_send(publisher, &msg, 0);
    debug_print("zeromq bytes sent: %d\n",rc);

    zmq_msg_close(&msg);
    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);

    return DISPATCH_LATER;
}

int 
    is_filename(char *path)
{
    int path_size = strlen(path);
    
    if (strchr(path, '.') != NULL && path_size > 2) {
        return 1; 
    }

    return 0;
}
#if TEST
#include "CUnit/CUnit.h"
int
    init_dispatch_test_suite(void)
{
    return 0;    
}

void
    test_is_filename(void)
{
    CU_ASSERT(1 == is_filename("/test/file.c"));
    CU_ASSERT(1 == is_filename("/test/file.js"));
    CU_ASSERT(1 == is_filename("/test/file.css"));
    CU_ASSERT(0 == is_filename("/test/endpoint"));

    return;
}
#endif
