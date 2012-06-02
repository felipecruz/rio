/*
 * Copyright (c) 2010 Putilov Andrey
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is modified by Felipe Cruz <felipecruz@loogica.net>
 */

#include "websocket.h"
#include "utils.h"
#include <openssl/sha.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 1
#endif

static char rn[] PROGMEM = "\r\n";

void nullhandshake(struct handshake *hs)
{
    hs->host = NULL;
    hs->key1 = NULL;
    hs->key2 = NULL;
    hs->origin = NULL;
    hs->protocol = NULL;
    hs->resource = NULL;
}

static char* get_upto_linefeed(const char *start_from)
{
    char *write_to;
    uint8_t new_length = strstr_P(start_from, rn) - start_from + 1;
    assert(new_length);
    write_to = (char *)malloc(new_length); //+1 for '\x00'
    assert(write_to);
    memcpy(write_to, start_from, new_length - 1);
    write_to[ new_length - 1 ] = 0;

    return write_to;
}

enum ws_frame_type ws_parse_handshake(const uint8_t *input_frame, size_t input_len,
        struct handshake *hs)
{
    const char *input_ptr = (const char *)input_frame;
    const char *end_ptr = (const char *)input_frame + input_len;

    // measure resource size
    char *first = strchr((const char *)input_frame, ' ');
    if (!first)
        return WS_ERROR_FRAME;
    first++;
    char *second = strchr(first, ' ');
    if (!second)
        return WS_ERROR_FRAME;

    if (hs->resource) {
        free(hs->resource);
        hs->resource = NULL;
    }
    hs->resource = (char *)malloc(second - first + 1); // +1 is for \x00 symbol
    assert(hs->resource);

    if (sscanf_P(input_ptr, PSTR("GET %s HTTP/1.1\r\n"), hs->resource) != 1)
        return WS_ERROR_FRAME;
    input_ptr = strstr_P(input_ptr, rn) + 2;

    /*
        parse next lines
     */
    #define input_ptr_len (input_len - (input_ptr-input_frame))
    #define prepare(x) do {if (x) { free(x); x = NULL; }} while(0)
    uint8_t connection_flag = FALSE;
    uint8_t upgrade_flag = FALSE;
    while (input_ptr < end_ptr && input_ptr[0] != '\r' && input_ptr[1] != '\n') {
        if (memcmp_P(input_ptr, host, strlen_P(host)) == 0) {
            input_ptr += strlen_P(host);
            prepare(hs->host);
            hs->host = get_upto_linefeed(input_ptr);
        } else
            if (memcmp_P(input_ptr, origin, strlen_P(origin)) == 0) {
            input_ptr += strlen_P(origin);
            prepare(hs->origin);
            hs->origin = get_upto_linefeed(input_ptr);
        } else
            if (memcmp_P(input_ptr, protocol, strlen_P(protocol)) == 0) {
            input_ptr += strlen_P(protocol);
            prepare(hs->protocol);
            hs->protocol = get_upto_linefeed(input_ptr);
        } else
            if (memcmp_P(input_ptr, key, strlen_P(key)) == 0) {
            input_ptr += strlen_P(key);
            prepare(hs->key1);
            hs->key1 = get_upto_linefeed(input_ptr);
        } else
            if (memcmp_P(input_ptr, key1, strlen_P(key1)) == 0) {
            input_ptr += strlen_P(key1);
            prepare(hs->key1);
            hs->key1 = get_upto_linefeed(input_ptr);
        } else
            if (memcmp_P(input_ptr, key2, strlen_P(key2)) == 0) {
            input_ptr += strlen_P(key2);
            prepare(hs->key2);
            hs->key2 = get_upto_linefeed(input_ptr);
        } else
            if (memcmp_P(input_ptr, connection, strlen_P(connection)) == 0) {
            connection_flag = TRUE;
        } else
            if (memcmp_P(input_ptr, upgrade, strlen_P(upgrade)) == 0) {
            upgrade_flag = TRUE;
        };

        input_ptr = strstr_P(input_ptr, rn) + 2;
    }

    // we have read all data, so check them
    if (!hs->host || !hs->origin || //!hs->key1 || !hs->key2 ||
            !connection_flag || !upgrade_flag)
        return WS_ERROR_FRAME;

