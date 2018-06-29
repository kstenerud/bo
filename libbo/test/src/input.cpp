#include "test_helpers.h"

TEST(BO_Input, hex_1_2_1_le)
{
    assert_conversion("oh1l2 s\" \" ih1l ab cd ef", "ab cd ef");
}

TEST(BO_Input, hex_1_2_2_le)
{
    assert_conversion("oh1l2 s\" \" ih2l abcd 1234", "cd ab 34 12");
}

TEST(BO_Input, hex_1_2_4_le)
{
    assert_conversion("oh1l2 s\" \" ih4l 01234567 89abcdef", "67 45 23 01 ef cd ab 89");
}

TEST(BO_Input, hex_1_2_8_le)
{
    assert_conversion("oh1l2 s\" \" ih8l 0123456789abcdef 1122334455667788", "ef cd ab 89 67 45 23 01 88 77 66 55 44 33 22 11");
}

TEST(BO_Input, int_1_2_1_le)
{
    assert_conversion("oh1l2 s\" \" ii1l 10 20 30", "0a 14 1e");
}

TEST(BO_Input, int_1_2_2_le)
{
    assert_conversion("oh1l2 s\" \" ii2l 1000 2000 3000", "e8 03 d0 07 b8 0b");
}

TEST(BO_Input, int_1_2_4_le)
{
    assert_conversion("oh1l2 s\" \" ii4l 1000000000 2000000000", "00 ca 9a 3b 00 94 35 77");
}

TEST(BO_Input, int_1_2_8_be)
{
    assert_conversion("oh8b16 s\" \" ii8b 1000000000000000000 2000000000000000000", "0de0b6b3a7640000 1bc16d674ec80000");
}

TEST(BO_Input, octal_1_2_1_le)
{
    assert_conversion("oh1l2 s\" \" io1l 17 44", "0f 24");
}

TEST(BO_Input, octal_1_2_2_le)
{
    assert_conversion("oh1l2 s\" \" io2l 34323", "d3 38");
}

TEST(BO_Input, octal_1_2_4_le)
{
    assert_conversion("oh1l2 s\" \" io4l 6412642441", "21 45 2b 34");
}

TEST(BO_Input, octal_1_2_8_le)
{
    assert_conversion("oh1l2 s\" \" io8l 1142501502145044200512", "4a 01 91 28 23 34 a8 98");
}

TEST(BO_Input, boolean_1_2_1_le)
{
    assert_conversion("oh1l2 s\" \" ib1l 10 11", "02 03");
}

TEST(BO_Input, boolean_1_2_2_le)
{
    assert_conversion("oh1l2 s\" \" ib2l 10 11", "02 00 03 00");
}

TEST(BO_Input, boolean_1_2_4_le)
{
    assert_conversion("oh1l2 s\" \" ib4l 10 11", "02 00 00 00 03 00 00 00");
}

TEST(BO_Input, boolean_1_2_8_le)
{
    assert_conversion("oh1l2 s\" \" ib8l 10 11", "02 00 00 00 00 00 00 00 03 00 00 00 00 00 00 00");
}

TEST(BO_Input, float_1_2_4_le)
{
    assert_conversion("oh1l2 s\" \" if4l -59.18", "52 b8 6c c2");
}

TEST(BO_Input, float_1_2_8_le)
{
    assert_conversion("oh1l2 s\" \" if8l 948334.662", "fc a9 f1 52 dd f0 2c 41");
}

TEST(BO_Input, negative_hex_le)
{
    assert_conversion("oh1l2 s\" \" ih8l -2", "fe ff ff ff ff ff ff ff");
}

TEST(BO_Input, negative_octal_le)
{
    assert_conversion("oh1l2 s\" \" io8l -2", "fe ff ff ff ff ff ff ff");
}

TEST(BO_Input, negative_boolean_le)
{
    assert_conversion("oh1l2 s\" \" ib8l -10", "fe ff ff ff ff ff ff ff");
}
