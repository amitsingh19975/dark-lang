#include <catch2/catch_test_macros.hpp>
#include <llvm/ADT/StringRef.h>
#include "adt/range.hpp"

using namespace dark;

TEST_CASE("Range Test", "[range]") {
    SECTION("Construction") {
        {
            auto range = Range(0, 10);
            REQUIRE(range.start() == 0);
            REQUIRE(range.last() == 10);
            REQUIRE(range.step() == 1);
            REQUIRE(range.size() == 10);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }

        {
            auto range = Range(0, 10, 2);
            REQUIRE(range.start() == 0);
            REQUIRE(range.last() == 10);
            REQUIRE(range.step() == 2);
            REQUIRE(range.size() == 5);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }

        {
            auto range = Range(10);
            REQUIRE(range.start() == 10);
            REQUIRE(range.last() == 11);
            REQUIRE(range.step() == 1);
            REQUIRE(range.size() == 1);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }

        {
            auto range = Range(Range::end_t{10});
            REQUIRE(range.start() == 0);
            REQUIRE(range.last() == 10);
            REQUIRE(range.step() == 1);
            REQUIRE(range.size() == 10);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }

        {
            auto range = Range(Range::end_t{10}, Range::step_t{2});
            REQUIRE(range.start() == 0);
            REQUIRE(range.last() == 10);
            REQUIRE(range.step() == 2);
            REQUIRE(range.size() == 5);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }

        {
            auto range = Range(Range::start_t{10}, Range::end_t{20});
            REQUIRE(range.start() == 10);
            REQUIRE(range.last() == 20);
            REQUIRE(range.step() == 1);
            REQUIRE(range.size() == 10);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }

        {
            auto range = Range(Range::start_t{10}, Range::end_t{20}, Range::step_t{2});
            REQUIRE(range.start() == 10);
            REQUIRE(range.last() == 20);
            REQUIRE(range.step() == 2);
            REQUIRE(range.size() == 5);
            REQUIRE(sizeof(Range) == sizeof(std::size_t) * 3);
        }
    }

    SECTION("Iterator") {
        {
            auto range = Range(0, 10);
            auto it = range.begin();
            REQUIRE(*it == 0);
            REQUIRE(it != range.end());
            
            for (auto j = 0ul; auto i : range) {
                REQUIRE(i == j);
                ++j;
            }
        }

        {
            auto range = Range(0, 10, 2);
            auto it = range.begin();
            REQUIRE(*it == 0);
            REQUIRE(it != range.end());

            for (auto j = 0ul; auto i : range) {
                REQUIRE(i == j);
                j += 2;
            }
        }

        {
            auto range = Range(10);
            auto it = range.rbegin();
            REQUIRE(*it == 10);
            REQUIRE(it != range.rend());

            for (auto j = 10ul; auto i : range) {
                REQUIRE(i == j);
                ++j;
            }
        }

        {
            auto range = Range(Range::end_t{10});
            auto j = 9ul;

            for (auto it = range.rbegin(); it < range.rend(); ++it) {
                REQUIRE(*it == j);
                --j;
            }
        }
        
        {
            auto range = Range(Range::end_t{10}, Range::step_t{2});
            auto j = 8ul;

            for (auto it = range.rbegin(); it < range.rend(); ++it) {
                REQUIRE(*it == j);
                j -= 2;
            }
        }
    }

    SECTION("Range contains function") {
        {
            auto range = Range(0, 10);
            REQUIRE(range.contains(0, true));
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(0, false));
            REQUIRE_FALSE(range.contains(10, false));
            REQUIRE(range.contains(5, true));
            REQUIRE(range.contains(5, false));
            REQUIRE_FALSE(range.contains(11, true));
        }

        {
            auto range = Range(0, 10, 2);
            REQUIRE(range.contains(0, true));
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(0, false));
            REQUIRE_FALSE(range.contains(10, false));
            REQUIRE(range.contains(4, true));
            REQUIRE(range.contains(4, false));
            REQUIRE_FALSE(range.contains(11, true));
        }

        {
            auto range = Range(10);
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(10, false));
            REQUIRE(range.contains(11, true));
        }

        {
            auto range = Range(Range::end_t{10});
            REQUIRE(range.contains(0, true));
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(0, false));
            REQUIRE_FALSE(range.contains(10, false));
            REQUIRE(range.contains(5, true));
            REQUIRE(range.contains(5, false));
            REQUIRE_FALSE(range.contains(11, true));
        }

        {
            auto range = Range(Range::end_t{10}, Range::step_t{2});
            REQUIRE(range.contains(0, true));
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(0, false));
            REQUIRE_FALSE(range.contains(10, false));
            REQUIRE(range.contains(4, true));
            REQUIRE(range.contains(4, false));
            REQUIRE_FALSE(range.contains(11, true));
        }

        {
            auto range = Range(Range::start_t{10}, Range::end_t{20});
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(20, true));
            REQUIRE(range.contains(10, false));
            REQUIRE_FALSE(range.contains(20, false));
            REQUIRE(range.contains(15, true));
            REQUIRE(range.contains(15, false));
            REQUIRE_FALSE(range.contains(21, true));
        }

        {
            auto range = Range(Range::start_t{10}, Range::end_t{20}, Range::step_t{2});
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(20, true));
            REQUIRE(range.contains(10, false));
            REQUIRE_FALSE(range.contains(20, false));
            REQUIRE(range.contains(16, true));
            REQUIRE(range.contains(16, false));
            REQUIRE_FALSE(range.contains(21, true));
        }
    }
}

