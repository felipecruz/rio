/*
 * =====================================================================================
 *
 *       Filename:  buffer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/28/2012 09:57:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <string.h>
#include "types.h"

rio_buffer *
    new_rio_buffer(void);

#if TEST
int
    init_buffer_test_suite(void);

void
    test_buffer_new(void);

void
    test_buffer_get_data(void);

void
    test_buffer_adjust(void);

#endif
