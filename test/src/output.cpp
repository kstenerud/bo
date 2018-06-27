#include <gtest/gtest.h>
#include <bo/bo.h>
#include <stdio.h>
#include <stdarg.h>

static bool g_has_errors = false;

static void on_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
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

static void assert_conversion(bo_config* config, const char* input, const char* expected_output)
{
	reset_errors();
	char buffer[10000];
	int buffer_length = sizeof(buffer) - 1;
	int bytes_written = bo_process_string(input, buffer, buffer_length, config);
	ASSERT_GE(bytes_written, 0);
	ASSERT_FALSE(has_errors());
	buffer[bytes_written] = 0;
	ASSERT_STREQ(expected_output, buffer);
}

static bo_config new_config(bo_data_type data_type, int data_width, int text_width, int precision, const char* prefix, const char* suffix, bo_endianness endianness)
{
    bo_config config =
    {
    	.output =
    	{
	    	.data_type = data_type,
	    	.data_width = data_width,
	    	.text_width = text_width,
	    	.precision = precision,
	    	.prefix = prefix,
	    	.suffix = suffix,
	    	.endianness = endianness,
	    },
	    .on_error = on_error,
    };
    return config;
}

// o{type}{data-width}{endian}{text-width/precision}
// prefix, suffix
// p"{string}"
// s"{string}"
// if nothing, binary out

TEST(BO_Output, hex_1_no_prefix_no_suffix)
{
    bo_config config = new_config(TYPE_HEX, 1, 1, 0, "", "", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 1 2 3 4 a b cd", "1234abcd");
}

TEST(BO_Output, hex_1_no_prefix)
{
    bo_config config = new_config(TYPE_HEX, 1, 1, 0, "", "|", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 1 2 3 4 a b cd", "1|2|3|4|a|b|cd");
}

TEST(BO_Output, hex_1_no_suffix)
{
    bo_config config = new_config(TYPE_HEX, 1, 1, 0, " ", "", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 1 2 3 4 a b cd", " 1 2 3 4 a b cd");
}

TEST(BO_Output, hex_1_2)
{
    bo_config config = new_config(TYPE_HEX, 1, 2, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 1 2 3 4 a b cd", "0x01, 0x02, 0x03, 0x04, 0x0a, 0x0b, 0xcd");
}

TEST(BO_Output, hex_1_3)
{
    bo_config config = new_config(TYPE_HEX, 1, 3, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 10 11 12", "0x010, 0x011, 0x012");
}

TEST(BO_Output, hex_1_4)
{
    bo_config config = new_config(TYPE_HEX, 1, 4, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 10 11 12", "0x0010, 0x0011, 0x0012");
}

TEST(BO_Output, hex_2_4)
{
    bo_config config = new_config(TYPE_HEX, 2, 4, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih2l 1000 1100 1200", "0x1000, 0x1100, 0x1200");
}

TEST(BO_Output, hex_2_4_b)
{
    bo_config config = new_config(TYPE_HEX, 2, 4, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 10 01 f8 99 12 43", "0x0110, 0x99f8, 0x4312");
}

TEST(BO_Output, hex_1_8)
{
    bo_config config = new_config(TYPE_HEX, 4, 8, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 01 02 03 04", "0x04030201");
}

TEST(BO_Output, hex_1_16)
{
    bo_config config = new_config(TYPE_HEX, 8, 16, 0, "0x", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 01 02 03 04 05 06 07 08", "0x0807060504030201");
}

TEST(BO_Output, float_4)
{
    bo_config config = new_config(TYPE_FLOAT, 4, 0, 6, "", ", ", BO_ENDIAN_LITTLE);
    assert_conversion(&config, "ih1l 00 00 60 40 ", "3.500000");
}
