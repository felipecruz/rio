/*
 * =====================================================================================
 *
 *       Filename:  buffer.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/28/2012 09:42:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "buffer.h"
#include "utils.h"

#define B_SIZE 4096

rio_buffer *
    new_rio_buffer(void)
{
    rio_buffer *buffer;
    
    buffer = malloc(sizeof(rio_buffer));
    if (buffer == NULL) {
        error_exit("malloc error");
    }

    buffer->content = malloc(sizeof(char) * B_SIZE);
    if (buffer->content == NULL) {
        error_exit("malloc error");
    }

    buffer->length = 0;
    buffer->read_count = 0;

    return buffer;
}

rio_buffer *
    new_rio_buffer_size(size_t size)
{
    rio_buffer *buffer;
    
    buffer = malloc(sizeof(rio_buffer));
    if (buffer == NULL) {
        error_exit("malloc error");
    }

    buffer->content = malloc(sizeof(char) * size);
    if (buffer->content == NULL) {
        error_exit("malloc error");
    }

    buffer->length = 0;
    buffer->read_count = 0;

    return buffer;
}
void
    rio_buffer_free(rio_buffer **buffer)
{
    if ((*buffer)->content != NULL) {
        free((*buffer)->content);
    }
    free(*buffer);
    *buffer = NULL;
}

void
    rio_buffer_copy_data(rio_buffer *buffer, void *content, size_t len)
{
    memcpy(buffer->content, content, len);
    buffer->length = len;
}

char*
    rio_buffer_get_data(rio_buffer *buffer)
{
    if (buffer->length == 0) {
        return NULL;
    }
    return buffer->content;
}

void
    rio_buffer_adjust(rio_buffer *buffer, size_t to)
{
    int sub;

    sub = buffer->length - to;

    if (sub == 0) {
        buffer->length = 0;
        return;
    }

    buffer->content = realloc(buffer->content,
                              sub + 1);

    buffer->content = memcpy(buffer->content, 
                             buffer->content + to, 
                             sub);

    memcpy((void *) buffer->content + sub, (void *) "\0", 1);
    
    buffer->length = sub;
}

#if TEST
#include <string.h>
#include "CUnit/CUnit.h"
int
    init_buffer_test_suite(void)
{
    return 0;
}
void
    test_buffer_new(void)
{
    rio_buffer *buffer = NULL;

    CU_ASSERT(NULL == buffer);
    
    buffer = new_rio_buffer();

    CU_ASSERT(NULL != buffer);
    CU_ASSERT(NULL != buffer->content);

    rio_buffer_free(&buffer);
    CU_ASSERT(NULL == buffer);

}

void
    test_buffer_adjust(void)
{
    rio_buffer *buffer = NULL;

    CU_ASSERT(NULL == buffer);
    
    buffer = new_rio_buffer();

    rio_buffer_copy_data(buffer, "riowebserver", 
                                 strlen("riowebserver"));

    CU_ASSERT(0 == strcmp("riowebserver", rio_buffer_get_data(buffer)));    
    
    rio_buffer_adjust(buffer, 3);

    CU_ASSERT(0 == strcmp("webserver", rio_buffer_get_data(buffer)));    
    CU_ASSERT(strlen("webserver") == buffer->length);
    
    rio_buffer_adjust(buffer, 3);

    CU_ASSERT(0 == strcmp("server", rio_buffer_get_data(buffer)));    
    CU_ASSERT(strlen("server") == buffer->length);
    
    rio_buffer_adjust(buffer, 6);

    CU_ASSERT(NULL == rio_buffer_get_data(buffer));    
    CU_ASSERT(0 == buffer->length);
    
    rio_buffer_free(&buffer);
    CU_ASSERT(NULL == buffer);
    
}
#endif
