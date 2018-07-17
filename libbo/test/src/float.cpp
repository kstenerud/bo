#include "test_helpers.h"

TEST(BO_Float, float32)
{
    assert_conversion("of4l3 if4l Ps 1.1 8.5 305.125 2", "1.100 8.500 305.125 2.000");
}
