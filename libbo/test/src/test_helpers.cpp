#include "test_helpers.h"
#include <stdio.h>
#include <stdarg.h>


static bool g_has_errors = false;

static void on_error(void* user_data, const char* message)
{
	printf("BO Error: %s\n", message);
	g_has_errors = true;
}

typedef struct
{
	char* pos;

} test_context;

static bool on_output(void* user_data, char* data, int length)
{
	test_context* context = (test_context*)user_data;
	memcpy(context->pos, data, length);
	context->pos[length] = 0;
	context->pos += length;
	return true;
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
	test_context test_context =
	{
		.pos = buffer,
	};
	void* context = bo_new_callback_context(&test_context, on_output, on_error);
	bool process_success = bo_process_string(context, input);
	bool flush_success = bo_flush_and_destroy_context(context);
	ASSERT_TRUE(process_success);
	ASSERT_TRUE(flush_success);
	ASSERT_FALSE(has_errors());
	ASSERT_STREQ(expected_output, buffer);
}

void assert_failed_conversion(int buffer_length, const char* input)
{
	reset_errors();
	char buffer[10000];
	test_context test_context =
	{
		.pos = buffer,
	};
	void* context = bo_new_callback_context(&test_context, on_output, on_error);
	bool process_success = bo_process_string(context, input);
	bool flush_success = bo_flush_and_destroy_context(context);
	bool is_successful = process_success && flush_success;
	ASSERT_FALSE(is_successful);
	ASSERT_TRUE(has_errors());
}
