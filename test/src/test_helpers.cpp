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
	int buffer_length = sizeof(buffer) - 1;
	int bytes_written = bo_process_string(input, buffer, buffer_length, on_error);
	ASSERT_GE(bytes_written, 0);
	ASSERT_FALSE(has_errors());
	buffer[bytes_written] = 0;
	ASSERT_STREQ(expected_output, buffer);
}

void assert_failed_conversion(const char* input, const char* expected_output)
{
	reset_errors();
	char buffer[10000];
	int buffer_length = sizeof(buffer) - 1;
	int bytes_written = bo_process_string(input, buffer, buffer_length, on_error);
	ASSERT_LT(bytes_written, 0);
	ASSERT_TRUE(has_errors());
}
