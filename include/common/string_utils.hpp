#ifndef __DARK_COMMON_STRING_UTILS_HPP__
#define __DARK_COMMON_STRING_UTILS_HPP__

#include <functional>
#include <string>
#include <string_view>

namespace dark::utils {

    constexpr auto string_contains_ptr(std::string_view str, const char* ptr) noexcept -> bool {
        constexpr auto le = std::less_equal<const char*>{};
        return le(str.data(), ptr) && le(ptr, str.end());
    }

    inline static auto ltrim(std::string& str, std::string_view chars = " \t\n\r\f\v") -> std::string& {
        str.erase(0, str.find_first_not_of(chars));
        return str;
    }

    inline static auto rtrim(std::string& str, std::string_view chars = " \t\n\r\f\v") -> std::string& {
        str.erase(str.find_last_not_of(chars) + 1);
        return str;
    }

    inline static auto trim(std::string& str, std::string_view chars = " \t\n\r\f\v") -> std::string& {
        return ltrim(rtrim(str, chars), chars);
    }
}

#endif // __DARK_COMMON_STRING_UTILS_HPP__