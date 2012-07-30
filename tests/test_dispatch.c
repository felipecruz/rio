#include "../src/dispatch.h"
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
