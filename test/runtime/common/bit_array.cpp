#include <catch2/catch_test_macros.hpp>
#include "common/bit_array.hpp"

using namespace dark;

TEST_CASE("Bit Array Test", "[bit_array]") {
    BitArray bit_array{ true, false, true, false, true, false, true, false, true, false };
    REQUIRE(bit_array.size() == 10);
    REQUIRE(bit_array.actual_size() == 2);

    REQUIRE(bit_array[0] == true);
    REQUIRE(bit_array[1] == false);
    REQUIRE(bit_array[2] == true);
    REQUIRE(bit_array[3] == false);
    REQUIRE(bit_array[4] == true);
    REQUIRE(bit_array[5] == false);
    REQUIRE(bit_array[6] == true);
    REQUIRE(bit_array[7] == false);
    REQUIRE(bit_array[8] == true);
    REQUIRE(bit_array[9] == false);
}