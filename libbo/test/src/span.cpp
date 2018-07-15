#include "test_helpers.h"

TEST(BO_Span, input)
{
    assert_spanning_conversion("ih1",  1, 0, "");
    assert_spanning_conversion("ih1",  2, 0, "");
    assert_spanning_conversion("ih2l", 3, 0, "");
}

TEST(BO_Span, output)
{
    assert_spanning_conversion("oB1",   1, 0, "");
    assert_spanning_conversion("oB1",   2, 0, "");
    assert_spanning_conversion("of4b6", 3, 0, "");
    assert_spanning_conversion("of4b6", 4, 0, "");
}

TEST(BO_Span, prefix)
{
    assert_spanning_conversion("p\"abcd\"",  1, 0, "");
    // assert_spanning_conversion("p\"abcd\"",  2, "");
    // assert_spanning_conversion("p\"abcd\"",  3, "");
    // assert_spanning_conversion("p\"abcd\"",  4, "");
    // assert_spanning_conversion("p\"abcd\"",  5, "");
    // assert_spanning_conversion("p\"abcd\"",  6, "");
}

