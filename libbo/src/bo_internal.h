#ifndef bo_internal_H
#define bo_internal_H
#ifdef __cplusplus
extern "C" {
#endif


#include "bo/bo.h"
#include <stdbool.h>
#include <stdint.h>


#define BO_IS_LE_INT (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define BO_IS_LE_FLOAT (__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#define BO_NATIVE_INT_ENDIANNESS (((__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) * BO_ENDIAN_LITTLE) + \
                                  ((__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) * BO_ENDIAN_BIG))
#define BO_NATIVE_FLOAT_ENDIANNESS (((__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__) * BO_ENDIAN_LITTLE) + \
                                    ((__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__) * BO_ENDIAN_BIG))

typedef enum
{
    TYPE_NONE    = 0,
    TYPE_BINARY  = 'B',
    TYPE_INT     = 'i',
    TYPE_HEX     = 'h',
    TYPE_OCTAL   = 'o',
    TYPE_BOOLEAN = 'b',
    TYPE_FLOAT   = 'f',
    TYPE_DECIMAL = 'd',
    TYPE_STRING  = 's',
} bo_data_type;

typedef enum
{
    BO_ENDIAN_LITTLE = 'l',
    BO_ENDIAN_BIG = 'b',
} bo_endianness;

typedef enum
{
    WIDTH_1  =  1,
    WIDTH_2  =  2,
    WIDTH_4  =  4,
    WIDTH_8  =  8,
    WIDTH_16 = 16,
} bo_data_width;

typedef struct
{
    uint8_t* start;
    uint8_t* end;
    uint8_t* pos;
} bo_buffer;

typedef struct
{
    bo_buffer work_buffer;
    bo_buffer output_buffer;
    FILE* output_stream;
    struct
    {
        bo_data_type data_type;
        bo_data_width data_width;
        bo_endianness endianness;
    } input;
    struct
    {
        bo_data_type data_type;
        int data_width;
        int text_width;
        const char* prefix;
        const char* suffix;
        bo_endianness endianness;
    } output;
    error_callback on_error;
} bo_context;


bo_context bo_new_context(int work_buffer_size, uint8_t* output, int output_length, FILE* output_stream, error_callback on_error);


bool bo_on_string(bo_context* context, const char* string);
bool bo_on_number(bo_context* context, const char* string_value);
bool bo_set_input_type(bo_context* context, const char* string_value);
bool bo_set_output_type(bo_context* context, const char* string_value);
bool bo_set_prefix(bo_context* context, const char* string_value);
bool bo_set_suffix(bo_context* context, const char* string_value);

bool bo_finish(bo_context* context);


#ifdef __cplusplus
}
#endif
#endif // bo_internal_H
