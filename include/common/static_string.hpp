#ifndef __DARK_COMMON_CSTR_HPP__
#define __DARK_COMMON_CSTR_HPP__

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <ostream>
#include <string_view>
#include "assert.hpp"

namespace dark {

    template <typename CharT, std::size_t N>
    struct BasicStaticString {
        CharT m_data[N + 1];

        constexpr BasicStaticString(const CharT (&str)[N]) {
            for (std::size_t i = 0; i < N; ++i) {
                m_data[i] = str[i];
            }
            m_data[N] = '\0';
        }

        constexpr BasicStaticString(const llvm::StringRef& str) {
            dark_assert(str.size() == N, "String size mismatch");
            for (std::size_t i = 0; i < N; ++i) {
                m_data[i] = str[i];
            }
            m_data[N] = '\0';
        }

        friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const BasicStaticString& str) {
            os << str.m_data;
            return os;
        }

        friend std::ostream& operator<<(std::ostream& os, const BasicStaticString& str) {
            os << str.m_data;
            return os;
        }

        constexpr auto operator==(const std::string_view& other) const -> bool {
            for (std::size_t i = 0; i < N; ++i) {
                if (m_data[i] != other[i]) {
                    return false;
                }
            }
            return true;
        }

        constexpr operator std::string_view() const {
            return std::string_view(m_data, N);
        }

        constexpr operator llvm::StringRef() const {
            return llvm::StringRef(m_data, N);
        }

        constexpr auto size() const -> std::size_t {
            return N;
        }

        constexpr auto data() const -> const CharT* {
            return m_data;
        }

        constexpr auto operator[](std::size_t index) const -> CharT {
            return m_data[index];
        }
    };

    template <typename CharT, std::size_t N>
    BasicStaticString(const CharT (&)[N]) -> BasicStaticString<CharT, N>;

    template <std::size_t N>
    struct StaticString : BasicStaticString<char, N> {
        using BasicStaticString<char, N>::BasicStaticString;

        template<typename... Ts>
        constexpr StaticString(Ts&&... ts) : BasicStaticString<char, N>{std::forward<Ts>(ts)...} {}
    };

    template <std::size_t N>
    StaticString(const char (&)[N]) -> StaticString<N>;

} // namespace dark

#endif // __DARK_COMMON_CSTR_HPP__