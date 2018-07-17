#include "test_helpers.h"

TEST(BO_String, string)
{
    assert_conversion("os ih1 \"Testing\" 01 02 \"ß\" 5", "Testing\\x01\\x02ß\\x05");
    assert_conversion("os is \"\\41\\x42\\u263a\"", "AB☺");
}
