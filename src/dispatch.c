#include <strings.h>
#include "czmq.h"
#include "buffer.h"
#include "dispatch.h"
#include "static.h"

char *default_response = 
    "HTTP/1.1 200 OK\r\nContent-Type: %s; charset=utf-8\r\n"
    "Server: rio 0.0.1\r\n"
    "Content-Length: %d\r\n\n%s";

char *body = 
    "<html><head><title>Test</title></head><body><h1>Response</h1>"
    "<p>Response Test</p></body></html>";

void *context; 
void *publisher; 
void *subscriber;

zlist_t *queue;

void
    init_dispatcher(void)
{
    debug_print("Initializing zmq context and publisher\n", context);
    
    context = zmq_init(1);
    
    publisher = zmq_socket(context, ZMQ_PUSH);
    subscriber =  zmq_socket(context, ZMQ_PULL);

    zmq_bind(publisher, "tcp://127.0.0.1:5555");
    zmq_bind(subscriber, "tcp://127.0.0.1:4444");

    queue = zlist_new();
}

void
    destroy_dispatcher(void)
{
    int ret;

    ret = zmq_close(publisher);
    debug_print("Dispatcher Publisher Socket close return %d\n", ret);
 
    ret = zmq_close(subscriber);
    debug_print("Dispatcher Subscriber Socket close return %d\n", ret);
    
    ret = zmq_term(context);
    debug_print("Dispatcher Context termination return :%d\n", ret);

    zlist_destroy(&queue);
}

void
    dispatch_responses(rio_worker *worker)
{
    int rc;
    int fd, len_content_type, len_content;
    char buf[4096];
    
    size_t offset = 0;
    
    rio_client cli;
    zmq_msg_t msg;

//    zmq_pollitem_t items[] = { {publisher, 0, ZMQ_POLLIN, 0} };
//    zmq_poll (items, 1, 0);

    zmq_msg_init(&msg);
    while ((rc = zmq_recvmsg(subscriber, &msg, ZMQ_NOBLOCK)) == 0) {
        debug_print("ZMQ message length: %zu\n", zmq_msg_size(&msg));
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
            
            msgpack_object _len_content_type;
            msgpack_object _content_type;

            msgpack_object _len_content;
            msgpack_object _content;

            _fd = result.data.via.array.ptr[0];
            _len_content_type = result.data.via.array.ptr[1];
            _content_type = result.data.via.array.ptr[2];
            _len_content = result.data.via.array.ptr[3];
            _content = result.data.via.array.ptr[4];
            
            fd = (int) _fd.via.u64;
            len_content_type = (int) _len_content_type.via.u64;
            len_content = (int) _len_content.via.u64;

            char* b = malloc(sizeof(char) * (len_content + 1));
            strncpy(b, _content.via.raw.ptr, len_content);
            b[len_content] = '\0';

            char* a = malloc(sizeof(char) * (len_content_type + 1));
            strncpy(a, _content_type.via.raw.ptr, len_content_type);
            a[len_content_type] = '\0';
//            _content.via.raw.ptr[len_content] = '\0';

            k = kh_get(clients, h, fd);
            cli = kh_val(h, k);

            cli.buffer = new_rio_buffer_size(sizeof(char) * 1024);

            sprintf(cli.buffer->content,
                    default_response, 
                    a,
                    _content.via.raw.size, 
                    b);

            cli.buffer->length = strlen(cli.buffer->content);
            cli.buffer->where = 0;

            debug_print("Response to: %d Content:%s\n", cli.fd, 
                                        rio_buffer_get_data(cli.buffer));

            msgpack_unpacked_destroy(&result);
            msgpack_unpacker_destroy(&pac);

            bzero(&ev, sizeof(struct epoll_event));

            ev.events = EPOLLOUT;
            ev.data.fd = cli.fd;
            
            if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_MOD, cli.fd, &ev) == -1) {
                debug_print("Error epoll_ctl_mod  fd: %d\n", cli.fd);
            }
            
            k = kh_put(clients, h, cli.fd , &rc);
            kh_value(h, k) = cli;
        }
    }
    //closing zmq message
    zmq_msg_close(&msg);
}

int 
    dispatch(rio_client *cli, char *path) 
{   
    debug_print("Request URI %s\n", path);

    if (is_filename(path)) {
        handle_static(cli, path);
        return DISPATCH_FINISHED;
    }
    
    int rc;
    zmq_msg_t msg;
    zmq_msg_t *pmsg;
    
    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    msgpack_pack_array(pk, 3);
    msgpack_pack_int(pk, cli->fd);
    msgpack_pack_int(pk, (int) cli->method);
    msgpack_pack_raw(pk, strlen(path));
    msgpack_pack_raw_body(pk, path, strlen(path));

    zmq_msg_init_size(&msg, buffer->size);
    memcpy(zmq_msg_data(&msg), buffer->data, buffer->size);

    if (zlist_size(queue) > 0) {
        debug_print("We have messages in the worker queue\n", cli->fd);
    }

    //TODO workn on the (naive) retry mechanism
    zlist_t *temp = zlist_new();

    for(int i = 0; i < zlist_size(queue); i++) {
        pmsg = zlist_pop(queue);
        rc = zmq_sendmsg(publisher, pmsg, ZMQ_NOBLOCK);
        if (rc < 0) {
            zlist_append(temp, pmsg);
        } else {
            zmq_msg_close(pmsg);
        }
    }

    while((pmsg = zlist_pop(temp)))
        zlist_append(queue, pmsg);

    rc = zmq_sendmsg(publisher, &msg, ZMQ_NOBLOCK);
    if (rc < 0) { 
        if (zmq_errno() == EAGAIN) {
            debug_print("zmq send EAGAIN on %d .. queueing\n", cli->fd);
            
            int _rc = zlist_append(queue, &msg);

            if (_rc == 0) {
                debug_print("EAGAIN -> Queue\n", cli->fd);
            }

            //TODO refactor! :)
            msgpack_sbuffer_free(buffer);
            msgpack_packer_free(pk);

            return DISPATCH_AGAIN;
        }
        //rc = zmq_sendmsg(publisher, &msg, ZMQ_NOBLOCK);
    }
    debug_print("zeromq send return: %d\n",rc);

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
