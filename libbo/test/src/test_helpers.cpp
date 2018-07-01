#include "test_helpers.h"
#include <stdio.h>
#include <stdarg.h>


static bool g_has_errors = false;

static void on_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("BO Error: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	g_has_errors = true;
}

static bool has_errors()
{
	return g_has_errors;
}

static void reset_errors()
{
	g_has_errors = false;
}

void assert_conversion(const char* input, const char* expected_output)
{
	reset_errors();
	char buffer[10000];
	void* context = bo_new_buffer_context((uint8_t*)buffer, sizeof(buffer), on_error);
	int bytes_written = bo_process_string(input, context);
	int bytes_flushed = bo_flush_output(context);
	bo_destroy_context(context);
	ASSERT_GE(bytes_written, 0);
	ASSERT_GE(bytes_flushed, 0);
	ASSERT_FALSE(has_errors());
	buffer[bytes_written + bytes_flushed] = 0;
	ASSERT_STREQ(expected_output, buffer);
}

void assert_failed_conversion(int buffer_length, const char* input)
{
	reset_errors();
	char buffer[buffer_length];
	void* context = bo_new_buffer_context((uint8_t*)buffer, sizeof(buffer), on_error);
	int bytes_written = bo_process_string(input, context);
	int bytes_flushed = bo_flush_output(context);
	bool is_successful = bytes_written >= 0 && bytes_flushed >= 0;
	bo_destroy_context(context);
	ASSERT_FALSE(is_successful);
	ASSERT_TRUE(has_errors());
}
