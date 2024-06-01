#include <catch2/catch_test_macros.hpp>
#include <llvm/ADT/StringRef.h>
#include <stdexcept>
#include "common/big_num.hpp"

using namespace dark;

TEST_CASE("Signed Big Number Test", "[signed_big_num]") {
    SECTION("Create a big number") {
        {
            auto a = SignedBigNum(llvm::StringRef("1234567890"), 10);
            REQUIRE(a.to_str(16) == "0x499602d2");
        }
        {
            auto a = SignedBigNum(llvm::StringRef("1234567890F"), 16);
            REQUIRE(a.to_str(16) == "0x1234567890f");
        }
    }

    SECTION("Add two large number") {
        {
            auto a = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            auto b = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);
            REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
            REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
            auto c = a + b;
            REQUIRE(c.to_str(16) == "0x605f9f6c5d04589bc");

            REQUIRE((a + 4).to_str() == "98765432109876543214");
        }
    }

    SECTION("Subtract two large number") {
        {
            auto a = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            auto b = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);
            
            REQUIRE(a.to_str(2) == "0b1010101101010100101010011010011100011100101001001100111111011101010");
            REQUIRE(b.to_str(2) == "0b1010101101010100101010011000110011101011000111110000101011010010");

            REQUIRE(a.to_str(8) == "0o12552452323434511477352");
            REQUIRE(b.to_str(8) == "0o1255245230635307605322");

            REQUIRE(a.to_str(10) == "98765432109876543210");
            REQUIRE(b.to_str(10) == "12345678901234567890");

            REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
            REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
            auto c = a - b;
            
            REQUIRE(c.to_str(2) == "0b1001010111101010000101000111010101111111010000001110111010000011000");
            REQUIRE(c.to_str(8) == "0o11275205072577201672030");
            REQUIRE(c.to_str(10) == "86419753208641975320");
            REQUIRE(c.to_str(16) == "0x4af50a3abfa077418");

            REQUIRE((a - 4).to_str() == "98765432109876543206");
        }

        {
            auto a = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            auto b = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);
            REQUIRE(a.to_str() == "98765432109876543210");
            REQUIRE(b.to_str() == "12345678901234567890");
            auto c = b - a;
            REQUIRE(c.to_str(16) == "-0x4af50a3abfa077418");
        }

        {
            auto a = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);
            auto b = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            REQUIRE(a.to_str(16) == "0xab54a98ceb1f0ad2");
            REQUIRE(b.to_str(16) == "0x55aa54d38e5267eea");
            a -= b;
            REQUIRE(a.to_str(16) == "-0x4af50a3abfa077418");
        }
    }

    SECTION("Multiply two large number") {
        auto a = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
        auto b = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);
        REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
        REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
        auto c = a * b;
        REQUIRE(c.to_str(16) == "0x39551b49bf4f8a3a2127989c1a6df3ff4");

        REQUIRE((a * 4).to_str() == "395061728439506172840");
    }

    SECTION("Divide two large number") {
        auto a = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
        auto b = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);
        REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
        REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
        auto c = a / b;
        REQUIRE(c.to_str(16) == "0x8");

        REQUIRE((a / 4).to_str() == "24691358027469135802");
    }

    SECTION("Compare two large number") {
        {
            auto a = SignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            auto b = SignedBigNum(llvm::StringRef("12345678901234567890"), 10);

            REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
            REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
            REQUIRE(a > b);
            REQUIRE(b < a);
            REQUIRE(a >= b);
            REQUIRE(b <= a);
            REQUIRE(a != b);
            REQUIRE(b == b);
        }
    }

    SECTION("Shift operation") {
        {
            auto a = SignedBigNum("0101", 2);
            REQUIRE(a.to_str(2) == "0b101");
            REQUIRE((a << 1ul).to_str(2) == "0b1010");
            REQUIRE((a >> 1ul).to_str(2) == "0b10");
        }
        {
            auto a = SignedBigNum("-2", 10);
            REQUIRE(a.to_str(2) == "-0b10");
            REQUIRE((a << 1ul).to_str(2) == "-0b100");
            REQUIRE((a >> 1ul).to_str(2) == "-0b1");
            REQUIRE((a >> 2ul).to_sign_extended_str(4) == "0b0000");
        }
    }
}

