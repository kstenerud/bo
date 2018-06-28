#include "test_helpers.h"

TEST(BO_Output, hex_1_1_le_no_prefix_no_suffix)
{
    assert_conversion("oh1l1 ih1l 1 2 3 4 a b cd", "1234abcd");
}

TEST(BO_Output, hex_1_1_le_no_prefix)
{
    assert_conversion("oh1l1 s\"|\" ih1l 1 2 3 4 a b cd", "1|2|3|4|a|b|cd");
}

TEST(BO_Output, hex_1_1_le_no_suffix)
{
    assert_conversion("oh1l1 p\" \" ih1l 1 2 3 4 a b cd", " 1 2 3 4 a b cd");
}

TEST(BO_Output, hex_1_1_2_le)
{
    assert_conversion("oh1l2 p\"0x\" s\", \" ih1l 1 2 3 4 a b cd", "0x01, 0x02, 0x03, 0x04, 0x0a, 0x0b, 0xcd");
}

TEST(BO_Output, hex_1_1_3_le)
{
    assert_conversion("oh1l3 p\"0x\" s\", \" ih1l 10 11 12", "0x010, 0x011, 0x012");
}

TEST(BO_Output, hex_1_1_4_le)
{
    assert_conversion("oh1l4 p\"0x\" s\", \" ih1l 10 11 12", "0x0010, 0x0011, 0x0012");
}

TEST(BO_Output, hex_2_2_4_le)
{
    assert_conversion("oh2l4 p\"0x\" s\", \" ih2l 1000 1100 1200", "0x1000, 0x1100, 0x1200");
}

TEST(BO_Output, hex_1_2_4_le_b)
{
    assert_conversion("oh2l4 p\"0x\" s\", \" ih1l 10 01 f8 99 12 43", "0x0110, 0x99f8, 0x4312");
}

TEST(BO_Output, hex_1_4_8_le)
{
    assert_conversion("oh4l8 p\"0x\" s\", \" ih1l 01 02 03 04", "0x04030201");
}

TEST(BO_Output, hex_1_8_16_le)
{
    assert_conversion("oh8l16 p\"0x\" s\", \" ih1l 01 02 03 04 05 06 07 08", "0x0807060504030201");
}

TEST(BO_Output, float_1_4_6_le)
{
    assert_conversion("of4l6 s\", \" ih1l 00 00 60 40", "3.500000");
}

TEST(BO_Output, float_8_8_3_le)
{
    assert_conversion("of8l3 s\", \" ih8l 4052880000000000", "74.125");
}

TEST(BO_Output, hex_1_2_4_be)
{
    assert_conversion("oh2b4 s\" \" ih1l 10 00 11 00 12 00", "1000 1100 1200");
}

TEST(BO_Output, hex_2_2_4_be)
{
    assert_conversion("oh2b4 s\" \" ih2l 1234 5678", "3412 7856");
}

TEST(BO_Output, hex_4_4_8_be)
{
    assert_conversion("oh4b8 s\" \" ih4l 12345678", "78563412");
}

TEST(BO_Output, hex_4_2_4_be)
{
    assert_conversion("oh2b4 s\" \" ih4l 12345678", "7856 3412");
}
