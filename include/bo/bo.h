#ifndef bo_H
#define bo_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef void (*error_callback)(const char* fmt, ...);

const char* bo_version();

/**
 * Escapes a string in-place (modifies the original string).
 *
 * @param str The string to escape (this string may get modified).
 * @return pointer to the offending character on failure, pointer to the end of the string (\0) on success.
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
int bo_process_string(const char* input, char* output, int output_length, error_callback on_error);


#ifdef __cplusplus
}
#endif
#endif // bo_H
