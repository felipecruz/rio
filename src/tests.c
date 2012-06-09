/*
 * =====================================================================================
 *
 *       Filename:  tests.c
 *
 *    Description:  
 *
 *        Version:  0.1
 *        Created:  03/06/2012 10:09:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Felipe J Cruz (felipecruz@loogica.net), 
 *   Organization:  loogica
 *
 * =====================================================================================
 */

#include <stdlib.h>

#include "network.h"
#include "static.h"
#include "dispatch.h"
#include "buffer.h"
#include "websocket.h"

#include "CUnit/Basic.h"

int main()
{
    CU_pSuite static_suite = NULL;
    CU_pSuite dispatch_suite = NULL;
    CU_pSuite buffer_suite = NULL;
    CU_pSuite websocket_suite = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   static_suite = CU_add_suite("Static Suite", init_static_test_suite, NULL);
   if (NULL == static_suite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   dispatch_suite = CU_add_suite("Dispatch Suite", init_dispatch_test_suite, NULL);
   if (NULL == dispatch_suite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   buffer_suite = CU_add_suite("Buffer Suite", init_buffer_test_suite, NULL);
   if (NULL == buffer_suite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   websocket_suite = CU_add_suite("Websocket Suite", init_websocket_test_suite, NULL);
   if (NULL == websocket_suite) {
      CU_cleanup_registry();
      return CU_get_error();
   }


   /* add the tests to the suite */
   if ((NULL == CU_add_test(static_suite, "test of file_extension", 
                            test_file_extension)) ||
      (NULL == CU_add_test(static_suite, "test of mime_types", 
                            test_mime_types)) 
      )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }
  
   if ((NULL == CU_add_test(dispatch_suite, "test of is_filename", 
                            test_is_filename)))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   if ((NULL == CU_add_test(buffer_suite, "test buffer creation", 
                            test_buffer_new)) ||
        (NULL == CU_add_test(buffer_suite, "test buffer adjust",
                             test_buffer_adjust))
        )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }
 
   if ((NULL == CU_add_test(websocket_suite, "test websocket check end frame", 
                            test_websocket_check_end_frame)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket get frame type",
                            test_websocket_get_frame_type)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket check masked",
                            test_websocket_check_masked)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket get payload length",
                            test_websocket_get_payload_length)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket extract mask",
                            test_websocket_extract_mask)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket extract payload",
                            test_websocket_extract_payload)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket extract masked payload",
                            test_websocket_extract_masked_payload)) ||
       (NULL == CU_add_test(websocket_suite, "test websocket frames",
                            test_frames))

        )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }
 
 
   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