TEST_CASE("Unsigned Big Num Test", "[unsigned_big_num]") {
    SECTION("Create a big number") {
        {
            auto a = UnsignedBigNum(llvm::StringRef("1234567890"), 10);
            REQUIRE(a.to_str(2) == "0b1001001100101100000001011010010");
            REQUIRE(a.to_str(8) == "0o11145401322");
            REQUIRE(a.to_str(10) == "1234567890");
            REQUIRE(a.to_str(16) == "0x499602d2");
        }
        {
            auto a = UnsignedBigNum(llvm::StringRef("1234567890F"), 16);
            REQUIRE(a.to_str(2) == "0b10010001101000101011001111000100100001111");
            REQUIRE(a.to_str(8) == "0o22150531704417");
            REQUIRE(a.to_str(10) == "1250999896335");
            REQUIRE(a.to_str(16) == "0x1234567890f");
        }
        {
            REQUIRE_THROWS_AS(UnsignedBigNum(llvm::StringRef("-123")), std::invalid_argument);
            REQUIRE_THROWS_AS(UnsignedBigNum(llvm::StringRef("123.0")), std::invalid_argument);
            REQUIRE_THROWS_AS(UnsignedBigNum(llvm::StringRef("0x123.0")), std::invalid_argument);
            REQUIRE_THROWS_AS(UnsignedBigNum(llvm::StringRef("-0x123.0")), std::invalid_argument);
        }
    }

    SECTION("Add two large number") {
        {
            auto a = UnsignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            auto b = UnsignedBigNum(llvm::StringRef("12345678901234567890"), 10);
            REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
            REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
            auto c = a + b;
            REQUIRE(c.to_str(16) == "0x605f9f6c5d04589bc");

            REQUIRE((a + 4).to_str() == "98765432109876543214");
        }
    }

    SECTION("Subtracting two large number") {
        auto a = UnsignedBigNum(llvm::StringRef("98765432109876543210"), 10);
        auto b = UnsignedBigNum(llvm::StringRef("12345678901234567890"), 10);
        REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
        REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
        {
            auto c = a - b;
            REQUIRE(c.to_str(16) == "0x4af50a3abfa077418");
        }
        {
            auto c = b - a;
            REQUIRE(c.to_str(16) == "0x0");
        }
        REQUIRE((a - 4).to_str() == "98765432109876543206");
    }

    SECTION("Multiply two large number") {
        auto a = UnsignedBigNum(llvm::StringRef("98765432109876543210"), 10);
        auto b = UnsignedBigNum(llvm::StringRef("12345678901234567890"), 10);
        REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
        REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
        auto c = a * b;
        REQUIRE(c.to_str(16) == "0x39551b49bf4f8a3a2127989c1a6df3ff4");
        REQUIRE((a * 4).to_str() == "395061728439506172840");
    }

    SECTION("Divide two large number") {
        auto a = UnsignedBigNum(llvm::StringRef("98765432109876543210"), 10);
        auto b = UnsignedBigNum(llvm::StringRef("12345678901234567890"), 10);
        REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
        REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
        auto c = a / b;
        REQUIRE(c.to_str(16) == "0x8");
        REQUIRE((a / 4).to_str() == "24691358027469135802");
    }

    SECTION("Compare two large number") {
        {
            auto a = UnsignedBigNum(llvm::StringRef("98765432109876543210"), 10);
            auto b = UnsignedBigNum(llvm::StringRef("12345678901234567890"), 10);

            REQUIRE(a.to_str(16) == "0x55aa54d38e5267eea");
            REQUIRE(b.to_str(16) == "0xab54a98ceb1f0ad2");
            REQUIRE(a > b);
            REQUIRE(b < a);
            REQUIRE(a >= b);
            REQUIRE(b <= a);
            REQUIRE(a != b);
            REQUIRE(b == b);
        }
    }

    SECTION("Shift operation") {
        auto a = UnsignedBigNum("0101", 2);
        REQUIRE(a.to_str(2) == "0b101");
        REQUIRE((a << 1ul).to_str(2) == "0b1010");
        REQUIRE((a >> 1ul).to_str(2) == "0b10");
    }
}

