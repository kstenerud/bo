#include <gtest/gtest.h>
#include <bo/bo.h>
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

static void assert_conversion(const char* input, const char* expected_output)
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

TEST(BO_Output, hex_1_no_prefix_no_suffix)
{
    assert_conversion("oh1l1 ih1l 1 2 3 4 a b cd", "1234abcd");
}

TEST(BO_Output, hex_1_no_prefix)
{
    assert_conversion("oh1l1 s\"|\" ih1l 1 2 3 4 a b cd", "1|2|3|4|a|b|cd");
}

TEST(BO_Output, hex_1_no_suffix)
{
    assert_conversion("oh1l1 p\" \" ih1l 1 2 3 4 a b cd", " 1 2 3 4 a b cd");
}

TEST(BO_Output, hex_1_2)
{
    assert_conversion("oh1l2 p\"0x\" s\", \" ih1l 1 2 3 4 a b cd", "0x01, 0x02, 0x03, 0x04, 0x0a, 0x0b, 0xcd");
}

TEST(BO_Output, hex_1_3)
{
    assert_conversion("oh1l3 p\"0x\" s\", \" ih1l 10 11 12", "0x010, 0x011, 0x012");
}

TEST(BO_Output, hex_1_4)
{
    assert_conversion("oh1l4 p\"0x\" s\", \" ih1l 10 11 12", "0x0010, 0x0011, 0x0012");
}

TEST(BO_Output, hex_2_4)
{
    assert_conversion("oh2l4 p\"0x\" s\", \" ih2l 1000 1100 1200", "0x1000, 0x1100, 0x1200");
}

TEST(BO_Output, hex_2_4_b)
{
    assert_conversion("oh2l4 p\"0x\" s\", \" ih1l 10 01 f8 99 12 43", "0x0110, 0x99f8, 0x4312");
}

TEST(BO_Output, hex_1_8)
{
    assert_conversion("oh4l8 p\"0x\" s\", \" ih1l 01 02 03 04", "0x04030201");
}

TEST(BO_Output, hex_1_16)
{
    assert_conversion("oh8l16 p\"0x\" s\", \" ih1l 01 02 03 04 05 06 07 08", "0x0807060504030201");
}

TEST(BO_Output, float_4)
{
    assert_conversion("of4l6 s\", \" ih1l 00 00 60 40 ", "3.500000");
}
