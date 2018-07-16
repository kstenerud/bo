#include "test_helpers.h"

TEST(BO_Boolean, boolean)
{
    assert_conversion("ob2b1 ib2b 1011", "0000000000001011");
    assert_conversion("ob2l1 ib2l 1011", "1101000000000000");
    assert_conversion("ob2b1 ib2l 1011", "0000101100000000");
    assert_conversion("ob2l1 ib2b 1011", "0000000011010000");
}