TEST_CASE("Float Number Test", "[float_num]") {
    SECTION("Construction") {
        {
            auto a = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            REQUIRE(a.to_str(10) == "1234567890.1123456789");
        }
        {
            auto a = BigFloatNum(llvm::StringRef("-1234567890.1123456789"), 10);
            REQUIRE(a.to_str(10) == "-1234567890.1123456789");
        }
        {
            auto a = BigFloatNum(llvm::StringRef("-1234567890"), 10);
            REQUIRE(a.to_str(10) == "-1234567890.0");
        }
    }

    SECTION("Add two large numbers") {
        {
            auto a = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto b = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto c = a + b;
            REQUIRE(c.to_str(10) == "2469135780.2246913578");
        }

        {
            auto a = BigFloatNum(llvm::StringRef("123"), 10);
            REQUIRE((a / 4).to_str() == "30.75");
        }
    }

    SECTION("Subtract two large numbers") {
        {
            auto a = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto b = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto c = a - b;
            REQUIRE(c.to_str(10) == "0.0");
        }
        {
            auto a = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto b = BigFloatNum(llvm::StringRef("0.1123456789"), 10);
            auto c = a - b;
            REQUIRE(c.to_str(10) == "1234567890.0");
        }

        {
            auto a = BigFloatNum(llvm::StringRef("123"), 10);
            REQUIRE((a - 4).to_str() == "119.0");
        }
    }

    SECTION("Multiply two large numbers") {
        {
            auto a = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto b = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto c = a * b;
            REQUIRE(c.to_str(10) == "1524157875296448835.51");
        }
        {
            auto a = BigFloatNum(llvm::StringRef("123"), 10);
            REQUIRE((a * 4.5).to_str() == "553.5");
        }
    }

    SECTION("Divide two large numbers") {
        {
            auto a = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto b = BigFloatNum(llvm::StringRef("1234567890.1123456789"), 10);
            auto c = a / b;
            REQUIRE(c.to_str(10) == "1.0");
        }
        {
            auto a = BigFloatNum(llvm::StringRef("125"), 10);
            REQUIRE((a / 0.5).to_str() == "250.0");
        }
    }
}

TEST_CASE("Real Number Test", "[real_num]") {
    SECTION("Construction") {
        {
            auto a = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            REQUIRE(a.to_str(10) == "1234567890/1123456789");
        }
        {
            auto a = BigRealNum(llvm::StringRef("-1234567890/1123456789"), 10);
            REQUIRE(a.to_str(10) == "-1234567890/1123456789");
        }
        {
            auto a = BigRealNum(3, 2);
            REQUIRE(a.to_str(10) == "3/2");
        }
        {
            auto num = SignedBigNum("1234567890", 10);
            auto den = UnsignedBigNum("1123456789", 10);
            auto a = BigRealNum(num, den);
            REQUIRE(a.to_str(10) == "1234567890/1123456789");
        }
    }

    SECTION("Add two large numbers") {
        {
            auto a = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto b = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto c = a + b;
            REQUIRE(c.to_str(10) == "2469135780/1123456789");
        }

        {
            auto a = BigRealNum(3, 2);
            REQUIRE((a + 4).to_str() == "11/2");
        }
    }

    SECTION("Subtract two large numbers") {
        {
            auto a = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto b = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto c = a - b;
            REQUIRE(c.to_str(10) == "0");
        }
        {
            auto a = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto b = BigRealNum(llvm::StringRef("0/1123456789"), 10);
            auto c = a - b;
            REQUIRE(c.to_str(10) == "1234567890/1123456789");
        }

        {
            auto a = BigRealNum(3, 2);
            REQUIRE((a - 4).to_str() == "-5/2");
        }
    }

    SECTION("Multiply two large numbers") {
        {
            auto a = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto b = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto c = a * b;
            REQUIRE(c.to_str(10) == "1524157875019052100/1262155156750190521");
        }
        {
            auto a = BigRealNum(3, 2);
            REQUIRE((a * 4.5).to_str() == "27/4");
        }
    }

    SECTION("Divide two large numbers") {
        {
            auto a = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto b = BigRealNum(llvm::StringRef("1234567890/1123456789"), 10);
            auto c = a / b;
            REQUIRE(c.to_str(10) == "1");
        }
        {
            auto a = BigRealNum(3, 2);
            REQUIRE((a / 0.5).to_str() == "3");
        }
    }
}

