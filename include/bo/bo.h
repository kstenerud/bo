#ifndef bo_H
#define bo_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>


typedef enum
{
    TYPE_NONE    = 0,
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
    BO_ENDIAN_LITTLE = 0,
    BO_ENDIAN_BIG = 1,
} bo_endianness;

typedef struct
{
	struct
	{
	    bo_data_type data_type;
    	int data_width;
    	int text_width;
    	int precision;
    	const char* prefix;
    	const char* suffix;
    	bo_endianness endianness;
    } output;
    void (*on_error)(const char* fmt, ...);
} bo_config;

const char* bo_version();

/**
 * Escapes a string in-place (modifies the original string).
 *
 * @param str The string to escape (this string may get modified).
 * @return NULL if successful. On failure, a pointer to the offending escape sequence in the string.
 */
char* bo_unescape_string(char* str);

/**
 * Process a BO command sequence from a string.
 *
 * @param input The sequence to parse.
 * @param output A buffer to hold the result.
 * @param output_length The length of the output buffer.
 * @param config The configuration to use.
 * @return The number of bytes written to the output buffer, or -1 if an error occurred.
 */
int bo_process_string(const char* input, char* output, int output_length, bo_config* config);


#ifdef __cplusplus
}
#endif
#endif // bo_H
