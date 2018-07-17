#include "test_helpers.h"

TEST(BO_String, string)
{
    assert_conversion("os ih1 \"Testing\" 11 02 \"ß\" 5", "Testing\\x11\\x2ß\\x5");
    assert_conversion("os is \"\\101\\x42\\u263a\"", "AB☺");
}
