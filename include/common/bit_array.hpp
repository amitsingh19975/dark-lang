#ifndef __DARK_COMMON_BIT_ARRAY_HPP__
#define __DARK_COMMON_BIT_ARRAY_HPP__

#include <array>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <iterator>
#include <type_traits>

namespace dark {
    template <std::size_t N>
    struct alignas(8) BitArray {
        constexpr BitArray() noexcept = default;
        constexpr BitArray(const BitArray&) noexcept = default;
        constexpr BitArray(BitArray&&) noexcept = default;
        constexpr BitArray& operator=(const BitArray&) noexcept = default;
        constexpr BitArray& operator=(BitArray&&) noexcept = default;
        constexpr ~BitArray() noexcept = default;

        template <typename... Ts>
            requires (sizeof...(Ts) == N && (std::same_as<std::decay_t<std::remove_cvref_t<Ts>>, bool> && ...))
        constexpr BitArray(Ts... ts) noexcept
            : BitArray(std::array{ ts... })
        {}

        constexpr BitArray(std::array<bool, N> const& arr) noexcept
            : BitArray()
        {
            for (std::size_t i = 0; i < N; ++i) {
                set(i, arr[i]);
            }
        }

        constexpr bool operator[](std::size_t index) const noexcept {
            return (data[index / 8] & (1 << (index % 8))) != 0;
        }

        constexpr void set(std::size_t index, bool value) noexcept {
            if (value) data[index / 8] |= (1 << (index % 8));
            else data[index / 8] &= ~(1 << (index % 8));
        }

        constexpr void reset() noexcept {
            for (std::size_t i = 0; i < m_size; ++i) {
                data[i] = 0;
            }
        }

        constexpr void flip() noexcept {
            for (std::size_t i = 0; i < m_size; ++i) {
                data[i] = ~data[i];
            }
        }

        constexpr BitArray& operator&=(const BitArray& other) noexcept {
            for (std::size_t i = 0; i < m_size; ++i) {
                data[i] &= other.data[i];
            }
            return *this;
        }

        constexpr BitArray& operator|=(const BitArray& other) noexcept {
            for (std::size_t i = 0; i < m_size; ++i) {
                data[i] |= other.data[i];
            }
            return *this;
        }

        constexpr BitArray& operator^=(const BitArray& other) noexcept {
            for (std::size_t i = 0; i < m_size; ++i) {
                data[i] ^= other.data[i];
            }
            return *this;
        }

        constexpr BitArray operator~() const noexcept {
            auto res = BitArray{};
            for (std::size_t i = 0; i < m_size; ++i) {
                res.data[i] = ~data[i];
            }
            return res;
        }

        constexpr BitArray operator&(const BitArray& other) const noexcept {
            auto res = BitArray{};
            for (std::size_t i = 0; i < m_size; ++i) {
                res.data[i] = data[i] & other.data[i];
            }
            return res;
        }

        constexpr BitArray operator|(const BitArray& other) const noexcept {
            auto res = BitArray{};
            for (std::size_t i = 0; i < m_size; ++i) {
                res.data[i] = data[i] | other.data[i];
            }
            return res;
        }

        constexpr BitArray operator^(const BitArray& other) const noexcept {
            auto res = BitArray{};
            for (std::size_t i = 0; i < m_size; ++i) {
                res.data[i] = data[i] ^ other.data[i];
            }
            return res;
        }

        constexpr auto operator<=>(const BitArray& other) const noexcept -> std::strong_ordering {
            for (std::size_t i = 0; i < m_size; ++i) {
                if (data[i] < other.data[i]) return std::strong_ordering::less;
                if (data[i] > other.data[i]) return std::strong_ordering::greater;
            }
            return std::strong_ordering::equal;
        }

        constexpr std::size_t size() const noexcept { return N; }
        constexpr std::size_t actual_size() const noexcept { return m_size; }

        struct Iterator;

        constexpr Iterator begin() noexcept { return Iterator{*this, 0}; }
        constexpr Iterator begin() const noexcept { return Iterator{*this, 0}; }

        constexpr Iterator end() noexcept { return Iterator{*this, N}; }
        constexpr Iterator end() const noexcept { return Iterator{*this, N}; }
    private:
        using base_type = std::uint8_t;
        static constexpr std::size_t m_size = (N / 8) + static_cast<std::size_t>(N % 8 != 0);
    private:
        base_type data[m_size] = {0};
    };

    template <typename... Ts>
    BitArray(Ts...) -> BitArray<sizeof...(Ts)>;
    
    template <std::size_t N>
    BitArray(std::array<bool, N>) -> BitArray<N>;

    template <std::size_t N>
    struct BitArray<N>::Iterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = bool*;
        using reference = bool&;

        constexpr Iterator() noexcept = default;
        constexpr Iterator(const Iterator&) noexcept = default;
        constexpr Iterator(Iterator&&) noexcept = default;
        constexpr Iterator& operator=(const Iterator&) noexcept = default;
        constexpr Iterator& operator=(Iterator&&) noexcept = default;
        constexpr ~Iterator() noexcept = default;

        constexpr Iterator(BitArray& bit_array, std::size_t index) noexcept
            : m_bit_array(bit_array)
            , m_index(index)
        {}

        constexpr bool operator*() const noexcept {
            return m_bit_array[m_index];
        }

        constexpr Iterator& operator++() noexcept {
            ++m_index;
            return *this;
        }

        constexpr Iterator operator++(int) noexcept {
            auto copy = *this;
            ++m_index;
            return copy;
        }

        constexpr Iterator& operator--() noexcept {
            --m_index;
            return *this;
        }

        constexpr Iterator operator--(int) noexcept {
            auto copy = *this;
            --m_index;
            return copy;
        }

        constexpr Iterator& operator+=(std::ptrdiff_t n) noexcept {
            m_index += n;
            return *this;
        }

        constexpr Iterator& operator-=(std::ptrdiff_t n) noexcept {
            m_index -= n;
            return *this;
        }

        constexpr Iterator operator+(std::ptrdiff_t n) const noexcept {
            auto copy = *this;
            copy += n;
            return copy;
        }

        constexpr Iterator operator-(std::ptrdiff_t n) const noexcept {
            auto copy = *this;
            copy -= n;
            return copy;
        }

        constexpr std::ptrdiff_t operator-(const Iterator& other) const noexcept {
            return m_index - other.m_index;
        }

        constexpr auto operator<=>(const Iterator& other) const noexcept -> std::strong_ordering {
            return m_index <=> other.m_index;
        }

    private:
        BitArray& m_bit_array;
        std::size_t m_index;
    };

} // namespace dark

#endif // __DARK_COMMON_BIT_ARRAY_HPP__