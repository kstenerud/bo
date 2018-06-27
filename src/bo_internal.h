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
    bo_data_type data_type;
    bo_data_width data_width;
    bo_buffer work_buffer;
    bo_buffer output_buffer;
    bo_config* config;
} bo_context;


bo_context bo_new_context(int work_buffer_size, uint8_t* output, int output_length, bo_config* config);

bool bo_on_string(bo_context* context, const char* string);
bool bo_on_number(bo_context* context, const char* string_value);
bool bo_set_data_type(bo_context* context, const char* string_value);

void bo_finish(bo_context* context);


#ifdef __cplusplus
}
#endif
#endif // bo_internal_H
