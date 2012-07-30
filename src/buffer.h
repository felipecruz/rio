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

rio_buffer *
    new_rio_buffer_size(size_t);

void
    rio_buffer_free(rio_buffer **buffer);

char *
    rio_buffer_get_data(rio_buffer *buffer);

void
    rio_buffer_copy_data(rio_buffer *buffer, void *content, size_t len);

void
    rio_buffer_adjust(rio_buffer *buffer, size_t to);
