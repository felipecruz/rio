#include "dispatch.h"
#include "static.h"

char *default_response = 
    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
    "Content-Length: %d\r\n\n%s";

char *body = 
    "<html><head><title>Test</title></head><body><h1>Response</h1>"
    "<p>Response Test</p></body></html>"; 

char* 
    dispatch(client *cli, const char *path) 
{   
    char *buf;
    
    debug_print("Request URI %s\n", path);

    if (is_filename(path)) {
        handle_static(cli, path);
        buf = NULL;
    } else {
        buf = malloc(sizeof(char) * 1024);
        sprintf(buf, default_response,strlen(body), body);
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
