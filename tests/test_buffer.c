#include <string.h>
#include "../src/buffer.h"
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
