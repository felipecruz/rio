#include "dispatch.h"
#include "static.h"

char *default_response = 
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\n%s";
char *body = "<html><head><title>Test</title></head><body><h1>Response</h1><p>Response Test</p></body></html>"; 

const char *static_files_rg = "/(.+\\.[a-zA-Z]{1,3})";
char * dispatch(client *cli, const char *path) {   
    pcre *regexp;
    const char *error;
    int erroroffset, result=0;
    int ovector[30];
    char *buf, *p;
    const char *filename;

    regexp = pcre_compile(static_files_rg, 0, &error, &erroroffset, NULL);
    if (!regexp) {
        error_exit("Cant compile pcre regex");
    }
    
    result = pcre_exec(regexp, NULL, path, (int)strlen(path), 0, 0, ovector, 30);
    if (result > 0) {
        filename = malloc(sizeof(char) * (ovector[3] - ovector[2]) + 1);
        p = &path[ovector[2]];
        strcpy(filename, p); //, ovector[3]+1);
        //printf("loading: %s\n", filename);
        handle_static(cli,filename);
        buf = malloc(sizeof(char)*2);
        strcpy(buf,""); 
        free(filename);
    } else {
        buf = malloc(sizeof(char)*1024);
        sprintf(buf, default_response,strlen(body), body);
        
    }
    pcre_free(regexp);

    return buf;
}
