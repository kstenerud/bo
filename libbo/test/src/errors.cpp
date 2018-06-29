#include "test_helpers.h"

TEST(BO_Errors, too_small)
{
    assert_failed_conversion(3, "oB ih1l 1 2 3 4");
}

TEST(BO_Errors, bad_src_width)
{
    assert_failed_conversion(3, "oB ii3l");
}

TEST(BO_Errors, bad_dst_width)
{
    assert_failed_conversion(3, "oh9l10 ii1l");
}

TEST(BO_Errors, no_input_or_output_config)
{
    assert_failed_conversion(1000, "1 2 3 4");
}

TEST(BO_Errors, no_input_config)
{
    assert_failed_conversion(1000, "oB 1 2 3 4");
}

TEST(BO_Errors, no_output_config)
{
    assert_failed_conversion(1000, "ii1l 1 2 3 4");
}
