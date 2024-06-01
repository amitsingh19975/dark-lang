#ifndef __DARK_LEXER_CHARACTER_SET_HPP__
#define __DARK_LEXER_CHARACTER_SET_HPP__

#include <cstdint>
#include <limits>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Format.h>
#include "common/bit_array.hpp"
// #include <experimental/simd>

namespace dark::lexer::char_set {

    // namespace stdx = std::experimental;

    constexpr inline auto is_alpha(char32_t c) noexcept -> bool { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    constexpr inline auto is_alpha(char c) noexcept -> bool { return is_alpha(static_cast<char32_t>(c)); }
    
    constexpr inline auto is_digit(char32_t c) noexcept -> bool { return c >= '0' && c <= '9'; }
    constexpr inline auto is_digit(char c) noexcept -> bool { return is_digit(static_cast<char32_t>(c)); }
    
    constexpr inline auto is_oct_digit(char32_t c) noexcept -> bool { return c >= '0' && c <= '7'; }
    constexpr inline auto is_oct_digit(char c) noexcept -> bool { return is_oct_digit(static_cast<char32_t>(c)); }
    
    constexpr inline auto is_alnum(char32_t c) noexcept -> bool { return is_alpha(c) || is_digit(c); }
    constexpr inline auto is_alnum(char c) noexcept -> bool { return is_alpha(c) || is_digit(c); }

    constexpr inline auto is_hex_digit(char32_t c) noexcept -> bool { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
    constexpr inline auto is_hex_digit(char c) noexcept -> bool { return is_hex_digit(static_cast<char32_t>(c)); }

    constexpr inline auto is_lower(char32_t c) noexcept -> bool { return c >= 'a' && c <= 'z'; }
    constexpr inline auto is_lower(char c) noexcept -> bool { return is_lower(static_cast<char32_t>(c)); }

    constexpr inline auto is_horizontal_space(char32_t c) noexcept -> bool { return c == ' ' || c == '\t'; }
    constexpr inline auto is_horizontal_space(char c) noexcept -> bool { return is_horizontal_space(static_cast<char32_t>(c)); }

    constexpr inline auto is_vertical_space(char32_t c) noexcept -> bool { return c == '\n'; }
    constexpr inline auto is_vertical_space(char c) noexcept -> bool { return is_vertical_space(static_cast<char32_t>(c)); }

    constexpr inline auto is_space(char c) noexcept -> bool { return is_horizontal_space(c) || is_vertical_space(c); }
    constexpr inline auto is_space(char32_t c) noexcept -> bool { return is_horizontal_space(c) || is_vertical_space(c); }


    namespace detail {
        constexpr inline auto is_valid_identifier_continuation_code_point_helper(char32_t c) noexcept -> bool {
            // || (c >= 0x10000 && c <= 0x1FFFD)
            // || (c >= 0x20000 && c <= 0x2FFFD)
            // || (c >= 0x30000 && c <= 0x3FFFD)
            // || (c >= 0x40000 && c <= 0x4FFFD)
            // || (c >= 0x50000 && c <= 0x5FFFD)
            // || (c >= 0x60000 && c <= 0x6FFFD)
            // || (c >= 0x70000 && c <= 0x7FFFD)
            // || (c >= 0x80000 && c <= 0x8FFFD)
            // || (c >= 0x90000 && c <= 0x9FFFD)
            // || (c >= 0xA0000 && c <= 0xAFFFD)
            // || (c >= 0xB0000 && c <= 0xBFFFD)
            // || (c >= 0xC0000 && c <= 0xCFFFD)
            // || (c >= 0xD0000 && c <= 0xDFFFD)
            // || (c >= 0xE0000 && c <= 0xEFFFD);
            auto const lower = c & 0x00FFFF;
            return (c >= 0x10000 && c <= 0xEFFFD) && (lower != 0xFFFF && lower != 0xFFFE);
        }
    
        static constexpr auto identifier_lookup_table = []() {
            BitArray<std::numeric_limits<std::uint16_t>::max()> table{ };
            
            // if (c < 0x80) return is_alpha(c) || c == '_' || c == '$';
            for (char32_t c = 0x00; c < 0x80; ++c) table.set(c, (is_alpha(c) || c == '_' || c == '$'));

            // N1518: Recommendations for extended identifier characters for C and C++
            // Proposed Annex X.1: Ranges of characters allowed
            // return c == 0x00A8 || c == 0x00AA || c == 0x00AD || c == 0x00AF
            //     || (c >= 0x00B2 && c <= 0x00B5) || (c >= 0x00B7 && c <= 0x00BA)
            //     || (c >= 0x00BC && c <= 0x00BE) || (c >= 0x00C0 && c <= 0x00D6)
            //     || (c >= 0x00D8 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FF)

            //     || (c >= 0x0100 && c <= 0x167F)
            //     || (c >= 0x1681 && c <= 0x180D)
            //     || (c >= 0x180F && c <= 0x1FFF)

            //     || (c >= 0x200B && c <= 0x200D)
            //     || (c >= 0x202A && c <= 0x202E)
            //     || (c >= 0x203F && c <= 0x2040)
            //     || c == 0x2054
            //     || (c >= 0x2060 && c <= 0x206F)

            //     || (c >= 0x2070 && c <= 0x218F)
            //     || (c >= 0x2460 && c <= 0x24FF)
            //     || (c >= 0x2776 && c <= 0x2793)
            //     || (c >= 0x2C00 && c <= 0x2DFF)
            //     || (c >= 0x2E80 && c <= 0x2FFF)

            //     || (c >= 0x3004 && c <= 0x3007)
            //     || (c >= 0x3021 && c <= 0x302F)
            //     || (c >= 0x3031 && c <= 0x303F)

            //     || (c >= 0x3040 && c <= 0xD7FF)

            //     || (c >= 0xF900 && c <= 0xFD3D)
            //     || (c >= 0xFD40 && c <= 0xFDCF)
            //     || (c >= 0xFDF0 && c <= 0xFE44)
            //     || (c >= 0xFE47 && c <= 0xFFF8);
            table.set(0x00A8, true);
            table.set(0x00AA, true);
            table.set(0x00AD, true);
            table.set(0x00AF, true);

            for (char32_t c = 0x00B2; c <= 0x00B5; ++c) table.set(c, true);
            for (char32_t c = 0x00B7; c <= 0x00BA; ++c) table.set(c, true);
            for (char32_t c = 0x00BC; c <= 0x00BE; ++c) table.set(c, true);
            for (char32_t c = 0x00C0; c <= 0x00D6; ++c) table.set(c, true);
            for (char32_t c = 0x00D8; c <= 0x00F6; ++c) table.set(c, true);
            for (char32_t c = 0x00F8; c <= 0x00FF; ++c) table.set(c, true);

            for (char32_t c = 0x0100; c <= 0x167F; ++c) table.set(c, true);
            for (char32_t c = 0x1681; c <= 0x180D; ++c) table.set(c, true);
            for (char32_t c = 0x180F; c <= 0x1FFF; ++c) table.set(c, true);

            for (char32_t c = 0x200B; c <= 0x200D; ++c) table.set(c, true);
            for (char32_t c = 0x202A; c <= 0x202E; ++c) table.set(c, true);
            for (char32_t c = 0x203F; c <= 0x2040; ++c) table.set(c, true);
            
            table.set(0x2054, true);

            for (char32_t c = 0x2060; c <= 0x206F; ++c) table.set(c, true);

            for (char32_t c = 0x2070; c <= 0x218F; ++c) table.set(c, true);
            for (char32_t c = 0x2460; c <= 0x24FF; ++c) table.set(c, true);
            for (char32_t c = 0x2776; c <= 0x2793; ++c) table.set(c, true);
            for (char32_t c = 0x2C00; c <= 0x2DFF; ++c) table.set(c, true);
            for (char32_t c = 0x2E80; c <= 0x2FFF; ++c) table.set(c, true);

            for (char32_t c = 0x3004; c <= 0x3007; ++c) table.set(c, true);
            for (char32_t c = 0x3021; c <= 0x302F; ++c) table.set(c, true);
            for (char32_t c = 0x3031; c <= 0x303F; ++c) table.set(c, true);

            for (char32_t c = 0x3040; c <= 0xD7FF; ++c) table.set(c, true);

            for (char32_t c = 0xF900; c <= 0xFD3D; ++c) table.set(c, true);
            for (char32_t c = 0xFD40; c <= 0xFDCF; ++c) table.set(c, true);
            for (char32_t c = 0xFDF0; c <= 0xFE44; ++c) table.set(c, true);
            for (char32_t c = 0xFE47; c <= 0xFFF8; ++c) table.set(c, true);

            return table;
        }();

        static constexpr auto operator_lookup_table = []() {
            BitArray<std::numeric_limits<std::uint16_t>::max()> table{ };

            // if c < 0x80 {
            //     return "/=-+*%<>!&|^~.?".contains(ch);
            // }

            // // Unicode math, symbol, arrow, dingbat, and line/box drawing chars.
            // (c >= 0x00A1 && c <= 0x00A7)
            //     || c == 0x00A9 || c == 0x00AB || c == 0x00Ac || c == 0x00AE
            //     || c == 0x00B0 || c == 0x00B1 || c == 0x00B6 || c == 0x00BB
            //     || c == 0x00BF || c == 0x00D7 || c == 0x00F7
            //     || c == 0x2016 || c == 0x2017 || (c >= 0x2020 && c <= 0x2027)
            //     || (c >= 0x2030 && c <= 0x203E) || (c >= 0x2041 && c <= 0x2053)
            //     || (c >= 0x2055 && c <= 0x205E) || (c >= 0x2190 && c <= 0x23FF)
            //     || (c >= 0x2500 && c <= 0x2775) || (c >= 0x2794 && c <= 0x2BFF)
            //     || (c >= 0x2E00 && c <= 0x2E7F) || (c >= 0x3001 && c <= 0x3003)
            //     || (c >= 0x3008 && c <= 0x3030)

            for (char32_t c : U"/=-+*%<>!&|^~.?") table.set(c, true);
            for (char32_t c = 0x00A1; c <= 0x00A7; ++c) table.set(c, true);
            table.set(0x00A9, true);
            table.set(0x00AB, true);
            table.set(0x00AC, true);
            table.set(0x00AE, true);
            table.set(0x00B0, true);
            table.set(0x00B1, true);
            table.set(0x00B6, true);
            table.set(0x00BB, true);
            table.set(0x00BF, true);
            table.set(0x00D7, true);
            table.set(0x00F7, true);
            table.set(0x2016, true);
            table.set(0x2017, true);
            for (char32_t c = 0x2020; c <= 0x2027; ++c) table.set(c, true);
            for (char32_t c = 0x2030; c <= 0x203E; ++c) table.set(c, true);
            for (char32_t c = 0x2041; c <= 0x2053; ++c) table.set(c, true);
            for (char32_t c = 0x2055; c <= 0x205E; ++c) table.set(c, true);
            for (char32_t c = 0x2190; c <= 0x23FF; ++c) table.set(c, true);
            for (char32_t c = 0x2500; c <= 0x2775; ++c) table.set(c, true);
            for (char32_t c = 0x2794; c <= 0x2BFF; ++c) table.set(c, true);
            for (char32_t c = 0x2E00; c <= 0x2E7F; ++c) table.set(c, true);
            for (char32_t c = 0x3001; c <= 0x3003; ++c) table.set(c, true);
            for (char32_t c = 0x3008; c <= 0x3030; ++c) table.set(c, true);

            return table;
        }();

        constexpr auto binary_digits = []() {
            auto res = BitArray<256>();
            res.set(static_cast<unsigned>('0'), true);
            res.set(static_cast<unsigned>('1'), true);
            return res;
        }();

        constexpr auto octal_digits = []() {
            auto res = BitArray<256>();
            for (unsigned c = 0; c < 8; ++c) {
                res.set(static_cast<unsigned>('0') + c, true);
            }
            return res;
        }();

        constexpr auto decimal_digits = []() {
            auto res = BitArray<256>();
            for (unsigned c = 0; c < 10; ++c) {
                res.set(static_cast<unsigned>('0') + c, true);
            }
            return res;
        }();

        constexpr auto hexadecimal_digits = []() {
            auto res = BitArray<256>();
            for (unsigned c = 0; c < 10; ++c) {
                res.set(static_cast<unsigned>('0') + c, true);
            }
            for (unsigned c = 0; c < 6; ++c) {
                res.set(static_cast<unsigned>('a') + c, true);
                res.set(static_cast<unsigned>('A') + c, true);
            }

            return res;
        }();

    } // namespace detail

    constexpr inline auto is_valid_identifier_continuation_code_point(char32_t c) noexcept -> bool {
        return detail::identifier_lookup_table[c & 0x7F] && detail::is_valid_identifier_continuation_code_point_helper(c);
    }

    constexpr inline auto is_valid_identifier_start_code_point(char32_t cp) noexcept -> bool {
        if (!is_valid_identifier_continuation_code_point(cp)) return false;

        if (is_digit(cp) || cp == '$') return false;

        // N1518: Recommendations for extended identifier characters for C and C++
        // Proposed Annex X.2: Ranges of characters disallowed initially
        if (
            (cp >= 0x0300 && cp <= 0x036F) || 
            (cp >= 0x1DC0 && cp <= 0x1DFF) || 
            (cp >= 0x20D0 && cp <= 0x20FF) || 
            (cp >= 0xFE20 && cp <= 0xFE2F)
        ) {
            return false;
        }

        return true;
    }

    constexpr inline auto is_valid_operator_continuation_code_point(char32_t c) noexcept -> bool {
        return detail::operator_lookup_table[c & 0x7F];
    }

    constexpr inline auto is_valid_operator_start_code_point(char32_t c) noexcept -> bool {
        if (is_valid_operator_continuation_code_point(c)) return true;

        return (c >= 0x0300 && c <= 0x036F)
            || (c >= 0x1DC0 && c <= 0x1DFF)
            || (c >= 0x20D0 && c <= 0x20FF)
            || (c >= 0xFE00 && c <= 0xFE0F)
            || (c >= 0xFE20 && c <= 0xFE2F)
            || (c >= 0xE0100 && c <= 0xE01EF);
    }
} // namespace dark::lexer::char_set

#endif // __DARK_LEXER_CHARACTER_SET_HPP__