#include "test_helpers.h"

TEST(BO_Config, input_config)
{
    assert_failed_conversion(1000, "i");
    assert_failed_conversion(1000, "if");
    assert_failed_conversion(1000, "if4");
    assert_conversion("if4b", "");
    assert_conversion("ih1", "");

    assert_failed_conversion(1000, "iB");
    assert_conversion("iB1", "");
    assert_conversion("iB2l", "");

    assert_failed_conversion(1000, "iv4l");
    assert_failed_conversion(1000, "if10l");
    assert_failed_conversion(1000, "if4h");
    // assert_failed_conversion(1000, "if1l");
}

TEST(BO_Config, output_config)
{
    assert_failed_conversion(1000, "o");
    assert_failed_conversion(1000, "oo");
    assert_failed_conversion(1000, "oo8");
    assert_failed_conversion(1000, "oo8l");
    assert_conversion("oo8l1", "");

    assert_failed_conversion(1000, "oB2");
    assert_conversion("oB2b", "");

    assert_failed_conversion(1000, "oa8l2");
    assert_failed_conversion(1000, "oo9l2");
    assert_failed_conversion(1000, "oo32l2");
    assert_failed_conversion(1000, "oo8j2");
    assert_failed_conversion(1000, "oo8l-1");
}

TEST(BO_Config, prefix)
{
    assert_failed_conversion(1000, "p");
    assert_failed_conversion(1000, "p\"");
    assert_conversion("p\"\"", "");
    assert_failed_conversion(1000, "pp");
}

TEST(BO_Config, suffix)
{
    assert_failed_conversion(1000, "s");
    assert_failed_conversion(1000, "s\"");
    assert_conversion("s\"\"", "");
    assert_failed_conversion(1000, "sq");
}

TEST(BO_Config, preset)
{
    assert_failed_conversion(1000, "P");
    assert_failed_conversion(1000, "P2");
    assert_conversion("Pc", "");
    assert_conversion("Ps", "");
}
