#ifndef bo_H
#define bo_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef void (*error_callback)(const char* fmt, ...);

const char* bo_version();

/**
 * Escapes a string in-place (modifies the original string).
 *
 * @param str The string to escape (this string may get modified).
 * @return pointer to the offending character on failure, pointer to the end of the string (\0) on success.
 */
char* bo_unescape_string(char* str);

void* bo_new_buffer_context(uint8_t* output_buffer, int output_buffer_length, error_callback on_error);

void* bo_new_stream_context(FILE* output_stream, error_callback on_error);

/**
 * Flushes a context's output and destroys the context.
 */
void bo_destroy_context(void* context);

/**
 * Process a BO command sequence from a string.
 *
 * @param input The sequence to parse.
 * @param context The context object.
 * @return The number of bytes written to the output buffer, or -1 if an error occurred.
 */
int bo_process_string(const char* input, void* context);

/**
 * Process a BO command sequence from a stream.
 *
 * @param src The stream to read from.
 * @param context The context object.
 * @return True if successful.
 */
bool bo_process_stream(FILE* src, void* context);

/**
 * Flush the output stream.
 * You need to do this to get the last kb or so of output.
 *
 * @param context The context object.
 * @return the number of bytes flushed, or -1 if an error occurred.
 */
int bo_flush_output(void* context);

#ifdef __cplusplus
}
#endif
#endif // bo_H
