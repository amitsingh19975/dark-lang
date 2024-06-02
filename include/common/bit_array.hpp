#ifndef __DARK_COMMON_BIT_ARRAY_HPP__
#define __DARK_COMMON_BIT_ARRAY_HPP__

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <iterator>
#include <type_traits>

namespace dark {
    template <std::size_t N>
    class alignas(8) BitArray {
        using base_type = std::uint8_t;
        static constexpr std::size_t char_bit = CHAR_BIT * sizeof(base_type);
        static constexpr std::size_t m_size = ((N + char_bit - 1) / char_bit);
    public:
        using size_type = std::size_t;

        template <bool isConst>
        struct BitWrapper{
            using base_type = std::conditional_t<isConst, std::add_const_t<BitArray::base_type>, BitArray::base_type>;
            using reference_t = std::add_lvalue_reference_t<base_type>;

            constexpr BitWrapper(reference_t chunk, std::size_t index) noexcept
                : chunk(chunk)
                , index(index)
            {}
            constexpr BitWrapper(const BitWrapper&) noexcept = default;
            constexpr BitWrapper(BitWrapper&&) noexcept = default;
            constexpr BitWrapper& operator=(const BitWrapper&) noexcept = default;
            constexpr BitWrapper& operator=(BitWrapper&&) noexcept = default;
            constexpr ~BitWrapper() noexcept = default;
            
            constexpr BitWrapper& operator=(bool value) noexcept requires (!isConst) {
                using value_type = BitArray::base_type;
                chunk = value_type(value ? (chunk | (value_type{1} << index)) : (chunk & ~(value_type{1} << index)));
                return *this;
            }

            constexpr operator bool() const noexcept {
                return (chunk & (1 << index)) != 0;
            }
        private:
            reference_t chunk;
            size_type index;
        };

        using const_bit_wrapper = BitWrapper<true>;
        using bit_wrapper = BitWrapper<false>;

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
            for (size_type i = 0; i < N; ++i) {
                set(i, arr[i]);
            }
        }

        constexpr const_bit_wrapper operator[](size_type index) const noexcept {
            return {data[index / char_bit], index % char_bit};
        }
        
        constexpr bit_wrapper operator[](size_type index) noexcept {
            return {data[index / char_bit], index % char_bit};
        }

        constexpr void set(size_type index, bool value) noexcept {
            if (value) data[index / char_bit] |= (1 << (index % char_bit));
            else data[index / char_bit] &= ~(1 << (index % char_bit));
        }

        constexpr void reset() noexcept {
            for (size_type i = 0; i < m_size; ++i) {
                data[i] = 0;
            }
        }

        constexpr void flip() noexcept {
            for (size_type i = 0; i < m_size; ++i) {
                data[i] = ~data[i];
            }
        }

        constexpr BitArray& operator&=(const BitArray& other) noexcept {
            for (size_type i = 0; i < m_size; ++i) {
                data[i] &= other.data[i];
            }
            return *this;
        }

        constexpr BitArray& operator|=(const BitArray& other) noexcept {
            for (size_type i = 0; i < m_size; ++i) {
                data[i] |= other.data[i];
            }
            return *this;
        }

        constexpr BitArray& operator^=(const BitArray& other) noexcept {
            for (size_type i = 0; i < m_size; ++i) {
                data[i] ^= other.data[i];
            }
            return *this;
        }

        constexpr BitArray operator~() const noexcept {
            auto res = BitArray{};
            for (size_type i = 0; i < m_size; ++i) {
                res.data[i] = ~data[i];
            }
            return res;
        }

        constexpr BitArray operator&(const BitArray& other) const noexcept {
            auto res = BitArray{};
            for (size_type i = 0; i < m_size; ++i) {
                res.data[i] = data[i] & other.data[i];
            }
            return res;
        }

        constexpr BitArray operator|(const BitArray& other) const noexcept {
            auto res = BitArray{};
            for (size_type i = 0; i < m_size; ++i) {
                res.data[i] = data[i] | other.data[i];
            }
            return res;
        }

        constexpr BitArray operator^(const BitArray& other) const noexcept {
            auto res = BitArray{};
            for (size_type i = 0; i < m_size; ++i) {
                res.data[i] = data[i] ^ other.data[i];
            }
            return res;
        }

        constexpr auto operator<=>(const BitArray& other) const noexcept -> std::strong_ordering {
            for (size_type i = 0; i < m_size; ++i) {
                if (data[i] < other.data[i]) return std::strong_ordering::less;
                if (data[i] > other.data[i]) return std::strong_ordering::greater;
            }
            return std::strong_ordering::equal;
        }

        constexpr size_type size() const noexcept { return N; }
        constexpr size_type actual_size() const noexcept { return m_size; }

        struct Iterator;

        constexpr Iterator begin() noexcept { return Iterator{*this, 0}; }
        constexpr Iterator begin() const noexcept { return Iterator{*this, 0}; }

        constexpr Iterator end() noexcept { return Iterator{*this, N}; }
        constexpr Iterator end() const noexcept { return Iterator{*this, N}; }
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