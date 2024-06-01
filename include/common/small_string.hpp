#ifndef __DARK_COMMON_SMALL_STRING_HPP__
#define __DARK_COMMON_SMALL_STRING_HPP__

#include <cstring>
#include <llvm/Support/raw_ostream.h>
#include <ostream>

namespace dark::detail {
    struct SmallStringRef {
        constexpr SmallStringRef() noexcept
            : m_data(nullptr)
            , m_size(0)
        {}
        constexpr SmallStringRef(char const* data, std::size_t size)
            : m_data(data)
            , m_size(std::numeric_limits<std::uint16_t>::max() < size ? std::numeric_limits<std::uint16_t>::max() : static_cast<std::uint16_t>(size))
        {}

        constexpr SmallStringRef(char const* data)
            : m_data(data)
            , m_size(0)
        {
            while (m_data[m_size] != '\0') { ++m_size; }
        }

        template <std::size_t N>
        constexpr SmallStringRef(char const (&data)[N])
            : SmallStringRef(data, (N == 0 ? 0 : N - 1))
        {}
        constexpr SmallStringRef(llvm::StringRef str)
            : SmallStringRef(str.data(), str.size())
        {}
        constexpr SmallStringRef(std::string_view str)
            : SmallStringRef(str.data(), str.size())
        {}
        constexpr SmallStringRef(SmallStringRef const&) = default;
        constexpr SmallStringRef(SmallStringRef&&) = default;
        constexpr SmallStringRef& operator=(SmallStringRef const&) = default;
        constexpr SmallStringRef& operator=(SmallStringRef&&) = default;
        constexpr ~SmallStringRef() = default;

        constexpr char const* begin() const noexcept { return m_data; }
        constexpr char const* end() const noexcept { return m_data + size(); }

        constexpr char const* data() const noexcept { return m_data; }
        constexpr std::size_t size() const noexcept { return static_cast<std::size_t>(m_size); }

        constexpr char const& operator[](std::size_t index) const noexcept { return m_data[index]; }
        constexpr operator llvm::StringRef() const noexcept { return llvm::StringRef(m_data, size()); }
        constexpr operator std::string_view() const noexcept { return std::string_view(m_data, size()); }

        constexpr auto empty() const noexcept -> bool { return size() == 0; }

        constexpr operator bool() const noexcept { return (m_data != nullptr) && !empty(); }

        friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, SmallStringRef const& str) {
            return os << llvm::StringRef(str.data(), str.size());
        }

        friend std::ostream& operator<<(std::ostream& os, SmallStringRef const& str) {
            return os << std::string_view(str.data(), str.size());
        }

    private:
        char const* m_data;
        std::uint16_t m_size;
    };
} // namespace dark::detail

#endif // __DARK_COMMON_SMALL_STRING_HPP__