TEST_CASE("Range with constant stride", "[constant_strided_range]") {
    SECTION("Construction") {
        {
            auto range = SimpleRange(0, 10);
            REQUIRE(range.start() == 0);
            REQUIRE(range.last() == 10);
            REQUIRE(range.step() == 1);
            REQUIRE(range.size() == 10);
            REQUIRE(sizeof(range) == sizeof(std::size_t) * 2);
        }

        {
            auto range = BasicRange<std::uint8_t, 2>(0, 10);
            REQUIRE(range.start() == 0);
            REQUIRE(range.last() == 10);
            REQUIRE(range.step() == 2);
            REQUIRE(range.size() == 5);
            REQUIRE(sizeof(range) == sizeof(std::uint8_t) * 2);
        }
    }

    SECTION("Iterator") {
        {
            auto range = SimpleRange(0, 10);
            auto it = range.begin();
            REQUIRE(*it == 0);
            REQUIRE(it != range.end());
            
            for (auto j = 0ul; auto i : range) {
                REQUIRE(i == j);
                ++j;
            }
        }

        {
            auto range = BasicRange<std::uint8_t, 2>(0, 10);
            auto it = range.begin();
            REQUIRE(*it == 0);
            REQUIRE(it != range.end());

            for (auto j = 0ul; auto i : range) {
                REQUIRE(i == j);
                j += 2;
            }
        }

        {
            auto range = SimpleRange(10);
            auto it = range.rbegin();
            REQUIRE(*it == 10);
            REQUIRE(it != range.rend());

            for (auto j = 10ul; auto i : range) {
                REQUIRE(i == j);
                ++j;
            }
        }
    }

    SECTION("Range contains function") {
        {
            auto range = SimpleRange(0, 10);
            REQUIRE(range.contains(0, true));
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(0, false));
            REQUIRE_FALSE(range.contains(10, false));
            REQUIRE(range.contains(5, true));
            REQUIRE(range.contains(5, false));
            REQUIRE_FALSE(range.contains(11, true));
        }

        {
            auto range = BasicRange<std::uint8_t, 2>(0, 10);
            REQUIRE(range.contains(0, true));
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(0, false));
            REQUIRE_FALSE(range.contains(10, false));
            REQUIRE(range.contains(4, true));
            REQUIRE(range.contains(4, false));
            REQUIRE_FALSE(range.contains(11, true));
        }

        {
            auto range = SimpleRange(10);
            REQUIRE(range.contains(10, true));
            REQUIRE(range.contains(10, false));
            REQUIRE(range.contains(11, true));
        }
    }
}