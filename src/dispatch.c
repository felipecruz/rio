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

char* 
    dispatch(client *cli, const char *path) 
{   
    char *buf;
    
    debug_print("Request URI %s\n", path);

    if (is_filename(path)) {
        handle_static(cli, path);
        buf = NULL;
    } else {
        int rc;
        zmq_msg_t msg, msg2;
        char identity[3] = "rio";

        msgpack_unpacked unpacked;
        msgpack_unpacked_init(&unpacked);

        msgpack_sbuffer* buffer = msgpack_sbuffer_new();
        msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

        msgpack_pack_array(pk, 2);
        msgpack_pack_int(pk, cli->method);
        msgpack_pack_raw(pk, strlen(path));
        msgpack_pack_raw_body(pk, path, strlen(path));

        zmq_msg_init_size(&msg, buffer->size);
        memcpy(zmq_msg_data(&msg), buffer->data, buffer->size);

        //zmq_setsockopt(publisher, ZMQ_IDENTITY, identity, sizeof(char) * 3 );

        rc = zmq_send(publisher, &msg, 0);
        debug_print("zeromq bytes sent: %d\n",rc);
        
        //zmq_msg_init(&out);
        //rc = zmq_recv(publisher, &out, 0);

        //msgpack_unpack_next(&unpacked, 
        //                    zmq_msg_data(&out), 
        //                    zmq_msg_size(&out),
        //                    NULL);

        //msgpack_object obj = unpacked.data;
        //msgpack_object_print(stdout, obj);

        size_t offset = 0;

        zmq_msg_init(&msg2);
        rc = zmq_recv(publisher, &msg2, 0);
 
        debug_print("zeromq received raw: %s\nsize %d\n", 
                zmq_msg_data(&msg2), zmq_msg_size(&msg2));

        msgpack_unpack_next(&unpacked, 
                            zmq_msg_data(&msg2), 
                            zmq_msg_size(&msg2),
                            &offset);

        /* debug_print("unpacked - status: %d\n", 
                unpacked.data.via.u64);

        msgpack_unpack_next(&unpacked, 
                            zmq_msg_data(&msg2), 
                            zmq_msg_size(&msg2),
                            &offset);

        debug_print("unpacked: %s\n", 
                unpacked.data.via.raw.ptr);

        msgpack_unpack_next(&unpacked, 
                            zmq_msg_data(&msg2), 
                            zmq_msg_size(&msg2),
                            &offset); */ 

        debug_print("unpacked %s\n", 
                unpacked.data.via.raw.ptr);

        buf = malloc(sizeof(char) * 1024);
        sprintf(buf, 
                default_response, 
                strlen(unpacked.data.via.raw.ptr), 
                unpacked.data.via.raw.ptr);
    }
    return buf;
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
