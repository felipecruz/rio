#include "static.h"

char *default_static_response =  
    "HTTP/1.1 200 OK\r\nContent-Type: %s;"
    "charset=utf-8\r\nContent-Length: %d\r\n\n";

char *default_four_zero_four = 
    "HTTP/1.1 404 NotFound\r\nContent-Type: text/html;"
    "charset=utf-8\r\nContent-Length: 47\r\n\n"
    "<!doctype html><body><h1>404</h1></body></html>";
   
char *end_static_response = "\n\n";

KHASH_MAP_INIT_STR(files, cached_file)
khash_t(files) *f;

void
    init_static_server()
{
    debug_print("Creating open files map\n", f);
    f = kh_init(files);
}

void 
    destroy_static_server()
{
    khiter_t element;

    debug_print("Closing open files\n", f);
    
    for (element = kh_begin(f); element != kh_end(f); ++element) {
        if (kh_exist(f, element)) {
            kh_del(files, f, element);
        }
    }

    kh_destroy(files, f);
}

void
    handle_static(rio_client *cli, char *path) 
{
    int ret, fd;
 
    char *filename;
    char *response;
    char *fextension;
     
    //key must be 'duplicated' in order to gain another memory
    //address
    filename = malloc(sizeof(char) * (strlen(".") + strlen(path) + 1));
    if (filename == NULL) {
        error_exit("malloc error");
    }

    strcpy(filename, ".");
    strcat(filename, path);

    debug_print("Serving %s\n", filename);
    //file not opened
    struct stat st;

    // open file
    fd = open(filename, O_RDONLY | O_DIRECT);
    if (fd == -1) {
        debug_print("Error opening file %s. 404 response\n", filename);
        ret = write(cli->fd, 
                    default_four_zero_four, 
                    strlen(default_four_zero_four));
        close(cli->fd);

        free(filename);
        return;
    }
    
    bzero(&st, sizeof(st));
    stat(filename, &st);

    debug_print("%s file opened\n", filename);

    response = malloc(512 * sizeof(char));

    fextension = extension(filename);

    debug_print("extension: %s\n", fextension);
    debug_print("mime type: %s\n", mime_type(fextension));

    sprintf(response, 
            default_static_response, 
            (char*) mime_type(fextension),
            (int) st.st_size);

    ret = write(cli->fd, 
                response, 
                strlen(response));
    
    sendfile(cli->fd, fd, NULL, st.st_size);
    close(cli->fd);
    close(fd);
    
    free(filename);
    free(response);

    return;
}

char*
    extension(char *value)
{
    char *p;
    
    p = strtok(value, ".");
    if (p == NULL) {
        return NULL;
    }

    p = strtok(NULL, "");

    return p;
}

const char*
    mime_type(char *value)
{
    if (strcmp(value, "html") == 0) {
        return "text/html\0";
    }
    else if (strcmp(value, "css") == 0) {
        return "text/css\0";
    }
    else if (strcmp(value, "json") == 0) {
        return "application/json\0";
    }
    else if (strcmp(value, "js") == 0) {
        return "application/javascript\0";
    }
    else if (strcmp(value, "png") == 0) {
        return "image/png\0";
    }
    else if (strcmp(value, "jpg") == 0) {
        return "image/jpg\0";
    }



}

#if TEST
#include "CUnit/CUnit.h"
int
    init_static_test_suite(void)
{
    return 0;    
}

void
    test_file_extension(void)
{
    char test1[] = "index.html";
    char test2[] = "rio.c";

    CU_ASSERT_STRING_EQUAL(extension(test1), "html");
    CU_ASSERT_STRING_EQUAL(extension(test2), "c");

    return;
}
void
    test_mime_types(void)
{
    char test1[] = "html";
    char test2[] = "js";
    char test3[] = "css";
    char test4[] = "json";
    
    CU_ASSERT_STRING_EQUAL(mime_type(test1), "text/html");
    CU_ASSERT_STRING_EQUAL(mime_type(test2), "application/javascript");
    CU_ASSERT_STRING_EQUAL(mime_type(test3), "text/css");
    CU_ASSERT_STRING_EQUAL(mime_type(test4), "application/json");

}
#endif
