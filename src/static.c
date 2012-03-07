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
khash_t(files) *f = NULL;

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
    cached_file *file;

    debug_print("Closing open files\n", f);
    
    for (element = kh_begin(f); element != kh_end(f); ++element) {
        *file = kh_val(f, element);
        close(file->fd);
        free(file);
    }

    kh_destroy(files, f);
}

void
    handle_static(client *cli, char *path) 
{
    int ret, num_read, missing, fd;
    
    const char pwd[] = ".";
    char *filename;
    char *response;
    char buf[1024];
     
    cached_file *file;
    khiter_t element;
    
    filename = strcat(pwd, path);

    element = kh_get(files, f, filename);
    missing = (element == kh_end(f));

    debug_print("Serving %s\n", filename);

    //file not opened
    if (!missing) {
        debug_print("%s file already opened. sending..\n", filename);
        //get from cache
        *file = kh_val(f, element);
        //seek to begin
        lseek(file->fd, 0, SEEK_SET);
    } else { 
        struct stat st;

        // open file
        fd = open(filename, O_RDONLY | O_DIRECT);
        if (fd == -1) {
            debug_print("Error opening file %s. 404 response\n", filename);
            write(cli->fd, 
                  default_four_zero_four, 
                  strlen(default_four_zero_four));
            close(cli->fd);
            return;
        }
        
        bzero(&st, sizeof(st));
        stat(filename, &st);

        debug_print("%s file opened\n", filename);
        new(file, cached_file);

        file->fd = fd;
        file->size = st.st_size;
        
        element = kh_put(files, f, filename, &ret);
        kh_value(f, element) = *file;

    }
    response = malloc(512 * sizeof(char));
    sprintf(response, default_static_response, (char*) mime_type(extension(filename)),
                                               (int)file->size);

    //check_write(
        write(
            cli->fd, 
            response, 
            strlen(response)
        );
    //);

    //while ((num_read = read(fileno(file), buf, 1024)) > 0)
    //    if (write(cli->fd, buf, num_read) != num_read)
    //        error_exit("write error on file copy");
    //    else
    //        total += num_read;

    sendfile(cli->fd, file->fd, NULL, file->size);

    write(
        cli->fd, 
        end_static_response, 
        strlen(end_static_response)
    );

    //if (!cli->keep_alive) {
        close(cli->fd);
    //}

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

char*
    mime_type(char *value)
{
    char text_html[] = "text/html";
    char text_css[] = "text/css";
    char app_json[] = "application/json";
    char app_js[] = "application/javascript";

    if (strcmp(value, "html") == 0) {
        return text_html;
    }
    else if (strcmp(value, "css") == 0) {
        return text_css;
    }
    else if (strcmp(value, "json") == 0) {
        return app_json;
    }
    else if (strcmp(value, "js") == 0) {
        return app_js;
    }

}

#if TEST
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
