#include "test_helpers.h"

TEST(BO_Span, input)
{
    assert_spanning_conversion("ih1",   1, 0, "");
    assert_spanning_conversion("ih1",   2, 0, "");
    assert_spanning_conversion("ih2l",  3, 0, "");
    assert_spanning_conversion("ih2l",  4, 0, "");
    assert_spanning_conversion("ih2l ", 5, 5, "");
}

TEST(BO_Span, output)
{
    assert_spanning_conversion("oB1",    1, 0, "");
    assert_spanning_conversion("oB1",    2, 0, "");
    assert_spanning_conversion("of4b6",  3, 0, "");
    assert_spanning_conversion("of4b6",  4, 0, "");
    assert_spanning_conversion("of4b6",  5, 0, "");
    assert_spanning_conversion("of4b6 ", 6, 6, "");
}

TEST(BO_Span, prefix)
{
    assert_spanning_conversion("p\"abcd\"",  1, 0, "");
    assert_spanning_conversion("p\"abcd\"",  2, 0, "");
    assert_spanning_conversion("p\"abcd\"",  3, 0, "");
    assert_spanning_conversion("p\"abcd\"",  4, 0, "");
    assert_spanning_conversion("p\"abcd\"",  5, 0, "");
    assert_spanning_conversion("p\"abcd\"",  6, 0, "");
    assert_spanning_conversion("p\"abcd\"",  7, 7, "");
}

TEST(BO_Span, suffix)
{
    assert_spanning_conversion("s\"abcd\"",  1, 0, "");
    assert_spanning_conversion("s\"abcd\"",  2, 0, "");
    assert_spanning_conversion("s\"abcd\"",  3, 0, "");
    assert_spanning_conversion("s\"abcd\"",  4, 0, "");
    assert_spanning_conversion("s\"abcd\"",  5, 0, "");
    assert_spanning_conversion("s\"abcd\"",  6, 0, "");
    assert_spanning_conversion("s\"abcd\"",  7, 7, "");
}

TEST(BO_Span, preset)
{
    assert_spanning_conversion("Pc",  1, 0, "");
    assert_spanning_conversion("Pc",  2, 0, "");
    assert_spanning_conversion("Pc ", 3, 3, "");
}

TEST(BO_Span, number)
{
    assert_spanning_conversion("ih1 oh1l2 12 ", 11, 10, "");
    assert_spanning_conversion("ih1 oh1l2 12 ", 12, 10, "");
    assert_spanning_conversion("ih1 oh1l2 12 ", 13, 13, "12");
}