    input_ptr += 2; // skip empty line
    if (end_ptr - input_ptr < 8)
        return WS_INCOMPLETE_FRAME;
    memcpy(hs->key2, input_ptr, 8);

    return WS_OPENING_FRAME;
}

enum ws_frame_type ws_get_handshake_answer(const struct handshake *hs,
        uint8_t *out_frame, size_t *out_len)
{
    assert(out_frame);

    SHA_CTX sha;
    SHA1_Init(&sha);

    char *pre_key = strcat(hs->key1, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    SHA1_Update(&sha, pre_key, strlen(pre_key));
    
    debug_print("##### BaseKey: %s\n", pre_key);

    unsigned char accept_key[30];
    unsigned char digest_key[20];

    SHA1_Final(digest_key, &sha);

    debug_print("##### DigestKey: %s\n", digest_key);

    lws_b64_encode_string(digest_key , strlen(digest_key), accept_key, 
                          sizeof(accept_key));

    debug_print("##### AcceptKey: %s\n", accept_key);

    unsigned int written = sprintf_P((char *)out_frame,
            PSTR("HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: %s\r\n\r\n"),
                    accept_key);
    if (hs->protocol)
        written += sprintf_P((char *)out_frame + written,
            PSTR("Sec-WebSocket-Protocol: %s\r\n"), hs->protocol);
    written += sprintf_P((char *)out_frame + written, rn);
    *out_len = written; 
    return WS_OPENING_FRAME;
}

enum ws_frame_type ws_make_frame(const uint8_t *data, size_t data_len,
        uint8_t *out_frame, size_t *out_len, enum ws_frame_type frame_type)
{
    assert(out_frame);
    assert(data);
    
    if (frame_type == WS_TEXT_FRAME) {
        
        uint8_t *data_ptr = (uint8_t *) data;
        uint8_t *end_ptr = (uint8_t *) data + data_len;
        out_frame[0] = 1 | 1 << 5; // 129; //(1 << 1 | 1 << 5);
        out_frame[1] = 1;
        out_frame[1] |= 3; //(1 << 1 | 1 << 2);
        memcpy(&out_frame[2], data, data_len);
        *out_len = data_len + 2;

    } else if (frame_type == WS_BINARY_FRAME) {
        size_t tmp = data_len;
        uint8_t out_size_buf[sizeof (size_t)];
        uint8_t *size_ptr = out_size_buf;
        while (tmp <= 0xFF) {
            *size_ptr = tmp & 0x7F;
            tmp >>= 7;
            size_ptr++;
        }
        *size_ptr = tmp;
        uint8_t size_len = size_ptr - out_size_buf + 1;

        assert(*out_len >= 1 + size_len + data_len);
        out_frame[0] = '\x80';
        uint8_t i = 0;
        for (i = 0; i < size_len; i++) // copying big-endian length
            out_frame[1 + i] = out_size_buf[size_len - 1 - i];
        memcpy(&out_frame[1 + size_len], data, data_len);
    }

    for (int i = 0; i < *out_len; i++) { 
        printf("%u ", out_frame[i]);
    }
    printf("\n");

    return frame_type;
}

enum ws_frame_type ws_parse_input_frame(const uint8_t *input_frame, size_t input_len,
        uint8_t **out_data_ptr, size_t *out_len)
{
    enum ws_frame_type frame_type;

    assert(out_len); 
    assert(input_len);

    if (input_len < 2)
        return WS_INCOMPLETE_FRAME;

    uint8_t fin_srv_opcode = input_frame[0];
    uint8_t mask_length = input_frame[1];
    uint8_t mask[4] = {0x00, 0x00, 0x00, 0x00};
    int c = 0;

    memcpy(mask, input_frame + 2, 4);

    uint8_t *ini = input_frame + 7; 

    while(*ini) {
        *ini ^= mask[c % 4];
        *ini++;
        c++;
    }

    /* uint8_t *end = (uint8_t *) memchr(data_start, 0xFF, input_len - 1);
    if (end) {
        assert((size_t)(end - data_start) <= input_len);
        *out_data_ptr = (uint8_t *)data_start;
        *out_len = end - data_start;
        frame_type = WS_TEXT_FRAME;
    } else {
        frame_type = WS_INCOMPLETE_FRAME;
    }*/

    return frame_type;
}
