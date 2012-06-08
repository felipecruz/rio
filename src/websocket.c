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
#include "b64.h"
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

int
    _end_frame(uint8_t *packet)
{
    return (packet[0] & 0x80) == 0x80;
}

enum ws_frame_type
    _type(uint8_t *packet)
{
    int opcode = packet[0] & 0xf;

    if (opcode == 0x01) {
        return WS_TEXT_FRAME;
    } else if (opcode == 0x00) {
        return WS_INCOMPLETE_FRAME;
    } else if (opcode == 0x02) {
        return WS_BINARY_FRAME;
    } else if (opcode == 0x08) {
        return WS_CLOSING_FRAME;
    }

}

int
    _masked(uint8_t *packet)
{
    return (packet[1] >> 7) & 0x1;
}

uint64_t
    f_uint8(uint8_t *value)
{
    uint64_t length = 0;

    for (int i = 0; i < 4; i++) {
        length = (length << 8) | value[i];
    }

    return length;
}

uint64_t
    _payload_length(uint8_t *packet)
{
    int length = -1;

    if (_masked(packet)) {
        length = packet[1] &= ~(1 << 7);
    } else {
        length = packet[1];
    }

    if (length < 125) {
        return length;
    } else if (length == 126) {
        uint16_t u16l = 0;
        memcpy((char*) &u16l, &packet[2], 2);
        return u16l;
    } else if (length == 127) {
        return f_uint8(&packet[2]);
    } else {
        return length;
    }
}

uint8_t*
    _extract_mask_len1(const uint8_t *packet)
{
    uint8_t *mask;
    int j = 0;

    mask = malloc(sizeof(uint8_t) * 4);

    for(int i = 2; i < 6; i++) {
        mask[j] = packet[i];
        j++;
    }

    return mask;
}
uint8_t*
    unmask(uint8_t *packet, uint64_t length, const uint8_t *mask)
{
    int i = 0;

    for (int i = 0; i < length; i++) {
        packet[i] ^= mask[i % 4];
    }

    return packet;
}

uint8_t*
    extract_payload(uint8_t *packet)
{
    uint8_t *data;
    uint8_t *mask; 
    int m = _masked(packet);
    uint64_t length = _payload_length(packet);

    if (m == 1) {
        printf("masked");
        if (length < 126) {
            mask = _extract_mask_len1(packet);
            return unmask(&packet[6], length, mask);
        }
        return NULL;
    } else {
        if (length < 126) {
            return &packet[2];
        }
        return NULL;
    }
    return NULL;

}

#if TEST
#include "CUnit/CUnit.h"
int
    init_websocket_test_suite(void)
{
    return 0;
}

/*
 o  A single-frame unmasked text message
      *  0x81 0x05 0x48 0x65 0x6c 0x6c 0x6f (contains "Hello")
   o  A single-frame masked text message
      *  0x81 0x85 0x37 0xfa 0x21 0x3d 0x7f 0x9f 0x4d 0x51 0x58
         (contains "Hello")
   o  A fragmented unmasked text message
      *  0x01 0x03 0x48 0x65 0x6c (contains "Hel")
      *  0x80 0x02 0x6c 0x6f (contains "lo")
   o  Unmasked Ping request and masked Ping response
      *  0x89 0x05 0x48 0x65 0x6c 0x6c 0x6f (contains a body of "Hello",
         but the contents of the body are arbitrary)
      *  0x8a 0x85 0x37 0xfa 0x21 0x3d 0x7f 0x9f 0x4d 0x51 0x58
         (contains a body of "Hello", matching the body of the ping)
   o  256 bytes binary message in a single unmasked frame
      *  0x82 0x7E 0x0100 [256 bytes of binary data]
   o  64KiB binary message in a single unmasked frame
      *  0x82 0x7F 0x0000000000010000 [65536 bytes of binary data]

*/

void
    test_websocket_parse_input(void)
{
    uint8_t single_frame[] = {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f};

    uint8_t first_frame[] = {0x01, 0x03, 0x48, 0x65, 0x6c};
    uint8_t second_frame[] = {0x80, 0x02, 0x6c, 0x6f};

    uint8_t single_frame_masked[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f,
                                     0x9f, 0x4d, 0x51, 0x58};

    uint8_t len_256[] = {0x82, 0x7E, 0x0100, 0x1, 0x1, 0x1, 0x1};
    uint8_t len_64k[] = {0x82, 0x7F, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00,
                         0x00, 0x00, 0x00, 0x00};

    uint8_t mask[4] = {0x37, 0xfa, 0x21, 0x3d};

//    CU_ASSERT(1 == _end_frame(&single_frame));
//    CU_ASSERT(0 == _end_frame(&first_frame));
//    CU_ASSERT(1 == _end_frame(&second_frame));
//    CU_ASSERT(1 == _end_frame(&single_frame_masked));
//
//    CU_ASSERT(WS_TEXT_FRAME == _type(&single_frame));
//    CU_ASSERT(WS_TEXT_FRAME == _type(&first_frame));
//    CU_ASSERT(WS_TEXT_FRAME != _type(&second_frame));
//    CU_ASSERT(WS_INCOMPLETE_FRAME == _type(&second_frame));
//    CU_ASSERT(WS_TEXT_FRAME == _type(&single_frame_masked));
//    CU_ASSERT(WS_BINARY_FRAME == _type(&len_256));
//    CU_ASSERT(WS_BINARY_FRAME == _type(&len_64k));
//
//    CU_ASSERT(0 == _masked(&single_frame));
//    CU_ASSERT(0 == _masked(&first_frame));
//    CU_ASSERT(0 == _masked(&second_frame));
//    CU_ASSERT(1 == _masked(&single_frame_masked));
//    CU_ASSERT(0 == _masked(&len_256));
//    CU_ASSERT(0 == _masked(&len_64k));
//
//    CU_ASSERT(5 == _payload_length(&single_frame));
//    CU_ASSERT(3 == _payload_length(&first_frame));
//    CU_ASSERT(2 == _payload_length(&second_frame));
//    CU_ASSERT(5 == _payload_length(&single_frame_masked));
//
//    CU_ASSERT(256 == _payload_length(&len_256));
//    CU_ASSERT(65536 == _payload_length(&len_64k));
//
//    CU_ASSERT(0 == strncmp((char*)_extract_mask_len1(&single_frame_masked),
//                           (char*) mask, 4));
//
//    CU_ASSERT(0 == strncmp((char*) extract_payload(&single_frame),
//                            "Hello", 5));
//
    CU_ASSERT(0 == strncmp((char*) extract_payload(&single_frame_masked),
                            "Hello", 5));
}

#endif
