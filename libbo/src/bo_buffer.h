//  Copyright (c) 2018 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#ifndef bo_buffer_H
#define bo_buffer_H
#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
    uint8_t* start;
    uint8_t* end;
    uint8_t* pos;
    uint8_t* high_water;
} bo_buffer;


static inline bo_buffer buffer_alloc(int size, int overhead)
{
    uint8_t* memory = malloc(size + overhead);
    bo_buffer buffer =
    {
        .start = memory,
        .pos = memory,
        .end = memory + size + overhead,
        .high_water = memory + size,
    };
    return buffer;
}

static inline void buffer_free(bo_buffer* buffer)
{
    free(buffer->start);
    buffer->start = buffer->pos = buffer->end = NULL;
}

static inline bool buffer_is_high_water(bo_buffer* buffer)
{
    return buffer->pos >= buffer->high_water;
}

static inline bool buffer_is_initialized(bo_buffer* buffer)
{
    return buffer->start != NULL;
}

static inline bool buffer_is_empty(bo_buffer* buffer)
{
    return buffer->pos == buffer->start;
}

static inline uint8_t* buffer_get_start(bo_buffer* buffer)
{
    return buffer->start;
}

static inline uint8_t* buffer_get_position(bo_buffer* buffer)
{
    return buffer->pos;
}

static inline uint8_t* buffer_get_end(bo_buffer* buffer)
{
    return buffer->end;
}

static inline int buffer_get_used(bo_buffer* buffer)
{
    return buffer->pos - buffer->start;
}

static inline int buffer_get_remaining(bo_buffer* buffer)
{
    return buffer->end - buffer->pos;
}

static inline void buffer_set_position(bo_buffer* buffer, uint8_t* position)
{
    buffer->pos = position;
}

static inline void buffer_clear(bo_buffer* buffer)
{
    buffer->pos = buffer->start;
}

static inline void buffer_use_space(bo_buffer* buffer, int bytes)
{
    buffer->pos += bytes;
}

static inline void buffer_append_string(bo_buffer* buffer, const char* string)
{
    char* position = (char*)buffer_get_position(buffer);
    strncpy(position, string, buffer_get_remaining(buffer));
    buffer_use_space(buffer, strlen(position));
}

static inline void buffer_append_bytes(bo_buffer* buffer, const uint8_t* bytes, int length)
{
    memcpy(buffer_get_position(buffer), bytes, length);
    buffer_use_space(buffer, length);
}


#ifdef __cplusplus
}
#endif
#endif // bo_buffer_H
