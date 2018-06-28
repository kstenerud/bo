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
