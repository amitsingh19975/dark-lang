#ifndef __DARK_ADT_BUFFER_HPP__
#define __DARK_ADT_BUFFER_HPP__

#include "common/assert.hpp"
#include <iterator>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/raw_ostream.h>
#include <string_view>
#include <type_traits>
namespace dark {

    template <typename T>
    struct Buffer {
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr Buffer() noexcept
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0)
        {}

        explicit Buffer(pointer data, size_type capacity) noexcept
            : m_data(data)
            , m_size(0)
            , m_capacity(capacity)
        {}

        constexpr Buffer(Buffer const&) noexcept = default;
        constexpr Buffer& operator=(Buffer const&) noexcept = default;
        constexpr Buffer(Buffer&&) noexcept = default;
        constexpr Buffer& operator=(Buffer&&) noexcept = default;
        ~Buffer() noexcept = default;

        constexpr pointer data() noexcept { return m_data; }
        constexpr const_pointer data() const noexcept { return m_data; }
        constexpr size_type size() const noexcept { return m_size; }
        constexpr size_type capacity() const noexcept { return m_capacity; }
        constexpr bool empty() const noexcept { return m_size == 0; }

        constexpr void clear() noexcept { m_size = 0; }
        
        void push_back(const_reference value) {
            dark_assert(m_size < m_capacity);    
            m_data[m_size++] = value;
        }

        void pop_back() {
            dark_assert(m_size > 0);
            --m_size;
        }

        void push_back(const_pointer in_ptr, size_type count) {
            dark_assert(size() + count <= capacity());
            std::copy_n(in_ptr, count, data() + size());
            m_size += count;
        }
        
        void push_back(std::string_view data, size_type count = std::string_view::npos) requires std::is_same_v<T, char> {
            auto temp = data.substr(0, count);
            push_back(data.data(), temp.size());
        }

        constexpr reference operator[](size_type index) noexcept {
            return m_data[index];
        }

        constexpr const_reference operator[](size_type index) const noexcept {
            return m_data[index];
        }

        constexpr reference front() noexcept {
            dark_assert(m_size > 0);
            return m_data[0];
        }

        constexpr const_reference front() const noexcept {
            dark_assert(m_size > 0);
            return m_data[0];
        }

        constexpr reference back() noexcept {
            dark_assert(m_size > 0);
            return m_data[m_size - 1];
        }

        constexpr const_reference back() const noexcept {
            dark_assert(m_size > 0);
            return m_data[m_size - 1];
        }

        constexpr iterator begin() noexcept { return m_data; }
        constexpr const_iterator begin() const noexcept { return m_data; }
        constexpr const_iterator cbegin() const noexcept { return m_data; }
        constexpr iterator end() noexcept { return m_data + m_size; }
        constexpr const_iterator end() const noexcept { return m_data + m_size; }
        constexpr const_iterator cend() const noexcept { return m_data + m_size; }

        constexpr pointer rbegin() noexcept { return std::reverse_iterator(end()); }
        constexpr const_pointer rbegin() const noexcept { return std::reverse_iterator(end()); }
        constexpr const_pointer crbegin() const noexcept { return std::reverse_iterator(end()); }
        constexpr pointer rend() noexcept { return std::reverse_iterator(begin()); }
        constexpr const_pointer rend() const noexcept { return std::reverse_iterator(begin()); }
        constexpr const_pointer crend() const noexcept { return std::reverse_iterator(begin()); }
    
        constexpr size_type space_left() const noexcept { return m_capacity - m_size; }
        constexpr auto set_size(size_type size) noexcept { m_size = size; }

        constexpr bool operator<=>(Buffer const&) const = default;

    private:
        pointer m_data;
        size_type m_size;
        size_type m_capacity;
    };

} // namespace dark

#endif // __DARK_ADT_BUFFER_HPP__