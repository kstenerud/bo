#include "test_helpers.h"
#include <stdio.h>
#include <stdarg.h>


#define ENABLE_LOGGING 1

#if ENABLE_LOGGING
#define LOG(...) do {fprintf(stdout, "LOG: ");fprintf(stdout, __VA_ARGS__);fprintf(stdout, "\n");fflush(stdout);} while(0)
#else
#define LOG(FMT, ...)
#endif


static bool g_has_errors = false;

static void on_error(void* user_data, const char* message)
{
	printf("BO Error: %s\n", message);
	fflush(stdout);
	g_has_errors = true;
}

typedef struct
{
	char* pos;

} test_context;

static test_context new_test_context(char* buffer)
{
	test_context context =
	{
		.pos = buffer
	};
	*context.pos = 0;
	return context;
}

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

char* process_and_terminate(void* context, char* data, int data_length, bo_data_segment_type data_segment_type)
{
	char* result = bo_process(context, data, data_length, data_segment_type);
	if(result != NULL)
	{
		*result = 0;
	}
	return result;
}

static bool check_processed_all_data(void* context, char* data, int data_length, bo_data_segment_type data_segment_type)
{
	char* result = process_and_terminate(context, data, data_length, data_segment_type);
	return result == data + data_length;
}

void assert_conversion(const char* input, const char* expected_output)
{
	reset_errors();
	char buffer[10000];
	test_context test_context = new_test_context(buffer);
	char* input_copy = strdup(input);
	void* context = bo_new_context(&test_context, on_output, on_error);
	bool process_success = check_processed_all_data(context, input_copy, strlen(input_copy), DATA_SEGMENT_LAST);
	bool flush_success = bo_flush_and_destroy_context(context);
	ASSERT_TRUE(process_success);
	ASSERT_TRUE(flush_success);
	ASSERT_FALSE(has_errors());
	ASSERT_STREQ(expected_output, buffer);
	free((void*)input_copy);
}

void assert_spanning_conversion(const char* input, int split_point, int expected_offset, const char* expected_output)
{
	reset_errors();
	char buffer[10000];
	test_context test_context = new_test_context(buffer);
	char* input_copy = strdup(input);
	void* context = bo_new_context(&test_context, on_output, on_error);
	char* processed_to = process_and_terminate(context, input_copy, split_point, DATA_SEGMENT_STREAM);
	bool flush_success = bo_flush_and_destroy_context(context);
	ASSERT_TRUE(flush_success);
	ASSERT_TRUE(processed_to != NULL);
	char* expected_ptr = input_copy + expected_offset;
	ASSERT_EQ(expected_ptr, processed_to);
	ASSERT_FALSE(has_errors());
	ASSERT_STREQ(expected_output, buffer);
	free((void*)input_copy);
}

void assert_spanning_continuation(const char* input, int split_point, int expected_offset, const char* expected_output)
{
	reset_errors();
	char buffer[10000];
	test_context test_context = new_test_context(buffer);
	char* input_copy = strdup(input);
	void* context = bo_new_context(&test_context, on_output, on_error);
	char* processed_to = bo_process(context, input_copy, split_point, DATA_SEGMENT_STREAM);
	ASSERT_TRUE(processed_to != NULL);
	char* expected_ptr = input_copy + expected_offset;
	ASSERT_EQ(expected_ptr, processed_to);
	ASSERT_FALSE(has_errors());

	memmove(input_copy, processed_to, strlen(processed_to) + 1);
	bool process_success = check_processed_all_data(context, input_copy, strlen(input_copy), DATA_SEGMENT_LAST);
	bool flush_success = bo_flush_and_destroy_context(context);
	ASSERT_TRUE(process_success);
	ASSERT_TRUE(flush_success);

	ASSERT_STREQ(expected_output, buffer);
	free((void*)input_copy);
}

void assert_failed_conversion(int buffer_length, const char* input)
{
	reset_errors();
	char buffer[10000];
	test_context test_context = new_test_context(buffer);
	char* input_copy = strdup(input);
	void* context = bo_new_context(&test_context, on_output, on_error);
	bool process_success = check_processed_all_data(context, input_copy, strlen(input_copy), DATA_SEGMENT_LAST);
	bool flush_success = bo_flush_and_destroy_context(context);
	bool is_successful = process_success && flush_success;
	ASSERT_FALSE(is_successful);
	ASSERT_TRUE(has_errors());
	free((void*)input_copy);
}