TEST_CASE("Testing Cast function", "[big_num_cast]") {
    SECTION("Testing from unsigned to other kind") {
        auto a = UnsignedBigNum(llvm::StringRef("1234567890"), 10);
        REQUIRE(a.to_str(10) == "1234567890");
        REQUIRE(a.is_unsigned());
        {
            auto&& temp = cast<BigNumKind::UnsignedInteger>(a);
            REQUIRE(&temp == &a);
            REQUIRE(temp.is_unsigned());
        }
        {
            auto temp = cast<BigNumKind::SignedInteger>(a);
            REQUIRE(temp.to_str(10) == "1234567890");
            REQUIRE(temp.is_signed());
        }
        {
            auto temp = cast<BigNumKind::Float>(a);
            REQUIRE(temp.to_str(10) == "1234567890.0");
            REQUIRE(temp.is_float());
        }
        {
            auto temp = cast<BigNumKind::Real>(a);
            REQUIRE(temp.to_str(10) == "1234567890");
            REQUIRE(temp.is_real());
        }
    }

    SECTION("Testing from signed to other kind") {
        auto a = SignedBigNum(llvm::StringRef("1234567890"), 10);
        REQUIRE(a.to_str(10) == "1234567890");
        REQUIRE(a.is_signed());
        {
            auto&& temp = cast<BigNumKind::SignedInteger>(a);
            REQUIRE(&temp == &a);
            REQUIRE(temp.is_signed());
        }
        {
            auto temp = cast<BigNumKind::UnsignedInteger>(a);
            REQUIRE(temp.to_str(10) == "1234567890");
            REQUIRE(temp.is_unsigned());
        }
        {
            auto temp = cast<BigNumKind::Float>(a);
            REQUIRE(temp.to_str(10) == "1234567890.0");
            REQUIRE(temp.is_float());
        }
        {
            auto temp = cast<BigNumKind::Real>(a);
            REQUIRE(temp.to_str(10) == "1234567890");
            REQUIRE(temp.is_real());
        }
    }

    SECTION("Testing from float to other kind") {
        auto a = BigFloatNum(llvm::StringRef("1234567890.123"), 10);
        REQUIRE(a.to_str(10) == "1234567890.123");
        REQUIRE(a.is_float());
        {
            auto&& temp = cast<BigNumKind::Float>(a);
            REQUIRE(&temp == &a);
            REQUIRE(temp.is_float());
        }
        {
            auto temp = cast<BigNumKind::SignedInteger>(a);
            REQUIRE(temp.to_str(10) == "1234567890");
            REQUIRE(temp.is_signed());
        }
        {
            auto temp = cast<BigNumKind::UnsignedInteger>(a);
            REQUIRE(temp.to_str(10) == "1234567890");
            REQUIRE(temp.is_unsigned());
        }
        {
            auto temp = cast<BigNumKind::Real>(a);
            REQUIRE(temp.to_str(10) == "420101683775643526789095912382611476500415237849/340282366920938463463374607431768211456");
            REQUIRE(temp.is_real());
        }
    }

    SECTION("Testing from real to other kind") {
        auto a = BigRealNum(llvm::StringRef("1234567890/123"), 10);
        REQUIRE(a.to_str(10) == "1234567890/123");
        REQUIRE(a.is_real());
        {
            auto&& temp = cast<BigNumKind::Real>(a);
            REQUIRE(&temp == &a);
            REQUIRE(temp.is_real());
        }
        {
            auto temp = cast<BigNumKind::SignedInteger>(a);
            REQUIRE(temp.to_str(10) == "10037137");
            REQUIRE(temp.is_signed());
        }
        {
            auto temp = cast<BigNumKind::UnsignedInteger>(a);
            REQUIRE(temp.to_str(10) == "10037137");
            REQUIRE(temp.is_unsigned());
        }
        {
            auto temp = cast<BigNumKind::Float>(a);
            REQUIRE(temp.to_str(10) == "10037137.3170731707317");
            REQUIRE(temp.is_float());
        }
    }
}
