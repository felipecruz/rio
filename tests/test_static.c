#include "../src/static.h"
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
