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


#ifndef bo_H
#define bo_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * Callback to notify of new output data.
 *
 * @param user_data The user data object that was passed to the context that generated this call.
 * @param data The data.
 * @param length The length of the data in bytes.
 * @return true if the receiver of this message successfully processed it.
 */
typedef bool (*output_callback)(void* user_data, char* data, int length);

/**
 * Callback to notify that an error occured while processing input data.
 *
 * @param user_data The user data object that was passed to the context that generated this call.
 * @param message The error message.
 */
typedef void (*error_callback)(void* user_data, const char* message);

/**
 * Get the bo library version. Bo follows the semantic version format.
 *
 * @return The format string.
 */
const char* bo_version();

/**
 * Create a new bo context that calls a callback as it processes data.
 *
 * @param user_data User-specified contextual data.
 * @param on_output Called whenever there's processed output data.
 * @param on_error Called if an error occurs while processing.
 */
void* bo_new_callback_context(void* user_data, output_callback on_output, error_callback on_error);

/**
 * Create a new bo context that streams to a file.
 *
 * @param user_data User-specified contextual data.
 * @param output_stream The file stream to output processed data to.
 * @param on_error Called if an error occurs while processing.
 */
void* bo_new_stream_context(void* user_data, FILE* output_stream, error_callback on_error);

/**
 * Flushes a context's output and destroys the context.
 *
 * @param context The context object.
 * @return True if flushing was successful. Context destruction succeeds regardless of return value.
 */
bool bo_flush_and_destroy_context(void* context);

/**
 * Process a BO command sequence from a string.
 *
 * @param input The sequence to parse.
 * @param context The context object.
 * @return True if successful.
 */
// bool bo_process_data(void* context, const char* data, int length);
bool bo_process_string(void* void_context, const char* string);

/**
 * Process a BO command sequence from a stream.
 *
 * @param src The stream to read from.
 * @param context The context object.
 * @return True if successful.
 */
bool bo_process_stream(void* context, FILE* src);


#ifdef __cplusplus
}
#endif
#endif // bo_H
