#ifndef __DARK_COMMON_UTF8_HPP__
#define __DARK_COMMON_UTF8_HPP__

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#include <simdutf.h>

namespace dark::utf8 {
    static constexpr std::array<std::uint8_t, 16> utf_8_lookup { 
        1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 2, 2, 3, 4 
    };

    constexpr auto get_utf8_length(char c) -> std::uint8_t {
        auto const byte = static_cast<std::uint8_t>(c);
        return utf_8_lookup[byte >> 4];
    }

    constexpr auto valid_utf8_character_with_char_len(std::string_view const str) -> std::pair<char32_t, std::size_t> {
        char32_t code_point{};
        auto length = simdutf::convert_utf8_to_utf32(str.data(), str.size(), &code_point);
        
        return {code_point, length};
    }

    constexpr auto is_string_utf8(std::string_view const str) -> bool {
        return simdutf::validate_utf8(str.data(), str.size());
    }

    constexpr auto utf32_to_utf8(char32_t code_point, char* buffer) -> std::size_t {
        return simdutf::convert_utf32_to_utf8(&code_point, 1, buffer);
    }
}

#endif // __DARK_COMMON_UTF8_HPP__