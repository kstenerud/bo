#include "test_helpers.h"

TEST(BO_Errors, too_small)
{
    assert_failed_conversion(3, "oB ih1l 1 2 3 4");
}
